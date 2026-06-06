#ifndef WILDPALMS_CONTACTS_PALMCONTACTSBACKEND_H
#define WILDPALMS_CONTACTS_PALMCONTACTSBACKEND_H

#include "syncbackendbase.h"

#include <QObject>

namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::ContactsPlugin {

class PalmContactsBackend final : public Kalburator::Sync::SyncBackendBase
{
    Q_OBJECT
public:
    static constexpr const char *BackendId        = "palm-contacts";
    static constexpr const char *PalmDbName       = "AddressDB";
    static constexpr const char *CollectionPrefix = "palm:contact/";

    explicit PalmContactsBackend(
        WildPalms::PalmSync::PalmBackend *palmBackend,
        const WildPalms::PalmCalendar::CategoryMappingStore *categoryStore,
        QObject *parent = nullptr);
    ~PalmContactsBackend() override;

    // SyncBackend identity
    QString backendType() const override { return QStringLiteral("palm-contacts"); }
    QList<Kalburator::Shape::Shape> nativeShapes() const override;

    // IBlobBackend identity
    QString backendId()   const override;
    QString displayName() const override;
    bool    isAvailable() const override;

    // IBlobBackend collections
    QList<Kalburator::Sync::CollectionInfo> availableCollections() override;
    Kalburator::Sync::CollectionInfo collectionInfo(const QString &collectionId) override;
    QString createCollection(const Kalburator::Sync::CollectionInfo &info) override;

    // IBlobBackend records
    QList<Kalburator::Sync::BackendRecord> loadRecords(const QString &collectionId) override;
    std::optional<Kalburator::Sync::BackendRecord> loadRecord(const QString &recordId) override;
    QString createRecord(const QString &collectionId,
                         const Kalburator::Sync::BackendRecord &record) override;
    bool    updateRecord(const Kalburator::Sync::BackendRecord &record) override;
    bool    deleteRecord(const QString &recordId) override;

    /// Clobber-sync entry point: drop every record from the Palm-side
    /// AddressDB so the subsequent push from the hub lands into an empty
    /// target. Used by SyncEngine's ExecutionOverride::clobber path
    /// (libkalburator v0.65+). Always clears the entire AddressDB
    /// regardless of which sub-collection (slot or domain-level) the
    /// engine names — Palm storage is whole-DB.
    bool    wipeCollection(const QString &collectionId) override;

    // IBlobBackend change detection
    QList<Kalburator::Sync::BackendRecord> modifiedSince(
        const QString &collectionId, const QDateTime &since) override;
    QStringList deletedSince(const QString &collectionId, const QDateTime &since) override;
    bool        supportsDeleteTracking() const override;

    // Helpers
    static int     slotFromCollectionId(const QString &collectionId);
    static QString collectionIdForSlot(int slot);


Q_SIGNALS:
    void recordCreated(const QString &recordId);
    void recordUpdated(const QString &recordId);
    void recordDeleted(const QString &recordId);
    void errorOccurred(const QString &error);
    void progressUpdated(int current, int total, const QString &message);

private:
    WildPalms::PalmSync::PalmBackend                    *m_palmBackend   = nullptr;
    const WildPalms::PalmCalendar::CategoryMappingStore *m_categoryStore = nullptr;
};

} // namespace WildPalms::ContactsPlugin

#endif // WILDPALMS_CONTACTS_PALMCONTACTSBACKEND_H
