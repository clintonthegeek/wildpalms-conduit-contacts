#ifndef WILDPALMS_CONTACTS_CONTACTSBACKENDPLUGIN_H
#define WILDPALMS_CONTACTS_CONTACTSBACKENDPLUGIN_H

#include <memory>

#include <QObject>

#include "core/ibackendplugin.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
class PalmDeviceConnection;

namespace WildPalms::ContactsPlugin {

/**
 * @brief Fourth new-ABI WildPalms plugin (Memo E.9, Calendar E.10, ToDo E.11).
 *
 * Provides:
 *   - ContactsBlobBackend wrapping the shared PalmBackend (one
 *     collection per populated category slot under "AddressDB").
 *   - No typed SyncBackend; libkalburator has no typed-contacts
 *     upper layer (extract-on-second-consumer per parent spec).
 *   - ContactsConflictHandler (multi-valued field-union overlay +
 *     Palm delegation).
 *
 * Owns the per-session CategoryMappingStore, populated from the
 * AddressDB AppInfo block at createBackends() time.
 *
 * Does NOT register a main-window view — legacy ContactView stays
 * attached to legacy ContactConduit until E.16's unified-runtime
 * cleanup.
 */
class ContactsBackendPlugin : public QObject, public WildPalms::IBackendPlugin
{
    Q_OBJECT
    Q_INTERFACES(WildPalms::IBackendPlugin)
public:
    explicit ContactsBackendPlugin(QObject *parent = nullptr);
    ~ContactsBackendPlugin() override;

    // IPlugin
    QString pluginId()    const override;
    QString displayName() const override;
    QIcon   icon()        const override;
    QString description() const override;
    QString version()     const override;

    // IBackendPlugin
    QStringList      claimedDatabases() const override;
    ProvidedBackends createBackends(Kalburator::Sync::ISyncHost *host,
                                    PalmDeviceConnection         *device) override;

    // IBackendPlugin — conflict handler
    Kalburator::Sync::QSyncCore::ConflictHandler *createConflictHandler() override;

    // IBackendPlugin — main view (none for E.12; legacy ContactView stays
    // attached to legacy ContactConduit until E.16). The IBackendPlugin
    // base provides default no-op implementations of createMainView /
    // mainViewName / mainViewIcon, so explicitly only override hasMainView.
    bool hasMainView() const override { return false; }

    // IBackendPlugin — conflict presentation
    void    enrichConflictSnapshot(
        Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
        bool isSourceSide) const override;
    QString formatConflictRecordHtml(
        const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const override;

private:
    std::unique_ptr<WildPalms::PalmCalendar::CategoryMappingStore> m_categoryStore;
    std::unique_ptr<WildPalms::PalmConflict::PalmBackendConfig>    m_palmConfig;
    PalmDeviceConnection *m_device = nullptr;   // borrowed; cached for createConflictHandler
};

} // namespace WildPalms::ContactsPlugin

#endif // WILDPALMS_CONTACTS_CONTACTSBACKENDPLUGIN_H
