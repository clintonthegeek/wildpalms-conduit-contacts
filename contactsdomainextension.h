#ifndef WILDPALMS_CONTACTS_CONTACTSDOMAINEXTENSION_H
#define WILDPALMS_CONTACTS_CONTACTSDOMAINEXTENSION_H

#include <shapecontribution.h>

namespace WildPalms::ContactsPlugin {

// O7: contributes the (contacts, palm) peer shape and palm<->vcard4 edges to
// the shape graph. The vcard4<->canon hop is libkalburator's (ContactsStockShapes).
// PluginManager registers this into the injected ShapeRegistries.
class ContactsPalmShapes : public Kalburator::Shape::ShapeContribution {
public:
    Kalburator::Shape::DomainId targetDomain() const override;
    QList<std::pair<Kalburator::Shape::Shape, Kalburator::Shape::PropertyCatalogue>>
        peerShapes() const override;
    QList<Kalburator::Shape::TransformationEdge> edges() const override;
};

} // namespace WildPalms::ContactsPlugin

#endif
