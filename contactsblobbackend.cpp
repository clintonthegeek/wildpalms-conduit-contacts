#include "contactsblobbackend.h"

#include "palm/calendar/categorymappingstore.h"
#include "palm/sync/palmbackend.h"
#include "palm/sync/palmrecord.h"

#include "backendrecord.h"
#include "collectioninfo.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QStringList>

namespace WildPalms::ContactsPlugin {

namespace {

QString sha256Hex(const QByteArray &bytes)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

QString idForPalmRecord(std::uint32_t recordId)
{
    return WildPalms::PalmSync::PalmBackend::encodeRecordId(
        QStringLiteral("AddressDB"), recordId);
}

bool decodeId(const QString &id, std::uint32_t *outRecordId)
{
    // PalmBackend::decodeRecordId reconstructs the dbName by uppercasing
    // only the first letter ("palm:address:N" -> "AddressDB"). For
    // "AddressDB" this happens to round-trip exactly, but compare
    // case-insensitively to mirror the other plugins and to stay robust
    // against any future change to the encoder.
    QString dbName;
    return WildPalms::PalmSync::PalmBackend::decodeRecordId(id, &dbName, outRecordId)
        && dbName.compare(QLatin1String("AddressDB"), Qt::CaseInsensitive) == 0;
}

} // namespace

ContactsBlobBackend::ContactsBlobBackend(
    WildPalms::PalmSync::PalmBackend *palmBackend,
    const WildPalms::PalmCalendar::CategoryMappingStore *categoryStore,
    QObject *parent)
    : QObject(parent)
    , m_palmBackend(palmBackend)
    , m_categoryStore(categoryStore)
{
}

ContactsBlobBackend::~ContactsBlobBackend() = default;

QString ContactsBlobBackend::backendId()   const { return QStringLiteral("palm-contacts"); }
QString ContactsBlobBackend::displayName() const { return QStringLiteral("Palm Contacts"); }
bool    ContactsBlobBackend::isAvailable() const
{
    return m_palmBackend != nullptr && m_palmBackend->isAvailable();
}

QList<Kalburator::Sync::CollectionInfo> ContactsBlobBackend::availableCollections()
{
    QList<Kalburator::Sync::CollectionInfo> out;

    Kalburator::Sync::CollectionInfo unfiled;
    unfiled.id   = collectionIdForSlot(0);
    unfiled.name = QStringLiteral("Unfiled");
    unfiled.type = QStringLiteral("contacts");
    out.append(unfiled);

    if (!m_categoryStore) return out;

    const QList<int> populated = m_categoryStore->populatedSlots(
        QStringLiteral("AddressDB"));
    for (int slot : populated) {
        Kalburator::Sync::CollectionInfo info;
        info.id   = collectionIdForSlot(slot);
        info.name = m_categoryStore->slotName(
            QStringLiteral("AddressDB"), slot);
        info.type = QStringLiteral("contacts");
        out.append(info);
    }
    return out;
}

Kalburator::Sync::CollectionInfo ContactsBlobBackend::collectionInfo(
    const QString &collectionId)
{
    for (const auto &c : availableCollections()) {
        if (c.id == collectionId) return c;
    }
    return {};
}

QString ContactsBlobBackend::createCollection(
    const Kalburator::Sync::CollectionInfo &)
{
    // Slots are governed by the device's AppInfo block — plugin
    // doesn't create new ones. Returning empty signals "not supported".
    return {};
}

QList<Kalburator::Sync::BackendRecord> ContactsBlobBackend::loadRecords(
    const QString &collectionId)
{
    const int slot = slotFromCollectionId(collectionId);
    QList<Kalburator::Sync::BackendRecord> out;
    if (slot < 0 || !m_palmBackend) return out;

    const auto records = m_palmBackend->loadPalmRecords(QStringLiteral("AddressDB"));
    for (const auto &pr : records) {
        if (static_cast<int>(pr.category) != slot) continue;
        if (pr.isDeleted()) continue;

        Kalburator::Sync::BackendRecord br;
        br.id           = idForPalmRecord(pr.recordId);
        br.data         = pr.toWireBytes();
        br.type         = QStringLiteral("contacts");
        br.lastModified = pr.lastModified;
        br.contentHash  = sha256Hex(br.data);
        out.append(br);
    }
    return out;
}

std::optional<Kalburator::Sync::BackendRecord>
ContactsBlobBackend::loadRecord(const QString &recordId)
{
    std::uint32_t rid = 0;
    if (!decodeId(recordId, &rid) || !m_palmBackend) return std::nullopt;
    auto pr = m_palmBackend->loadPalmRecord(QStringLiteral("AddressDB"), rid);
    if (!pr) return std::nullopt;

    Kalburator::Sync::BackendRecord br;
    br.id           = recordId;
    br.data         = pr->toWireBytes();
    br.type         = QStringLiteral("contacts");
    br.lastModified = pr->lastModified;
    br.contentHash  = sha256Hex(br.data);
    return br;
}

QString ContactsBlobBackend::createRecord(
    const QString &collectionId,
    const Kalburator::Sync::BackendRecord &record)
{
    const int slot = slotFromCollectionId(collectionId);
    if (slot < 0 || !m_palmBackend) return {};
    if (record.data.isEmpty()) return {};

    // Phase Ia: callers (the engine, after demoting through the
    // registered edge) hand us palm-native bytes. Deserialize directly.
    auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(record.data);
    pr.category     = static_cast<std::uint8_t>(slot);
    pr.recordId     = 0;   // device assigns
    pr.lastModified = record.lastModified.isValid()
        ? record.lastModified
        : QDateTime::currentDateTimeUtc();

    const auto newId = m_palmBackend->createPalmRecord(
        QStringLiteral("AddressDB"), pr);
    if (newId == 0) return {};
    return idForPalmRecord(newId);
}

bool ContactsBlobBackend::updateRecord(
    const Kalburator::Sync::BackendRecord &record)
{
    std::uint32_t rid = 0;
    if (!decodeId(record.id, &rid) || !m_palmBackend) return false;
    if (record.data.isEmpty()) return false;

    // Look up the existing record to recover its slot (the
    // BackendRecord's id alone doesn't carry the slot).
    auto existing = m_palmBackend->loadPalmRecord(
        QStringLiteral("AddressDB"), rid);
    if (!existing) return false;
    const int slot = static_cast<int>(existing->category);

    // Phase Ia: callers hand us palm-native bytes (engine demotes
    // through the registered edge). Deserialize directly.
    auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(record.data);
    pr.recordId     = rid;
    pr.category     = static_cast<std::uint8_t>(slot);
    pr.lastModified = record.lastModified.isValid()
        ? record.lastModified
        : QDateTime::currentDateTimeUtc();

    return m_palmBackend->updatePalmRecord(QStringLiteral("AddressDB"), pr);
}

bool ContactsBlobBackend::deleteRecord(const QString &recordId)
{
    if (!m_palmBackend) return false;
    std::uint32_t rid = 0;
    if (!decodeId(recordId, &rid)) return false;
    // Use the dbName-aware delete so the canonical "AddressDB" name is
    // used (avoids any asymmetry in PalmBackend::decodeRecordId).
    return m_palmBackend->deletePalmRecord(QStringLiteral("AddressDB"), rid);
}

QList<Kalburator::Sync::BackendRecord>
ContactsBlobBackend::modifiedSince(const QString &collectionId,
                                   const QDateTime &since)
{
    const int slot = slotFromCollectionId(collectionId);
    QList<Kalburator::Sync::BackendRecord> out;
    if (slot < 0 || !m_palmBackend) return out;

    // Forward to PalmBackend's underlying list, then filter.
    const auto records = m_palmBackend->loadPalmRecords(QStringLiteral("AddressDB"));
    for (const auto &pr : records) {
        if (static_cast<int>(pr.category) != slot) continue;
        if (since.isValid() && pr.lastModified <= since) continue;

        Kalburator::Sync::BackendRecord br;
        br.id           = idForPalmRecord(pr.recordId);
        br.data         = pr.toWireBytes();
        br.type         = QStringLiteral("contacts");
        br.lastModified = pr.lastModified;
        br.contentHash  = sha256Hex(br.data);
        out.append(br);
    }
    return out;
}

QStringList ContactsBlobBackend::deletedSince(const QString & /*collectionId*/,
                                              const QDateTime &since)
{
    if (!m_palmBackend) return {};
    // KNOWN LIMITATION (mirrors TodoBlobBackend / CalendarBlobBackend):
    // returns deletions from ALL category slots, not just the requested
    // collection. The record's category byte is lost when the record is
    // deleted. BlobSyncEngine tolerates over-broad returns. Tighten when
    // PalmBackend grows a slot-aware deletedSince variant (post-E.15).
    const QString sourceCollection =
        WildPalms::PalmSync::PalmBackend::encodeCollectionId(
            QStringLiteral("AddressDB"));
    return m_palmBackend->deletedSince(sourceCollection, since);
}

bool ContactsBlobBackend::supportsDeleteTracking() const
{
    return m_palmBackend && m_palmBackend->supportsDeleteTracking();
}

int ContactsBlobBackend::slotFromCollectionId(const QString &collectionId)
{
    static constexpr QLatin1String prefix(CollectionPrefix);
    if (!collectionId.startsWith(prefix)) return -1;
    bool ok = false;
    const int slot = collectionId.mid(prefix.size()).toInt(&ok);
    if (!ok || slot < 0 || slot > 15) return -1;
    return slot;
}

QString ContactsBlobBackend::collectionIdForSlot(int slot)
{
    return QString::fromLatin1(CollectionPrefix) + QString::number(slot);
}

} // namespace WildPalms::ContactsPlugin
