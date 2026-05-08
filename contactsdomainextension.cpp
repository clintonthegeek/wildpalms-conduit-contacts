#include "contactsdomainextension.h"

#include "domainregistry.h"
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

    // Phase Ia Task 15: ensure libkalburator's stock domain plugins
    // (including the contacts plugin that owns vcard4) have populated
    // the TransformationRegistry before we register edges that reference
    // their shapes. DomainRegistry::initialize is idempotent (a no-op on
    // subsequent calls), so this is cheap and safe to call from the
    // ContactsBackendPlugin constructor regardless of process startup
    // order.
    DomainRegistry::instance().initialize(registry);

    // Defensive: if libkalburator's contacts domain plugin's
    // static-init registrar didn't run in this address space (e.g.
    // because contactsdomainplugin.cpp is in a static library that's
    // included via plain link rather than --whole-archive — the
    // wildpalms_contacts_v2.so plugin module hits this case), the
    // initialize() call above leaves vcard4 unregistered. We need
    // vcard4 in the registry before registerEdge for the palm <-> v4
    // edges below, otherwise Q_ASSERT_X("to-shape not registered")
    // fires. Register a minimal placeholder catalogue; libkalburator's
    // own registerEdges, when it eventually runs, replaces the
    // catalogue under the same shape key (registerShape is idempotent).
    if (registry.catalogueFor(canonical) == nullptr) {
        registry.registerShape(canonical, {});
    }

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
