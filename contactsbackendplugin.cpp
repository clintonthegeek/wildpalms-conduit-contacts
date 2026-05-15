#include "contactsbackendplugin.h"

#include "palmcontactsbackend.h"
#include "contactsconflicthandler.h"
#include "contactsdomainextension.h"
#include "contactsvcardtranscoder.h"

#include "palm/calendar/categoryappinforeader.h"
#include "palm/calendar/categorymappingstore.h"
#include "palm/conflict/palmbackendconfig.h"
#include "palm/sync/palmbackend.h"
#include "runtime/palmdeviceaccess.h"

#include "conflictrecord.h"
#include "transformationregistry.h"

#include <KContacts/Addressee>
#include <KContacts/VCardConverter>

#include <QIcon>
#include <QLoggingCategory>
#include <QString>

namespace {
Q_LOGGING_CATEGORY(WP_CONTACTS_PLUGIN, "wildpalms.contacts.plugin")
}

namespace WildPalms::ContactsPlugin {

ContactsBackendPlugin::ContactsBackendPlugin()
    : m_categoryStore(std::make_unique<WildPalms::PalmCalendar::CategoryMappingStore>())
    , m_palmConfig(std::make_unique<WildPalms::PalmConflict::PalmBackendConfig>())
{
    // Phase Ia: register the (contacts, palm) peer shape and palm <-> vcard4
    // edges with the process-wide TransformationRegistry as soon as the
    // plugin is constructed. This is path (a) from Task 13 (plugin
    // self-registers its peer shape). Idempotent — TransformationRegistry's
    // registerEdge accepts identical re-registration without asserting,
    // so multiple plugin instances in the same process are safe.
    ContactsDomainExtension::registerWith(
        Kalburator::Shape::TransformationRegistry::instance());
}

ContactsBackendPlugin::~ContactsBackendPlugin() = default;

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

std::unique_ptr<Kalburator::Sync::SyncBackend>
ContactsBackendPlugin::createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device)
{
    if (!device) return nullptr;

    m_device = device;
    m_palmBackend = std::make_unique<WildPalms::PalmSync::PalmBackend>(device);

    WildPalms::PalmCalendar::populateFromAppInfo(
        *m_categoryStore,
        QStringLiteral("AddressDB"),
        m_palmBackend->readAppBlock(QStringLiteral("AddressDB")));

    return std::make_unique<PalmContactsBackend>(m_palmBackend.get(), m_categoryStore.get());
}

Kalburator::Conflict::ConflictHandler *
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
    Kalburator::Conflict::RecordSnapshot &snapshot,
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
    const Kalburator::Conflict::RecordSnapshot &snapshot) const
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
