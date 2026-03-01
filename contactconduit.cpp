#include "contactconduit.h"
#include "contactview.h"
#include "contactmapper.h"
#include "palm/pilotrecord.h"
#include "palm/categoryinfo.h"
#include "sync/localfilebackend.h"
#include "sync/qsynccore/conflictrecord.h"

#include <QDebug>

namespace Sync {

ContactConduit::ContactConduit(QObject *parent)
    : SyncConduitBase(parent)
{
}

BackendRecord* ContactConduit::palmToBackend(PilotRecord *palmRecord,
                                              SyncContext *context)
{
    if (!palmRecord) return nullptr;

    // Unpack Palm contact
    ContactMapper::Contact contact = ContactMapper::unpackContact(palmRecord);

    // Convert to vCard
    QString catName = categoryName(contact.category);
    QString vcard = ContactMapper::contactToVCard(contact, catName);

    // Create backend record
    BackendRecord *record = new BackendRecord();
    record->data = vcard.toUtf8();
    record->type = "contact";
    record->contentHash = LocalFileBackend::calculateHash(record->data);
    record->lastModified = QDateTime::currentDateTime();

    // Set display name from contact name
    QStringList parts;
    if (!contact.firstName.isEmpty()) parts << contact.firstName;
    if (!contact.lastName.isEmpty()) parts << contact.lastName;
    QString name = parts.join(" ");
    if (name.isEmpty() && !contact.company.isEmpty()) {
        name = contact.company;
    }
    if (name.isEmpty()) {
        name = contact.phone1;
    }
    record->displayName = name;

    return record;
}

PilotRecord* ContactConduit::backendToPalm(BackendRecord *backendRecord,
                                            SyncContext *context)
{
    if (!backendRecord) return nullptr;

    // Parse vCard content
    QString content = QString::fromUtf8(backendRecord->data);
    ContactMapper::Contact contact = ContactMapper::vCardToContact(content);

    // Look up or create category from name
    if (!contact.categoryName.isEmpty() && m_categories) {
        contact.category = m_categories->getOrCreateCategory(contact.categoryName);
        qDebug() << "[ContactConduit] Category" << contact.categoryName << "-> index" << contact.category;
    }

    // Pack to Palm record
    PilotRecord *record = ContactMapper::packContact(contact);

    return record;
}

bool ContactConduit::recordsEqual(PilotRecord *palm, BackendRecord *backend) const
{
    if (!palm || !backend) return false;

    // Unpack Palm contact
    ContactMapper::Contact palmContact = ContactMapper::unpackContact(palm);

    // Parse backend content
    QString backendContent = QString::fromUtf8(backend->data);
    ContactMapper::Contact backendContact = ContactMapper::vCardToContact(backendContent);

    // Compare key fields
    if (palmContact.firstName != backendContact.firstName) return false;
    if (palmContact.lastName != backendContact.lastName) return false;
    if (palmContact.company != backendContact.company) return false;

    // Compare phone numbers (at least first one)
    if (palmContact.phone1 != backendContact.phone1) return false;

    // Compare categories
    QString palmCategoryName = categoryName(palmContact.category);

    // Normalize: "Unfiled" (index 0) and empty string are equivalent
    QString normalizedPalmCat = palmCategoryName;
    QString normalizedBackendCat = backendContact.categoryName;

    if (normalizedPalmCat.compare("Unfiled", Qt::CaseInsensitive) == 0) {
        normalizedPalmCat.clear();
    }
    if (normalizedBackendCat.compare("Unfiled", Qt::CaseInsensitive) == 0) {
        normalizedBackendCat.clear();
    }

    if (normalizedPalmCat.compare(normalizedBackendCat, Qt::CaseInsensitive) != 0) {
        return false;
    }

    return true;
}

QString ContactConduit::palmRecordDescription(PilotRecord *record) const
{
    if (!record) return QString();

    ContactMapper::Contact contact = ContactMapper::unpackContact(record);

    // Build display name
    QStringList parts;
    if (!contact.firstName.isEmpty()) parts << contact.firstName;
    if (!contact.lastName.isEmpty()) parts << contact.lastName;

    QString name = parts.join(" ");
    if (name.isEmpty() && !contact.company.isEmpty()) {
        name = contact.company;
    }
    if (name.isEmpty()) {
        name = contact.phone1;  // Fallback to phone
    }
    if (name.isEmpty()) {
        name = "<Unnamed>";
    }

    return name;
}

void ContactConduit::enrichConflictSnapshot(QSyncCore::RecordSnapshot &snapshot,
                                              bool isSourceSide) const
{
    if (snapshot.content.isEmpty()) return;

    ContactMapper::Contact contact;

    if (isSourceSide) {
        // Source: Palm binary — unpack via mapper, convert to vCard text
        PilotRecord tempRecord(0, 0, 0, snapshot.content);
        contact = ContactMapper::unpackContact(&tempRecord);
        QString catName = categoryName(contact.category);
        snapshot.content = ContactMapper::contactToVCard(contact, catName).toUtf8();
    } else {
        // Target: already vCard text — parse for metadata
        contact = ContactMapper::vCardToContact(QString::fromUtf8(snapshot.content));
    }

    // Populate metadata
    QStringList nameParts;
    if (!contact.firstName.isEmpty()) nameParts << contact.firstName;
    if (!contact.lastName.isEmpty()) nameParts << contact.lastName;
    QString name = nameParts.join(QStringLiteral(" "));
    if (name.isEmpty()) name = contact.company;

    if (!name.isEmpty())
        snapshot.metadata[QStringLiteral("name")] = name;
    if (!contact.phone1.isEmpty())
        snapshot.metadata[QStringLiteral("phone")] = contact.phone1;
    if (!contact.custom1.isEmpty() && contact.custom1.contains('@'))
        snapshot.metadata[QStringLiteral("email")] = contact.custom1;
    if (!contact.company.isEmpty())
        snapshot.metadata[QStringLiteral("company")] = contact.company;
    if (!contact.title.isEmpty())
        snapshot.metadata[QStringLiteral("title")] = contact.title;

    QStringList addrParts;
    if (!contact.address.isEmpty()) addrParts << contact.address;
    if (!contact.city.isEmpty()) addrParts << contact.city;
    if (!contact.state.isEmpty()) addrParts << contact.state;
    if (!contact.zip.isEmpty()) addrParts << contact.zip;
    if (!contact.country.isEmpty()) addrParts << contact.country;
    if (!addrParts.isEmpty())
        snapshot.metadata[QStringLiteral("address")] = addrParts.join(QStringLiteral(", "));

    snapshot.contentType = QStringLiteral("text/vcard");
}

QString ContactConduit::formatConflictRecordHtml(const QSyncCore::RecordSnapshot &snapshot) const
{
    QString html;
    const QVariantMap &m = snapshot.metadata;

    QString name = m.value(QStringLiteral("name")).toString();
    if (!name.isEmpty())
        html += QStringLiteral("<h3>%1</h3>").arg(name.toHtmlEscaped());

    html += QStringLiteral("<table cellpadding='4'>");

    auto addRow = [&html](const QString &label, const QString &value) {
        if (!value.isEmpty())
            html += QStringLiteral("<tr><td><b>%1:</b></td><td>%2</td></tr>")
                .arg(label.toHtmlEscaped(), value.toHtmlEscaped());
    };

    addRow(QStringLiteral("Phone"), m.value(QStringLiteral("phone")).toString());
    addRow(QStringLiteral("Email"), m.value(QStringLiteral("email")).toString());
    addRow(QStringLiteral("Company"), m.value(QStringLiteral("company")).toString());
    addRow(QStringLiteral("Title"), m.value(QStringLiteral("title")).toString());
    addRow(QStringLiteral("Address"), m.value(QStringLiteral("address")).toString());

    html += QStringLiteral("</table>");

    // Also show raw vCard for full details
    QString content = QString::fromUtf8(snapshot.content);
    html += QStringLiteral("<hr><details><summary>Full vCard</summary><pre>%1</pre></details>")
        .arg(content.toHtmlEscaped());

    return html;
}

QWidget *ContactConduit::createView(QWidget *parent)
{
    return new ContactView(parent);
}

} // namespace Sync

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(ContactConduitFactory, "contacts-conduit.json",
                           registerPlugin<Sync::ContactConduit>();)

#include "contactconduit.moc"
