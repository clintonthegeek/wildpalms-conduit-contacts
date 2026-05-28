#include "contactsvcardtranscoder.h"

#include "palm/codecs/contactcodec.h"
#include "palm/codecs/kde_pim_convert.h"
#include "palm/calendar/categorymappingstore.h"

#include <KContacts/Addressee>
#include <KContacts/VCardConverter>

#include <QString>

namespace WildPalms::ContactsPlugin {

QByteArray encodePalmToVcard(const WildPalms::PalmSync::PalmRecord &record,
                             const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                             const QString &dbName)
{
    if (record.data.isEmpty()) return {};
    auto contact = WildPalms::PalmCodecs::decodeContact(QByteArrayView(record.data));
    if (!contact.has_value()) return {};

    KContacts::Addressee addressee = WildPalms::PalmCodecs::toAddressee(*contact);

    addressee.insertCustom(QStringLiteral("WP-PALM"),
                           QStringLiteral("CATEGORY-SLOT"),
                           QString::number(record.category));
    addressee.insertCustom(QStringLiteral("WP-PALM"),
                           QStringLiteral("RECORDID"),
                           QString::number(record.recordId));
    // The CLASS property exists in vCard 3.0 but was removed in 4.0, so
    // KContacts::Secrecy doesn't survive a v4.0 round-trip. Stash the
    // bit in an X- property so the decoder can recover it regardless of
    // the wire format.
    if (record.isSecret()) {
        addressee.setSecrecy(KContacts::Secrecy(KContacts::Secrecy::Private));
        addressee.insertCustom(QStringLiteral("WP-PALM"),
                               QStringLiteral("SECRET"),
                               QStringLiteral("1"));
    }

    // Carry the Palm category slot as the vCard CATEGORIES property (name-based).
    // libkalburator's vcard4<->canon stage lifts it into canon `categories`.
    // If the slot has a name in the store, it is written; otherwise nothing is
    // emitted and the return trip falls back to Unfiled (slot 0).
    if (cats && record.category != 0) {
        const QString nm = cats->slotName(dbName, record.category);
        if (!nm.isEmpty()) addressee.setCategories(QStringList{nm});
    }

    KContacts::VCardConverter conv;
    return conv.createVCard(addressee, KContacts::VCardConverter::v4_0);
}

std::optional<WildPalms::PalmSync::PalmRecord>
decodeVcardToPalm(const QByteArray &vcardBytes,
                  const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                  const QString &dbName)
{
    if (vcardBytes.isEmpty()) return std::nullopt;

    KContacts::VCardConverter conv;
    auto list = conv.parseVCards(vcardBytes);
    if (list.isEmpty()) return std::nullopt;

    const KContacts::Addressee &addressee = list.first();
    if (addressee.isEmpty()) return std::nullopt;

    WildPalms::PalmCodecs::Contact contact = WildPalms::PalmCodecs::fromAddressee(addressee);
    QByteArray bytes = WildPalms::PalmCodecs::encodeContact(contact);
    if (bytes.isEmpty()) return std::nullopt;

    // Map the first CATEGORIES name back to a Palm slot. No store or no
    // categories => slot 0 (Unfiled).
    const int slot = (cats && !addressee.categories().isEmpty())
        ? cats->slotForName(dbName, addressee.categories().constFirst())
        : 0;

    WildPalms::PalmSync::PalmRecord pr;
    pr.data     = bytes;
    pr.category = static_cast<std::uint8_t>(slot);

    const bool secrecyPrivate =
        addressee.secrecy().type() == KContacts::Secrecy::Private
     || addressee.secrecy().type() == KContacts::Secrecy::Confidential;
    const QString stashedSecret = addressee.custom(QStringLiteral("WP-PALM"),
                                                   QStringLiteral("SECRET"));
    if (secrecyPrivate || stashedSecret == QStringLiteral("1")) {
        pr.attributes |= WildPalms::PalmSync::PalmRecord::AttrSecret;
    }

    const QString rid = addressee.custom(QStringLiteral("WP-PALM"),
                                         QStringLiteral("RECORDID"));
    if (!rid.isEmpty()) {
        bool ok = false;
        const std::uint32_t parsed = rid.toUInt(&ok);
        if (ok) pr.recordId = parsed;
    }
    return pr;
}

} // namespace WildPalms::ContactsPlugin
