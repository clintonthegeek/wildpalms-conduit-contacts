#ifndef WILDPALMS_CONTACTS_CONTACTSDOMAINEXTENSION_H
#define WILDPALMS_CONTACTS_CONTACTSDOMAINEXTENSION_H

#include <shapecontribution.h>

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::ContactsPlugin {

// O7: contributes the (contacts, palm) peer shape and palm<->vcard4 edges to
// the shape graph. The vcard4<->canon hop is libkalburator's (ContactsStockShapes).
// PluginManager registers this into the injected ShapeRegistries.
//
// Borrows a (nullable) CategoryMappingStore, threaded into the palm<->vcard4
// stages so the Palm category slot rides as the canonical `categories` field.
// The store is owned by the plugin and outlives this contribution.
class ContactsPalmShapes : public Kalburator::Shape::ShapeContribution {
public:
    explicit ContactsPalmShapes(const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr);

    Kalburator::Shape::DomainId targetDomain() const override;
    QList<std::pair<Kalburator::Shape::Shape, Kalburator::Shape::PropertyCatalogue>>
        peerShapes() const override;
    QList<Kalburator::Shape::TransformationEdge> edges() const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

} // namespace WildPalms::ContactsPlugin

#endif // WILDPALMS_CONTACTS_CONTACTSDOMAINEXTENSION_H
