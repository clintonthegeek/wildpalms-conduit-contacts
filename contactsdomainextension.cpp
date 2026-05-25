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

    // K.7: DomainRegistry::initialize() was removed; stock plugin shapes
    // are now registered by Kalburator::registerStockPlugins() at
    // PluginManager load time. The defensive fallback below handles the
    // case where registerStockPlugins() hasn't run in this address space
    // (e.g. in WildPalms unit tests that don't go through the full
    // plugin init path).
    //
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

    // PHASE 2 (verify-and-lock, 2026-05-24): contacts rides the shape graph via
    // palm -> vcard4 -> canon. We register only the palm <-> vcard4 edges; the
    // vcard4 <-> canon hop is libkalburator's (ContactsStockShapes). Palm identity
    // (RECORDID + secret bit) survives the canon round-trip because libkalburator's
    // VCard4ToCanonStage collects all KContacts customs into providerExtras["x-vcard"]
    // and CanonToVCard4Stage re-emits them. The CATEGORY-SLOT custom rides along but
    // is NOT restored by the demote (slotHint=-1); the backend remaps slots on write.
    // Verified by tests/plugins/contacts/tst_contacts_canon_roundtrip.cpp.

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
