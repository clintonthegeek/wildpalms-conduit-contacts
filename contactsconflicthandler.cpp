#include "contactsconflicthandler.h"

#include "contactsvcardtranscoder.h"

#include "palm/codecs/contactcodec.h"
#include "palm/conflict/palmconflicthandler.h"
#include "palm/conflict/palmbackendconfig.h"
#include "palm/sync/palmrecord.h"

#include "conflictrecord.h"

namespace WildPalms::ContactsPlugin {

namespace {

using WildPalms::PalmCodecs::Contact;
using WildPalms::PalmCodecs::decodeContact;
using WildPalms::PalmCodecs::encodeContact;
using WildPalms::PalmSync::PalmRecord;

struct DecodedSide {
    PalmRecord record;
    Contact    contact;
    bool       valid = false;
};

DecodedSide decodeSide(const QByteArray &vcardBytes)
{
    DecodedSide out;
    auto pr = WildPalms::ContactsPlugin::decodeVcardToPalm(vcardBytes, /*slotHint*/ 0);
    if (!pr.has_value()) return out;
    auto c = decodeContact(QByteArrayView(pr->data));
    if (!c.has_value()) return out;
    out.record  = *pr;
    out.contact = *c;
    out.valid   = true;
    return out;
}

bool singleValuedFieldsAgree(const Contact &a, const Contact &b,
                             bool aSecret, bool bSecret)
{
    return a.lastName  == b.lastName
        && a.firstName == b.firstName
        && a.company   == b.company
        && a.title     == b.title
        && a.address   == b.address
        && a.city      == b.city
        && a.state     == b.state
        && a.zip       == b.zip
        && a.country   == b.country
        && a.note      == b.note
        && a.showPhone == b.showPhone
        && aSecret     == bSecret;
}

bool slotsAreNonConflicting(const Contact &a, const Contact &b)
{
    for (int i = 0; i < 5; ++i) {
        if (!a.phone[i].isEmpty() && !b.phone[i].isEmpty() && a.phone[i] != b.phone[i]) {
            return false;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (!a.custom[i].isEmpty() && !b.custom[i].isEmpty() && a.custom[i] != b.custom[i]) {
            return false;
        }
    }
    return true;
}

Contact unionMerge(const Contact &a, const Contact &b)
{
    Contact merged = a;

    QStringList mergedLabels;
    for (int i = 0; i < 5; ++i) {
        QString chosen;
        QString chosenLabel;
        if (!a.phone[i].isEmpty()) {
            chosen = a.phone[i];
            int aIndex = 0;
            for (int j = 0; j < i; ++j) if (!a.phone[j].isEmpty()) ++aIndex;
            if (aIndex < a.phoneLabels.size()) chosenLabel = a.phoneLabels[aIndex];
        } else if (!b.phone[i].isEmpty()) {
            chosen = b.phone[i];
            int bIndex = 0;
            for (int j = 0; j < i; ++j) if (!b.phone[j].isEmpty()) ++bIndex;
            if (bIndex < b.phoneLabels.size()) chosenLabel = b.phoneLabels[bIndex];
        }
        merged.phone[i] = chosen;
        if (!chosen.isEmpty()) {
            mergedLabels.append(chosenLabel.isEmpty()
                                ? QStringLiteral("Other")
                                : chosenLabel);
        }
    }
    merged.phoneLabels = mergedLabels;

    for (int i = 0; i < 4; ++i) {
        if (!a.custom[i].isEmpty()) {
            merged.custom[i] = a.custom[i];
        } else {
            merged.custom[i] = b.custom[i];
        }
    }
    return merged;
}

QByteArray buildMergedVcard(const PalmRecord &peer, const Contact &merged)
{
    PalmRecord pr = peer;
    pr.data = encodeContact(merged);
    return WildPalms::ContactsPlugin::encodePalmToVcard(pr);
}

} // namespace

ContactsConflictHandler::ContactsConflictHandler(
    WildPalms::PalmSync::IPalmDatabaseAccess *device,
    const WildPalms::PalmConflict::PalmBackendConfig *config)
    : m_palm(std::make_unique<WildPalms::PalmConflict::PalmConflictHandler>(device, config))
{
}

ContactsConflictHandler::~ContactsConflictHandler() = default;

Kalburator::Sync::QSyncCore::ConflictDecision ContactsConflictHandler::handleConflict(
    Kalburator::Sync::QSyncCore::ConflictRecord &conflict,
    const Kalburator::Sync::QSyncCore::ConflictPolicy &policy)
{
    DecodedSide source = decodeSide(conflict.source.content);
    DecodedSide target = decodeSide(conflict.target.content);

    if (!source.valid || !target.valid) {
        m_lastOverlay = QStringLiteral("delegated");
        return m_palm->handleConflict(conflict, policy);
    }

    const bool sourceSecret = source.record.isSecret();
    const bool targetSecret = target.record.isSecret();

    if (!singleValuedFieldsAgree(source.contact, target.contact,
                                 sourceSecret, targetSecret)
     || !slotsAreNonConflicting(source.contact, target.contact)) {
        m_lastOverlay = QStringLiteral("delegated");
        return m_palm->handleConflict(conflict, policy);
    }

    Contact merged = unionMerge(source.contact, target.contact);
    conflict.mergedContent = buildMergedVcard(source.record, merged);
    m_lastOverlay = QStringLiteral("field-union");
    return Kalburator::Sync::QSyncCore::ConflictDecision::Merge;
}

} // namespace WildPalms::ContactsPlugin
