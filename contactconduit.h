#ifndef CONTACTCONDUIT_H
#define CONTACTCONDUIT_H

#include "sync/conduit.h"

namespace Sync {

/**
 * @brief Conduit for Palm Contacts <-> vCard files
 *
 * Syncs:
 *   - Palm AddressDB (binary format)
 *   - Local .vcf files (vCard 4.0 format)
 *
 * Uses ContactMapper for format conversion.
 * Supports bidirectional category sync.
 */
class ContactConduit : public SyncConduitBase
{
    Q_OBJECT

public:
    explicit ContactConduit(QObject *parent = nullptr);

    // ========== Conduit Identity ==========

    QString conduitId() const override { return "contacts"; }
    QString displayName() const override { return "Contacts"; }
    QString palmDatabaseName() const override { return "AddressDB"; }
    QString fileExtension() const override { return ".vcf"; }

    // ========== UI Contribution ==========
    QIcon icon() const override {
        return QIcon::fromTheme(QStringLiteral("view-pim-contacts"));
    }
    QString description() const override {
        return QStringLiteral("Synchronizes Palm AddressDB with vCard files");
    }
    bool hasView() const override { return true; }
    QWidget *createView(QWidget *parent) override;
    QString viewName() const override { return QStringLiteral("Contacts"); }
    QIcon viewIcon() const override {
        return QIcon::fromTheme(QStringLiteral("view-pim-contacts"));
    }

    // ========== Record Conversion ==========

    BackendRecord* palmToBackend(PilotRecord *palmRecord,
                                  SyncContext *context) override;

    PilotRecord* backendToPalm(BackendRecord *backendRecord,
                                SyncContext *context) override;

    bool recordsEqual(PilotRecord *palm, BackendRecord *backend) const override;

    QString palmRecordDescription(PilotRecord *record) const override;

    // ========== Conflict Display ==========

    void enrichConflictSnapshot(QSyncCore::RecordSnapshot &snapshot,
                                 bool isSourceSide) const override;
    QString formatConflictRecordHtml(const QSyncCore::RecordSnapshot &snapshot) const override;
};

} // namespace Sync

#endif // CONTACTCONDUIT_H
