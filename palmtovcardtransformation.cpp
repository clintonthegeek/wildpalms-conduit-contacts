#include "palmtovcardtransformation.h"

#include "contactsvcardtranscoder.h"
#include "palm/sync/palmrecord.h"

using namespace Kalburator::Shape;

namespace WildPalms::ContactsPlugin {

namespace {

// Source bytes for PalmToVCardStage are the wire serialization of a
// PalmRecord — the exact format established by ContactsBlobBackend in
// Task 14 / 16 / 17. PalmRecord::toWireBytes / fromWireBytes (Phase Ia
// Task 11) are the round-trip primitive.
WildPalms::PalmSync::PalmRecord palmRecordFromBytes(const QByteArray &bytes)
{
    return WildPalms::PalmSync::PalmRecord::fromWireBytes(bytes);
}

QByteArray palmRecordToBytes(const WildPalms::PalmSync::PalmRecord &r)
{
    return r.toWireBytes();
}

} // namespace

QByteArray PalmToVCardStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty())
        return {};
    const auto pr = palmRecordFromBytes(sourceBytes);
    return encodePalmToVcard(pr);
}

QByteArray VCardToPalmStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty())
        return {};
    // slotHint = -1 means "use whatever's in X-WP-PALM-CATEGORY-SLOT
    // on the vCard". This is the round-trip discipline; the backend
    // remaps slots authoritatively on write.
    const auto prOpt = decodeVcardToPalm(sourceBytes, /*slotHint*/ -1);
    if (!prOpt)
        return {};
    return palmRecordToBytes(*prOpt);
}

LossProfile vcardToPalmLoss()
{
    LossProfile p;
    // Palm AddressDB has no slot for these vCard 4.0 properties — genuinely dropped.
    // (Richer Reversible/providerExtras treatment is Phase 2, not track-to-green.)
    p.affected.insert(PropertyId{QStringLiteral("photo")},       LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("anniversary")}, LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("kind")},        LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("member")},      LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("lang")},        LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("gender")},      LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("related")},     LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("geo")},         LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("tz")},          LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("x-custom")},    LossKind::Dropped);
    return p;
}

LossProfile palmToVCardLoss()
{
    return {};  // lossless: palm -> vcard4 preserves everything via X-WP-PALM-* stamps
}

} // namespace WildPalms::ContactsPlugin
