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

    // Kalburator::Plugin — no backend contributions (Palm backends are created
    // directly by PalmRuntime, not via the BackendContribution system).
    QList<std::shared_ptr<Kalburator::Sync::BackendContribution>>
        backendContributions() const override { return {}; }

    // O7: contribute the (contacts, palm) peer shape + palm<->vcard4 edges via
    // the shape-graph contribution system (PluginManager registers them into
    // the injected ShapeRegistries). Replaces the old ctor-time registerWith().
    QList<std::shared_ptr<Kalburator::Shape::ShapeContribution>>
        shapeContributions() const override;

    // Plugin identity
    QString     pluginId()         const { return QStringLiteral("contacts"); }
    QString     displayName()      const;
    QIcon       icon()             const;
    QString     description()      const;
    QString     version()          const;
    QStringList claimedDatabases() const { return {QStringLiteral("AddressDB")}; }

    // F.3: Category slot snapshot — used by PalmRuntime::finishConnect to
    // write the snapshot into Profile after createPalmBackend populates
    // m_categoryStore from the live AppInfo block. Returns empty list if
    // the store hasn't been populated yet.
    QString     primaryDbName()       const { return QStringLiteral("AddressDB"); }
    QStringList categorySlotNames()   const;

    // Task 3: borrowed accessor for hub<->remote routing translation.
    WildPalms::PalmCalendar::CategoryMappingStore *categoryStore() const;

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
