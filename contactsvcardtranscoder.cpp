#include "contactsvcardtranscoder.h"

#include "palm/codecs/contactcodec.h"
#include "palm/codecs/kde_pim_convert.h"

#include <KContacts/Addressee>
#include <KContacts/VCardConverter>

#include <QString>

namespace WildPalms::ContactsPlugin {

namespace {

constexpr const char *kCategorySlotProp = "X-WP-PALM-CATEGORY-SLOT";
constexpr const char *kRecordIdProp     = "X-WP-PALM-RECORDID";

} // namespace

QByteArray encodePalmToVcard(const WildPalms::PalmSync::PalmRecord &record)
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

    KContacts::VCardConverter conv;
    return conv.createVCard(addressee, KContacts::VCardConverter::v4_0);
}

std::optional<WildPalms::PalmSync::PalmRecord>
decodeVcardToPalm(const QByteArray &vcardBytes, int slotHint)
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

    WildPalms::PalmSync::PalmRecord pr;
    pr.data     = bytes;
    pr.category = static_cast<std::uint8_t>(slotHint);

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
