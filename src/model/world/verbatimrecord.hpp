#ifndef VERBATIMRECORD_HPP
#define VERBATIMRECORD_HPP

// Verbatim round-trip for untouched records.
//
// Some records cannot be parsed losslessly yet (compressed NPC_ records,
// quest stage/objective interleavings, localized-width words). Rather than
// trusting the structured parse for those, opt-in record structs keep the
// exact on-disk payload plus a snapshot of the parsed state taken at load.
// On save, when the current struct still equals its load snapshot (and its
// components do too), the original bytes — including the original header
// flags such as the compressed bit — are re-emitted untouched. Any user
// edit trips the comparison and falls back to the normal structured save,
// so editing behavior is unchanged.
//
// Opt-in members a record struct declares:
//   QByteArray verbatimBody;                    // exact payload after the 24B header
//   quint32 verbatimFlags = 0;                  // original RecHeader.flags
//   std::shared_ptr<T> verbatimSnapshot;        // struct state right after load

#include "record.hpp"

#include "../../../libs/components/formcomponents.hpp"
#include "../../../libs/files/esm/common.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "../../../libs/files/esm/records.hpp"

#include <memory>
#include <type_traits>

template<typename T, typename = void>
struct HasVerbatimRecord : std::false_type {};

template<typename T>
struct HasVerbatimRecord<T, std::void_t<decltype(std::declval<T>().verbatimBody)>> : std::true_type {};

template<typename T, typename = void>
struct HasVerbatimComponents : std::false_type {};

template<typename T>
struct HasVerbatimComponents<T, std::void_t<decltype(std::declval<T>().components)>> : std::true_type {};

// Emits the record's original bytes when it is still identical to its load
// state; returns true when it emitted, false to fall back to structured save.
template<typename ESXRecord>
bool tryWriteVerbatimRecord(ESMWriter& writer, NAME tag, const Record<ESXRecord>& rec)
{
    if constexpr (HasVerbatimRecord<ESXRecord>::value)
    {
        if (rec.state != State_Modified && rec.state != State_ModifiedOnly)
            return false;
        const ESXRecord& cur = rec.get();
        if (cur.verbatimBody.isEmpty() || !cur.verbatimSnapshot)
            return false;
        if (!(cur == *cur.verbatimSnapshot))
            return false;
        if constexpr (HasVerbatimComponents<ESXRecord>::value)
        {
            if (!(cur.components == cur.verbatimSnapshot->components))
                return false;
        }
        RecHeader header;
        header.id = cur.formId;
        header.flags.val = cur.verbatimFlags;
        writer.startRecord(tag, header);
        writer.writeRawData(cur.verbatimBody.constData(),
            static_cast<qint32>(cur.verbatimBody.size()));
        writer.endRecord();
        return true;
    }
    return false;
}

#endif // VERBATIMRECORD_HPP
