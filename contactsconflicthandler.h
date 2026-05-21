#ifndef WILDPALMS_CONTACTS_CONTACTSCONFLICTHANDLER_H
#define WILDPALMS_CONTACTS_CONTACTSCONFLICTHANDLER_H

#include "conflictpolicy.h"   // brings in Kalburator::Conflict::ConflictHandler

#include <memory>

#include <QString>

namespace WildPalms::PalmSync { class IPalmDatabaseAccess; }
namespace WildPalms::PalmConflict {
class PalmConflictHandler;
struct PalmBackendConfig;
}

namespace WildPalms::ContactsPlugin {

/**
 * @brief ConflictHandler with one Contacts overlay, delegating to PalmConflictHandler.
 *
 * Resolution order:
 *   1. Decode both sides as PalmRecord -> Contact POD via the transcoder.
 *      If either side fails to decode -> delegate to PalmConflictHandler.
 *   2. If any single-valued field differs (lastName, firstName, company,
 *      title, address, city, state, zip, country, note, showPhone, or the
 *      record secret bit) -> delegate.
 *   3. If any multi-valued slot (phone[0..4], custom[0..3]) has different
 *      non-empty values on both sides -> delegate.
 *   4. Otherwise merge: per-slot union for the multi-valued fields,
 *      re-serialise to vCard, set mergedContent, return Merge.
 *
 * Owns its inner PalmConflictHandler.
 *
 * Lifetime: does NOT own device or config. Both must outlive the
 * handler.
 */
class ContactsConflictHandler : public Kalburator::Conflict::ConflictHandler
{
public:
    ContactsConflictHandler(WildPalms::PalmSync::IPalmDatabaseAccess *device,
                            const WildPalms::PalmConflict::PalmBackendConfig *config);
    ~ContactsConflictHandler() override;

    Kalburator::Conflict::ConflictDecision handleConflict(
        Kalburator::Conflict::ConflictRecord &conflict,
        const Kalburator::Conflict::ConflictPolicy &policy) override;

    bool canPrompt() const override { return false; }

    /// Test hook: which path was last taken.
    /// Values: "" (uninitialised), "field-union", "delegated".
    const QString &lastOverlay() const { return m_lastOverlay; }

private:
    std::unique_ptr<WildPalms::PalmConflict::PalmConflictHandler> m_palm;
    QString m_lastOverlay;
};

} // namespace WildPalms::ContactsPlugin

#endif // WILDPALMS_CONTACTS_CONTACTSCONFLICTHANDLER_H
