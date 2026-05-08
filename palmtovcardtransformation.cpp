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
    p.level = LossLevel::IntraDomainLossy;
    p.dropped.insert(PropertyId{QStringLiteral("photo")});
    p.dropped.insert(PropertyId{QStringLiteral("anniversary")});
    p.dropped.insert(PropertyId{QStringLiteral("kind")});
    p.dropped.insert(PropertyId{QStringLiteral("member")});
    p.dropped.insert(PropertyId{QStringLiteral("lang")});
    p.dropped.insert(PropertyId{QStringLiteral("gender")});
    p.dropped.insert(PropertyId{QStringLiteral("related")});
    p.dropped.insert(PropertyId{QStringLiteral("geo")});
    p.dropped.insert(PropertyId{QStringLiteral("tz")});
    p.dropped.insert(PropertyId{QStringLiteral("x-custom")});
    return p;
}

LossProfile palmToVCardLoss()
{
    LossProfile p;
    p.level = LossLevel::Lossless;
    return p;
}

} // namespace WildPalms::ContactsPlugin
