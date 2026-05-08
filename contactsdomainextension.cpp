#include "contactsdomainextension.h"

#include "palmtovcardtransformation.h"
#include "propertycatalogue.h"
#include "transformationregistry.h"

using namespace Kalburator::Shape;

namespace WildPalms::ContactsPlugin {

namespace {

PropertyCatalogue makePalmCatalogue()
{
    PropertyCatalogue cat;
    // Palm AddressDB native fields. These are the names we use to
    // describe what a palm-shape record carries; exact list mirrors
    // PalmRecord's field set. Loss-profile UI will show these.
    cat.addProperty({ PropertyId{"name"},     PropertyKind::Json,   QStringLiteral("Name") });
    cat.addProperty({ PropertyId{"company"},  PropertyKind::String, QStringLiteral("Company") });
    cat.addProperty({ PropertyId{"phones"},   PropertyKind::Json,   QStringLiteral("Phones") });
    cat.addProperty({ PropertyId{"address"},  PropertyKind::Json,   QStringLiteral("Address") });
    cat.addProperty({ PropertyId{"note"},     PropertyKind::String, QStringLiteral("Note") });
    cat.addProperty({ PropertyId{"category"}, PropertyKind::Integer,QStringLiteral("Category Slot") });
    return cat;
}

} // namespace

void ContactsDomainExtension::registerWith(TransformationRegistry &registry)
{
    const Shape palm     { DomainId{"contacts"}, EncodingId{"palm"}   };
    const Shape canonical{ DomainId{"contacts"}, EncodingId{"vcard4"} };

    // registerShape is idempotent; safe to re-call.
    registry.registerShape(palm, makePalmCatalogue());

    // palm -> vcard4 (lossless under X-WP-PALM-* extension stamping)
    registry.registerEdge(TransformationEdge{
        palm, canonical,
        palmToVCardLoss(),
        std::make_shared<PalmToVCardStage>()
    });

    // vcard4 -> palm (lossy: most v4 properties don't fit Palm AddressDB)
    registry.registerEdge(TransformationEdge{
        canonical, palm,
        vcardToPalmLoss(),
        std::make_shared<VCardToPalmStage>()
    });
}

} // namespace WildPalms::ContactsPlugin
