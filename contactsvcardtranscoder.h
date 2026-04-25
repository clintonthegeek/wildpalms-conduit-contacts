#ifndef WILDPALMS_CONTACTS_CONTACTSVCARDTRANSCODER_H
#define WILDPALMS_CONTACTS_CONTACTSVCARDTRANSCODER_H

#include <optional>

#include <QByteArray>

#include "palm/sync/palmrecord.h"

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
 * Returns empty QByteArray on decode failure or empty input.
 */
QByteArray encodePalmToVcard(const WildPalms::PalmSync::PalmRecord &record);

/**
 * @brief Decode vCard bytes (v3.0 or v4.0) into a PalmRecord.
 *
 * `slotHint` populates `PalmRecord::category` (overriding any
 * X-WP-PALM-CATEGORY-SLOT in the body — collection-id wins). The
 * X-WP-PALM-RECORDID property, if present, populates
 * `PalmRecord::recordId`; otherwise recordId stays 0 and the device
 * assigns on write. The KContacts::Addressee's `secrecy()` is mapped to
 * PalmRecord::AttrSecret via `fromAddressee`'s convention.
 *
 * Returns std::nullopt if `vcardBytes` doesn't parse to at least one
 * Addressee, or if encoding to Palm bytes fails.
 */
std::optional<WildPalms::PalmSync::PalmRecord>
decodeVcardToPalm(const QByteArray &vcardBytes, int slotHint);

} // namespace WildPalms::ContactsPlugin

#endif // WILDPALMS_CONTACTS_CONTACTSVCARDTRANSCODER_H
