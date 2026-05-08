#ifndef WILDPALMS_CONTACTS_CONTACTSDOMAINEXTENSION_H
#define WILDPALMS_CONTACTS_CONTACTSDOMAINEXTENSION_H

namespace Kalburator::Shape { class TransformationRegistry; }

namespace WildPalms::ContactsPlugin {

/// Registers the (contacts, palm) peer shape and palm <-> vcard4
/// transformation edges with the process-wide TransformationRegistry.
///
/// Idempotent. Safe to call before or after KalburatorDomainContacts'
/// own registerEdges() runs, as long as the contacts domain hasn't
/// been frozen by a prior compile() call.
class ContactsDomainExtension {
public:
    static void registerWith(Kalburator::Shape::TransformationRegistry& registry);
};

} // namespace WildPalms::ContactsPlugin

#endif
