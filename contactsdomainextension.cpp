#include "contactsdomainextension.h"

#include "palmtovcardtransformation.h"
#include <propertycatalogue.h>

using namespace Kalburator::Shape;

namespace WildPalms::ContactsPlugin {

namespace {

PropertyCatalogue makePalmCatalogue()
{
    PropertyCatalogue cat;
    // Palm AddressDB native fields. These describe what a palm-shape record
    // carries; the loss-profile UI shows them.
    cat.addProperty({ PropertyId{"name"},     PropertyKind::Json,    QStringLiteral("Name") });
    cat.addProperty({ PropertyId{"company"},  PropertyKind::String,  QStringLiteral("Company") });
    cat.addProperty({ PropertyId{"phones"},   PropertyKind::Json,    QStringLiteral("Phones") });
    cat.addProperty({ PropertyId{"address"},  PropertyKind::Json,    QStringLiteral("Address") });
    cat.addProperty({ PropertyId{"note"},     PropertyKind::String,  QStringLiteral("Note") });
    cat.addProperty({ PropertyId{"category"}, PropertyKind::Integer, QStringLiteral("Category Slot") });
    return cat;
}

} // namespace

ContactsPalmShapes::ContactsPalmShapes(
    const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

DomainId ContactsPalmShapes::targetDomain() const
{
    return DomainId{QStringLiteral("contacts")};
}

QList<std::pair<Shape, PropertyCatalogue>> ContactsPalmShapes::peerShapes() const
{
    const Shape palm{ DomainId{"contacts"}, EncodingId{"palm"} };
    return { { palm, makePalmCatalogue() } };
}

QList<TransformationEdge> ContactsPalmShapes::edges() const
{
    const Shape palm     { DomainId{"contacts"}, EncodingId{"palm"}   };
    const Shape canonical{ DomainId{"contacts"}, EncodingId{"vcard4"} };
    // palm -> vcard4 (lossless under X-WP-PALM-* extension stamping).
    // vcard4 -> palm (lossy: most v4 properties don't fit Palm AddressDB).
    // The vcard4 endpoint is registered by libkalburator's ContactsStockShapes,
    // which loads earlier in the same PluginManager batch.
    return {
        TransformationEdge{ palm, canonical, palmToVCardLoss(), std::make_shared<PalmToVCardStage>(m_cats) },
        TransformationEdge{ canonical, palm, vcardToPalmLoss(), std::make_shared<VCardToPalmStage>(m_cats) },
    };
}

} // namespace WildPalms::ContactsPlugin
