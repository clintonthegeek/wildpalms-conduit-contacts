#include "contactsbackendplugin.h"

#include "contactsblobbackend.h"
#include "contactsconflicthandler.h"
#include "contactsvcardtranscoder.h"

#include "palm/calendar/categoryappinforeader.h"
#include "palm/calendar/categorymappingstore.h"
#include "palm/conflict/palmbackendconfig.h"
#include "palm/sync/palmbackend.h"
#include "runtime/palmdeviceaccess.h"

#include "conflictrecord.h"

#include <KContacts/Addressee>
#include <KContacts/VCardConverter>

#include <QIcon>
#include <QLoggingCategory>
#include <QString>

namespace {
Q_LOGGING_CATEGORY(WP_CONTACTS_PLUGIN, "wildpalms.contacts.plugin")
}

namespace WildPalms::ContactsPlugin {

ContactsBackendPlugin::ContactsBackendPlugin(QObject *parent)
    : QObject(parent)
    , m_categoryStore(std::make_unique<WildPalms::PalmCalendar::CategoryMappingStore>())
    , m_palmConfig(std::make_unique<WildPalms::PalmConflict::PalmBackendConfig>())
{
}

ContactsBackendPlugin::~ContactsBackendPlugin() = default;

QString ContactsBackendPlugin::pluginId()    const { return QStringLiteral("contacts"); }
QString ContactsBackendPlugin::displayName() const { return QStringLiteral("Contacts"); }
QIcon   ContactsBackendPlugin::icon()        const
{
    return QIcon::fromTheme(QStringLiteral("view-pim-contacts"));
}
QString ContactsBackendPlugin::description() const
{
    return QStringLiteral(
        "Synchronizes Palm AddressDB with vCard 4.0 files via virtual category sub-collections");
}
QString ContactsBackendPlugin::version()     const { return QStringLiteral("2.0"); }

QStringList ContactsBackendPlugin::claimedDatabases() const
{
    return { QStringLiteral("AddressDB") };
}

std::unique_ptr<Kalburator::Sync::IBlobBackend>
ContactsBackendPlugin::createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device)
{
    if (!device) return nullptr;

    m_device = device;
    m_palmBackend = std::make_unique<WildPalms::PalmSync::PalmBackend>(device);

    WildPalms::PalmCalendar::populateFromAppInfo(
        *m_categoryStore,
        QStringLiteral("AddressDB"),
        m_palmBackend->readAppBlock(QStringLiteral("AddressDB")));

    return std::make_unique<ContactsBlobBackend>(m_palmBackend.get(), m_categoryStore.get());
}

Kalburator::Sync::QSyncCore::ConflictHandler *
ContactsBackendPlugin::createConflictHandler()
{
    if (!m_device) {
        qCWarning(WP_CONTACTS_PLUGIN)
            << "createConflictHandler called before createPalmBackend — "
               "runtime must invoke createPalmBackend first to wire the device.";
        return nullptr;
    }
    // PalmDeviceAccess IS-A IPalmDatabaseAccess; no cast needed.
    return new ContactsConflictHandler(m_device, m_palmConfig.get());
}

void ContactsBackendPlugin::enrichConflictSnapshot(
    Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
    bool /*isSourceSide*/) const
{
    if (snapshot.content.isEmpty()) return;

    KContacts::VCardConverter conv;
    auto list = conv.parseVCards(snapshot.content);
    if (list.isEmpty()) return;

    const KContacts::Addressee &a = list.first();
    if (a.isEmpty()) return;

    const QString fn = a.formattedName();
    const QString name = !fn.isEmpty() ? fn : a.realName();
    snapshot.metadata[QStringLiteral("title")]   = name;
    snapshot.metadata[QStringLiteral("company")] = a.organization();
    snapshot.contentType = QStringLiteral("text/vcard");
}

QString ContactsBackendPlugin::formatConflictRecordHtml(
    const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const
{
    QString html;
    const QString title   = snapshot.metadata.value(QStringLiteral("title")).toString();
    const QString company = snapshot.metadata.value(QStringLiteral("company")).toString();
    if (!title.isEmpty()) {
        html += QStringLiteral("<h3>%1</h3>").arg(title.toHtmlEscaped());
    }
    if (!company.isEmpty()) {
        html += QStringLiteral("<p><b>Company:</b> %1</p>").arg(company.toHtmlEscaped());
    }
    html += QStringLiteral("<pre>%1</pre>")
        .arg(QString::fromUtf8(snapshot.content).toHtmlEscaped());
    return html;
}

} // namespace WildPalms::ContactsPlugin

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(ContactsBackendPluginFactory,
                           "contacts-backend-plugin.json",
                           registerPlugin<WildPalms::ContactsPlugin::ContactsBackendPlugin>();)

#include "contactsbackendplugin.moc"
