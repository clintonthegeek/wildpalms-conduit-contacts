#ifndef WILDPALMS_CONTACTS_CONTACTSVCARDTRANSCODER_H
#define WILDPALMS_CONTACTS_CONTACTSVCARDTRANSCODER_H

#include <optional>

#include <QByteArray>
#include <QString>

#include "palm/sync/palmrecord.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::ContactsPlugin {

/**
 * @brief Encode a Palm Address record into a vCard 4.0 byte string.
 *
 * Composes WildPalms::PalmCodecs::decodeContact (Palm bytes -> Contact
 * POD) with WildPalms::PalmCodecs::toAddressee (Contact -> Addressee),
 * then serialises via KContacts::VCardConverter::v4_0. Stamps
 * X-WP-PALM-CATEGORY-SLOT and X-WP-PALM-RECORDID extension properties
 * on the Addressee for round-trip.
 *
 * If `cats` is non-null and `record.category != 0`, the slot's display
 * name (via CategoryMappingStore::slotName(dbName, slot)) is written to
 * the Addressee's CATEGORIES property — libkalburator's vcard4<->canon
 * stage then carries it into canon `categories`. A null `cats` emits no
 * CATEGORIES (degrades gracefully). If the slot has no name in the store
 * the field is omitted; on the return trip that yields slot 0 (Unfiled).
 *
 * Returns empty QByteArray on decode failure or empty input.
 */
QByteArray encodePalmToVcard(const WildPalms::PalmSync::PalmRecord &record,
                             const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                             const QString &dbName);

/**
 * @brief Decode vCard bytes (v3.0 or v4.0) into a PalmRecord.
 *
 * The category slot is derived from the Addressee's CATEGORIES property:
 * if `cats` is non-null and the addressee carries at least one category,
 * the first category name is mapped back to a slot via
 * CategoryMappingStore::slotForName(dbName, name). A null `cats` (or no
 * categories) yields slot 0 (Unfiled). The X-WP-PALM-RECORDID property,
 * if present, populates `PalmRecord::recordId`; otherwise recordId stays
 * 0 and the device assigns on write. The KContacts::Addressee's
 * `secrecy()` is mapped to PalmRecord::AttrSecret via `fromAddressee`'s
 * convention.
 *
 * Returns std::nullopt if `vcardBytes` doesn't parse to at least one
 * Addressee, or if encoding to Palm bytes fails.
 */
std::optional<WildPalms::PalmSync::PalmRecord>
decodeVcardToPalm(const QByteArray &vcardBytes,
                  const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                  const QString &dbName);

} // namespace WildPalms::ContactsPlugin

#endif // WILDPALMS_CONTACTS_CONTACTSVCARDTRANSCODER_H
