#include "palmtovcardtransformation.h"

#include "contactsvcardtranscoder.h"
#include "palm/sync/palmrecord.h"

using namespace Kalburator::Shape;

namespace WildPalms::ContactsPlugin {

PalmToVCardStage::PalmToVCardStage(
    const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

QByteArray PalmToVCardStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty())
        return {};
    const auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(sourceBytes);
    // Single-DB plugin: AddressDB is always the relevant database.
    return encodePalmToVcard(pr, m_cats, QStringLiteral("AddressDB"));
}

VCardToPalmStage::VCardToPalmStage(
    const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

QByteArray VCardToPalmStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty())
        return {};
    // The category slot is derived from the vCard CATEGORIES property via
    // the borrowed CategoryMappingStore (name -> slot). With no store / no
    // categories the slot is 0 (Unfiled).
    // Single-DB plugin: AddressDB is always the relevant database.
    const auto prOpt = decodeVcardToPalm(sourceBytes, m_cats, QStringLiteral("AddressDB"));
    if (!prOpt)
        return {};
    return prOpt->toWireBytes();
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
