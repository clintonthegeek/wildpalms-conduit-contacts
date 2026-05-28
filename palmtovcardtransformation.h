#ifndef WILDPALMS_CONTACTS_PALMTOVCARDTRANSFORMATION_H
#define WILDPALMS_CONTACTS_PALMTOVCARDTRANSFORMATION_H

#include "transformationedge.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::ContactsPlugin {

/// (contacts, palm) → (contacts, vcard4)
/// Body delegates to encodePalmToVcard. Decodes the source bytes as a
/// PalmRecord first. Borrows a (nullable) CategoryMappingStore to carry
/// the Palm category slot into the vCard CATEGORIES property.
class PalmToVCardStage : public Kalburator::Shape::TransformationStage {
public:
    explicit PalmToVCardStage(const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr);
    QByteArray transform(const QByteArray &sourceBytes) const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

/// (contacts, vcard4) → (contacts, palm)
/// Body delegates to decodeVcardToPalm. Encodes the resulting PalmRecord
/// back to wire bytes for the backend. Borrows a (nullable) CategoryMappingStore
/// to map the vCard CATEGORIES name back to a Palm category slot.
class VCardToPalmStage : public Kalburator::Shape::TransformationStage {
public:
    explicit VCardToPalmStage(const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr);
    QByteArray transform(const QByteArray &sourceBytes) const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

/// vcard4 → palm: lossy (Palm AddressDB has fixed fields; most of v4
/// doesn't fit). Exact dropped set deferred to test-driven refinement.
Kalburator::Shape::LossProfile vcardToPalmLoss();

/// palm → vcard4: nominally lossless. Palm's representation is a strict
/// subset of vCard 4 under the X-WP-PALM-* extension stamps used for
/// recordId / category-slot round-trip. Profile is Lossless unless the
/// stage adds extension parameters that v3 might lose later.
Kalburator::Shape::LossProfile palmToVCardLoss();

} // namespace WildPalms::ContactsPlugin

#endif // WILDPALMS_CONTACTS_PALMTOVCARDTRANSFORMATION_H
