#include "recordpaste.hpp"

#include "addrecordcommand.hpp"
#include "componentrecordtypes.hpp"
#include "undostack.hpp"

#include "../world/basecollection.hpp"
#include "../world/collection.hpp"
#include "../world/idtable.hpp"
#include "../world/record.hpp"

#include "../../../libs/files/esm/actirecord.hpp"
#include "../../../libs/files/esm/actorvalueinforecord.hpp"
#include "../../../libs/files/esm/alchrecord.hpp"
#include "../../../libs/files/esm/ammorecord.hpp"
#include "../../../libs/files/esm/aniorecord.hpp"
#include "../../../libs/files/esm/apparatusrecord.hpp"
#include "../../../libs/files/esm/artvrecord.hpp"
#include "../../../libs/files/esm/armorrecord.hpp"
#include "../../../libs/files/esm/birthsignrecord.hpp"
#include "../../../libs/files/esm/bookrecord.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/classrecord.hpp"
#include "../../../libs/files/esm/clfmrecord.hpp"
#include "../../../libs/files/esm/climaterecord.hpp"
#include "../../../libs/files/esm/clothrecord.hpp"
#include "../../../libs/files/esm/constructibleobjectrecord.hpp"
#include "../../../libs/files/esm/combatstylerecord.hpp"
#include "../../../libs/files/esm/contrecord.hpp"
#include "../../../libs/files/esm/creaturerecord.hpp"
#include "../../../libs/files/esm/debrrecord.hpp"
#include "../../../libs/files/esm/dialrecord.hpp"
#include "../../../libs/files/esm/doorrecord.hpp"
#include "../../../libs/files/esm/effectshaderrecord.hpp"
#include "../../../libs/files/esm/ecznrecord.hpp"
#include "../../../libs/files/esm/enchrecord.hpp"
#include "../../../libs/files/esm/explosionrecord.hpp"
#include "../../../libs/files/esm/eyesrecord.hpp"
#include "../../../libs/files/esm/factrecord.hpp"
#include "../../../libs/files/esm/florrecord.hpp"
#include "../../../libs/files/esm/formlistrecord.hpp"
#include "../../../libs/files/esm/furnrecord.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/grassrecord.hpp"
#include "../../../libs/files/esm/hairrecord.hpp"
#include "../../../libs/files/esm/hazdrecord.hpp"
#include "../../../libs/files/esm/hdptrecord.hpp"
#include "../../../libs/files/esm/idleanimationrecord.hpp"
#include "../../../libs/files/esm/idlemarkerrecord.hpp"
#include "../../../libs/files/esm/imagespacerecord.hpp"
#include "../../../libs/files/esm/inforecord.hpp"
#include "../../../libs/files/esm/ingrrecord.hpp"
#include "../../../libs/files/esm/ipctrecord.hpp"
#include "../../../libs/files/esm/ipdsrecord.hpp"
#include "../../../libs/files/esm/keymrecord.hpp"
#include "../../../libs/files/esm/keywordrecord.hpp"
#include "../../../libs/files/esm/landrecord.hpp"
#include "../../../libs/files/esm/lighrecord.hpp"
#include "../../../libs/files/esm/loadscreenrecord.hpp"
#include "../../../libs/files/esm/locationrecord.hpp"
#include "../../../libs/files/esm/ltexrecord.hpp"
#include "../../../libs/files/esm/lvlcreaturerecord.hpp"
#include "../../../libs/files/esm/lvlistrecord.hpp"
#include "../../../libs/files/esm/lvspellrecord.hpp"
#include "../../../libs/files/esm/magicrecord.hpp"
#include "../../../libs/files/esm/materialrecord.hpp"
#include "../../../libs/files/esm/mattrecord.hpp"
#include "../../../libs/files/esm/movtrecord.hpp"
#include "../../../libs/files/esm/muscrecord.hpp"
#include "../../../libs/files/esm/ffkwrecord.hpp"
#include "../../../libs/files/esm/fogvrecord.hpp"
#include "../../../libs/files/esm/forcrecord.hpp"
#include "../../../libs/files/esm/fstprecord.hpp"
#include "../../../libs/files/esm/fstsrecord.hpp"
#include "../../../libs/files/esm/fxpdrecord.hpp"
#include "../../../libs/files/esm/gbfmrecord.hpp"
#include "../../../libs/files/esm/gbftrecord.hpp"
#include "../../../libs/files/esm/gcvrrecord.hpp"
#include "../../../libs/files/esm/imadrecord.hpp"
#include "../../../libs/files/esm/innrrecord.hpp"
#include "../../../libs/files/esm/iresrecord.hpp"
#include "../../../libs/files/esm/kssmrecord.hpp"
#include "../../../libs/files/esm/layrrecord.hpp"
#include "../../../libs/files/esm/lensrecord.hpp"
#include "../../../libs/files/esm/lgdirecord.hpp"
#include "../../../libs/files/esm/lgtmrecord.hpp"
#include "../../../libs/files/esm/lmswrecord.hpp"
#include "../../../libs/files/esm/lvlbrecord.hpp"
#include "../../../libs/files/esm/lvlnrecord.hpp"
#include "../../../libs/files/esm/lvlprecord.hpp"
#include "../../../libs/files/esm/lvscrecord.hpp"
#include "../../../libs/files/esm/maamrecord.hpp"
#include "../../../libs/files/esm/mrhprecord.hpp"
#include "../../../libs/files/esm/mtptrecord.hpp"
#include "../../../libs/files/esm/navirecord.hpp"
#include "../../../libs/files/esm/nocmrecord.hpp"
#include "../../../libs/files/esm/omodrecord.hpp"
#include "../../../libs/files/esm/oswprecord.hpp"
#include "../../../libs/files/esm/ovisrecord.hpp"
#include "../../../libs/files/esm/pcbnrecord.hpp"
#include "../../../libs/files/esm/pccnrecord.hpp"
#include "../../../libs/files/esm/pcmtrecord.hpp"
#include "../../../libs/files/esm/pdclrecord.hpp"
#include "../../../libs/files/esm/pgrerecord.hpp"
#include "../../../libs/files/esm/messagerecord.hpp"
#include "../../../libs/files/esm/miscrecord.hpp"
#include "../../../libs/files/esm/msttrecord.hpp"
#include "../../../libs/files/esm/mustrecord.hpp"
#include "../../../libs/files/esm/navmrecord.hpp"
#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/noterecord.hpp"
#include "../../../libs/files/esm/outfitrecord.hpp"
#include "../../../libs/files/esm/packagerecord.hpp"
#include "../../../libs/files/esm/perkrecord.hpp"
#include "../../../libs/files/esm/pndrecord.hpp"
#include "../../../libs/files/esm/projectilerecord.hpp"
#include "../../../libs/files/esm/questrecord.hpp"
#include "../../../libs/files/esm/racerecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"
#include "../../../libs/files/esm/regionrecord.hpp"
#include "../../../libs/files/esm/relarecord.hpp"
#include "../../../libs/files/esm/revbrecord.hpp"
#include "../../../libs/files/esm/roadrecord.hpp"
#include "../../../libs/files/esm/scenrecord.hpp"
#include "../../../libs/files/esm/scriptrecord.hpp"
#include "../../../libs/files/esm/scrollrecord.hpp"
#include "../../../libs/files/esm/shaderparticlerecord.hpp"
#include "../../../libs/files/esm/shourecord.hpp"
#include "../../../libs/files/esm/slgmrecord.hpp"
#include "../../../libs/files/esm/sounrecord.hpp"
#include "../../../libs/files/esm/soundmarkerrecord.hpp"
#include "../../../libs/files/esm/spellrecord.hpp"
#include "../../../libs/files/esm/staticcollectionrecord.hpp"
#include "../../../libs/files/esm/statrecord.hpp"
#include "../../../libs/files/esm/Tes3record.hpp"
#include "../../../libs/files/esm/termrecord.hpp"
#include "../../../libs/files/esm/texturesetrecord.hpp"
#include "../../../libs/files/esm/treerecord.hpp"
#include "../../../libs/files/esm/waterecord.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "../../../libs/files/esm/wthrrecord.hpp"
#include "../../../libs/files/esm/aactrecord.hpp"
#include "../../../libs/files/esm/aamdrecord.hpp"
#include "../../../libs/files/esm/aapdrecord.hpp"
#include "../../../libs/files/esm/achrrecord.hpp"
#include "../../../libs/files/esm/addnrecord.hpp"
#include "../../../libs/files/esm/afferecord.hpp"
#include "../../../libs/files/esm/ambsrecord.hpp"
#include "../../../libs/files/esm/amdlrecord.hpp"
#include "../../../libs/files/esm/aopfrecord.hpp"
#include "../../../libs/files/esm/aopsrecord.hpp"
#include "../../../libs/files/esm/aorurecord.hpp"
#include "../../../libs/files/esm/armarecord.hpp"
#include "../../../libs/files/esm/artorecord.hpp"
#include "../../../libs/files/esm/aspcrecord.hpp"
#include "../../../libs/files/esm/atmrecord.hpp"
#include "../../../libs/files/esm/avmdrecord.hpp"
#include "../../../libs/files/esm/biomrecord.hpp"
#include "../../../libs/files/esm/bmmorecord.hpp"
#include "../../../libs/files/esm/bmodrecord.hpp"
#include "../../../libs/files/esm/bndsrecord.hpp"
#include "../../../libs/files/esm/bptdrecord.hpp"
#include "../../../libs/files/esm/camsrecord.hpp"
#include "../../../libs/files/esm/chalrecord.hpp"
#include "../../../libs/files/esm/cldfrecord.hpp"
#include "../../../libs/files/esm/cndfrecord.hpp"
#include "../../../libs/files/esm/collrecord.hpp"
#include "../../../libs/files/esm/cpthrecord.hpp"
#include "../../../libs/files/esm/dlbrrecord.hpp"
#include "../../../libs/files/esm/cur3record.hpp"
#include "../../../libs/files/esm/curvrecord.hpp"
#include "../../../libs/files/esm/dfobrecord.hpp"
#include "../../../libs/files/esm/dmgtrecord.hpp"
#include "../../../libs/files/esm/dobjrecord.hpp"
#include "../../../libs/files/esm/efsqrecord.hpp"
#include "../../../libs/files/esm/equprecord.hpp"
#include "../../../libs/files/esm/phzdrecord.hpp"
#include "../../../libs/files/esm/pkinrecord.hpp"
#include "../../../libs/files/esm/pmftrecord.hpp"
#include "../../../libs/files/esm/psdcrecord.hpp"
#include "../../../libs/files/esm/ptstrecord.hpp"
#include "../../../libs/files/esm/rfgprecord.hpp"
#include "../../../libs/files/esm/rsgdrecord.hpp"
#include "../../../libs/files/esm/rspjrecord.hpp"
#include "../../../libs/files/esm/sdltrecord.hpp"
#include "../../../libs/files/esm/sechrecord.hpp"
#include "../../../libs/files/esm/sfbkrecord.hpp"
#include "../../../libs/files/esm/sfpcrecord.hpp"
#include "../../../libs/files/esm/sfptrecord.hpp"
#include "../../../libs/files/esm/sftrrecord.hpp"
#include "../../../libs/files/esm/smbnrecord.hpp"
#include "../../../libs/files/esm/smenrecord.hpp"
#include "../../../libs/files/esm/spchrecord.hpp"
#include "../../../libs/files/esm/stagrecord.hpp"
#include "../../../libs/files/esm/stbhrecord.hpp"
#include "../../../libs/files/esm/stdtrecord.hpp"
#include "../../../libs/files/esm/stmprecord.hpp"
#include "../../../libs/files/esm/stndrecord.hpp"
#include "../../../libs/files/esm/sunprecord.hpp"
#include "../../../libs/files/esm/tmlmrecord.hpp"
#include "../../../libs/files/esm/toddrecord.hpp"
#include "../../../libs/files/esm/travrecord.hpp"
#include "../../../libs/files/esm/trnsrecord.hpp"
#include "../../../libs/files/esm/volirecord.hpp"
#include "../../../libs/files/esm/vtyprecord.hpp"
#include "../../../libs/files/esm/wbarrecord.hpp"
#include "../../../libs/files/esm/wkmfrecord.hpp"
#include "../../../libs/files/esm/wthsrecord.hpp"
#include "../../../libs/files/esm/wwedrecord.hpp"
#include "../../../libs/files/esm/zoomrecord.hpp"
namespace openck {
namespace {

template <typename T>
bool tryCopyRecordForPaste(BaseCollection* coll, int recordIndex,
                           const QString& editorId, quint32 formId,
                           std::unique_ptr<BaseRecord>& out)
{
    auto* typed = dynamic_cast<Collection<T>*>(coll);
    if (!typed)
        return false;
    if (recordIndex < 0 || recordIndex >= typed->size())
        return false;
    std::unique_ptr<BaseRecord> copy = typed->cloneRecordAt(recordIndex);
    if (!copy)
        return false;
    auto* typedCopy = static_cast<Record<T>*>(copy.get());
    typedCopy->get().editorId = editorId;
    typedCopy->get().formId = formId;
    // The pasted record is new, so it must not inherit the source's flags:
    // a base record that was never modified becomes modified-only.
    typedCopy->state = State_ModifiedOnly;
    out = std::move(copy);
    return true;
}

template <typename T>
bool tryAddRecordCopy(BaseCollection* coll, int recordIndex,
                      const QString& editorId, quint32 formId,
                      IdTable* table, UndoStack* stack,
                      const QString& description)
{
    std::unique_ptr<BaseRecord> copy;
    if (!tryCopyRecordForPaste<T>(coll, recordIndex, editorId, formId, copy))
        return false;
    if (!table || !stack)
        return false;
    auto* typed = static_cast<Collection<T>*>(coll);
    stack->push(new AddRecordCommand(table, coll,
        typed->getAppendIndex(editorId, CkId::Type_None), *copy, description));
    return true;
}

} // namespace

bool copyRecordForPaste(BaseCollection* coll, int recordIndex,
                        const QString& editorId, quint32 formId,
                        std::unique_ptr<BaseRecord>& out)
{
#define PASTE_COPY_TYPE(recType) \
    if (tryCopyRecordForPaste<recType>(coll, recordIndex, editorId, formId, out)) return true;
    FOR_EACH_COMPONENT_RECORD_TYPE(PASTE_COPY_TYPE)
#undef PASTE_COPY_TYPE
    return false;
}

bool addRecordCopyThroughUndo(BaseCollection* coll, int recordIndex,
                              const QString& editorId, quint32 formId,
                              IdTable* table, UndoStack* stack,
                              const QString& description)
{
#define PASTE_ADD_TYPE(recType) \
    if (tryAddRecordCopy<recType>(coll, recordIndex, editorId, formId, table, stack, description)) return true;
    FOR_EACH_COMPONENT_RECORD_TYPE(PASTE_ADD_TYPE)
#undef PASTE_ADD_TYPE
    return false;
}

} // namespace openck
