#ifndef WILDPALMS_CONTACTS_CONTACTSBACKENDPLUGIN_H
#define WILDPALMS_CONTACTS_CONTACTSBACKENDPLUGIN_H

#include <memory>

#include "plugin.h"

namespace Kalburator::Conflict { struct RecordSnapshot; class ConflictHandler; }
namespace Kalburator::Sync { class SyncBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::Runtime { class PalmDeviceAccess; }

namespace WildPalms::ContactsPlugin {

/**
 * @brief Contacts plugin (K.8b): inherits Kalburator::Plugin (K.7 surface).
 *
 * No longer a KCoreAddons MODULE plugin. Linked STATIC and loaded
 * in-process by PalmRuntime::registerPalmPlugins() (Task 6).
 *
 * Provides:
 *   - PalmContactsBackend via createPalmBackend() — called directly
 *     by PalmRuntime; not routed through BackendContributions.
 *   - ContactsConflictHandler (contacts-aware overlays + Palm
 *     delegation).
 *   - No main view (legacy ContactView stays on legacy conduit until E.16).
 */
class ContactsBackendPlugin : public Kalburator::Plugin
{
public:
    ContactsBackendPlugin();
    ~ContactsBackendPlugin() override;

    // Kalburator::Plugin — all return {} (Palm plugins don't contribute
    // to the libkalburator BackendContribution system)
    QList<std::shared_ptr<Kalburator::Sync::BackendContribution>>
        backendContributions() const override { return {}; }

    // Plugin identity
    QString     pluginId()         const { return QStringLiteral("contacts"); }
    QString     displayName()      const;
    QIcon       icon()             const;
    QString     description()      const;
    QString     version()          const;
    QStringList claimedDatabases() const { return {QStringLiteral("AddressDB")}; }

    // Palm backend — called directly by PalmRuntime (Task 6)
    std::unique_ptr<Kalburator::Sync::SyncBackend>
        createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device);

    // Conflict handler
    Kalburator::Conflict::ConflictHandler *createConflictHandler();

    // No main view for contacts (legacy ContactView stays on legacy conduit until E.16)
    bool hasMainView() const { return false; }

    // Conflict presentation (called by conflict UI layer)
    void    enrichConflictSnapshot(
        Kalburator::Conflict::RecordSnapshot &snapshot,
        bool isSourceSide) const;
    QString formatConflictRecordHtml(
        const Kalburator::Conflict::RecordSnapshot &snapshot) const;

private:
    std::unique_ptr<WildPalms::PalmCalendar::CategoryMappingStore> m_categoryStore;
    std::unique_ptr<WildPalms::PalmConflict::PalmBackendConfig>    m_palmConfig;
    std::unique_ptr<WildPalms::PalmSync::PalmBackend>              m_palmBackend;
    WildPalms::Runtime::PalmDeviceAccess *m_device = nullptr; // borrowed; cached for createConflictHandler
};

} // namespace WildPalms::ContactsPlugin

#endif // WILDPALMS_CONTACTS_CONTACTSBACKENDPLUGIN_H
