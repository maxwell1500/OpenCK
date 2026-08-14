#ifndef IDCOLLECTION_HPP
#define IDCOLLECTION_HPP

#include "../../../libs/files/esm/esmreader.hpp"
#include "../../../libs/files/log/logger.hpp"
#include "collection.hpp"

template<typename ESXRecord, typename IdAccessorT = IdAccessor<ESXRecord>>
class IdCollection : public Collection<ESXRecord, IdAccessorT>
{
public:
    int load(ESMReader& esm, bool base);
    int load(const ESXRecord& record, int index, bool base);

private:
    virtual void loadRecord(ESXRecord& record, ESMReader& reader, bool base);
};

template<typename ESXRecord, typename IdAccessorT>
void IdCollection<ESXRecord, IdAccessorT>::loadRecord(ESXRecord& record, ESMReader& reader, bool base)
{
    record.load(reader, base);
}

template<typename ESXRecord, typename IdAccessorT>
int IdCollection<ESXRecord, IdAccessorT>::load(const ESXRecord& record, int index, bool base)
{
    if (index == -1)
    {
        Record<ESXRecord> rec;
        rec.state = base ? State::State_Base : State::State_ModifiedOnly;
        (base ? rec.baseRecord : rec.modifiedRecord) = record;

        index = this->size();
        this->appendRecord(rec);
    }

    return index;
}

template<typename ESXRecord, typename IdAccessorT>
int IdCollection<ESXRecord, IdAccessorT>::load(ESMReader& esm, bool base)
{
    ESXRecord record;
    loadRecord(record, esm, base);

    if (esm.recLeft() != 0)
    {
        // A loader that does not consume exactly its declared record size
        // desyncs every following record in the file. Surface it loudly so
        // misparses are caught against real data instead of producing
        // silently corrupted records.
        LOG_WARNING(QString("Record %1 left %2 unconsumed bytes in record")
            .arg(IdAccessorT().getId(record))
            .arg(esm.recLeft()));
    }

    QString id = IdAccessorT().getId(record);
    int index = this->searchId(id);

    return load(record, index, base);
}

#endif // IDCOLLECTION_HPP
