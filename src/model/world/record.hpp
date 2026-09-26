#ifndef RECORD_H
#define RECORD_H

#include "../../../libs/components/formcomponents.hpp"

#include <memory>
#include <stdexcept>
#include <type_traits>

namespace openck { class FormComponents; }

enum State
{
    State_Base = 0,         // Base record (in parent master)
    State_Modified,         // Modified record (defined in master, modified in plugin)
    State_ModifiedOnly,     // Modified record (defined in plugin)
    State_Deleted,          // Deleted record
    State_Erased
};

class BaseRecord
{
public:
    State state;

    virtual ~BaseRecord();

    virtual std::unique_ptr<BaseRecord> clone() const = 0;
    virtual std::unique_ptr<BaseRecord> modifiedCopy() const = 0;
    virtual void assign(const BaseRecord& record) = 0;

    bool isModified() const;
    bool isErased() const;
    bool isDeleted() const;
};

/// Detects record structs that embed a `components` member, so that
/// TypedRecordEditSession only offers working components for the types that
/// have one.
template <typename T, typename = void>
struct HasFormComponents : std::false_type {};

template <typename T>
struct HasFormComponents<T, std::void_t<decltype(std::declval<T&>().components)>>
    : std::true_type {};

template<typename ESXRecord>
class Record : public BaseRecord
{
public:
    Record();
    Record(State inState, const ESXRecord* base = nullptr, const ESXRecord* modified = nullptr);

    ESXRecord& get();
    const ESXRecord& get() const;
    const ESXRecord& getBase() const;

    std::unique_ptr<BaseRecord> clone() const override;
    std::unique_ptr<BaseRecord> modifiedCopy() const override;
    void assign(const BaseRecord& record) override;

    void setModified(const ESXRecord& modified);
    void merge();

    ESXRecord baseRecord;
    ESXRecord modifiedRecord;
};

template<typename ESXRecord>
Record<ESXRecord>::Record()
    : baseRecord(), modifiedRecord()
{
    state = State_Base;
}

template<typename ESXRecord>
Record<ESXRecord>::Record(State inState, const ESXRecord* base, const ESXRecord* modified)
{
    state = inState;

    if (base)
    {
        baseRecord = *base;
    }

    if (modified)
    {
        modifiedRecord = *modified;
    }
}

template<typename ESXRecord>
std::unique_ptr<BaseRecord> Record<ESXRecord>::modifiedCopy() const
{
    return std::make_unique<Record<ESXRecord>>(State_ModifiedOnly, nullptr, &(this->get()));
}

template<typename ESXRecord>
std::unique_ptr<BaseRecord> Record<ESXRecord>::clone() const
{
    return std::make_unique<Record<ESXRecord>>(*this);
}

template<typename ESXRecord>
void Record<ESXRecord>::assign(const BaseRecord& record)
{
    *this = dynamic_cast<const Record<ESXRecord>&>(record);
}

template<typename ESXRecord>
ESXRecord& Record<ESXRecord>::get()
{
    if (state == State_Erased)
    {
        throw std::logic_error("Cannot access a deleted record.");
    }
    else
    {
        return state == State_Base || state == State_Deleted ? baseRecord : modifiedRecord;
    }
}

template<typename ESXRecord>
const ESXRecord& Record<ESXRecord>::get() const
{
    if (state == State_Erased)
    {
        throw std::logic_error("Cannot access a deleted record.");
    }
    else
    {
        return state == State_Base || state == State_Deleted ? baseRecord : modifiedRecord;
    }
}

template<typename ESXRecord>
const ESXRecord& Record<ESXRecord>::getBase() const
{
    if (state == State_Erased)
    {
        throw std::logic_error("Cannot access a deleted record.");
    }

    return state == State_ModifiedOnly ? modifiedRecord : baseRecord;
}

template<typename ESXRecord>
void Record<ESXRecord>::setModified(const ESXRecord& modified)
{
    if (state == State_Erased)
    {
        throw std::logic_error("Cannot access a deleted record.");
    }
    else
    {
        modifiedRecord = modified;
    }

    if (state != State_ModifiedOnly)
    {
        state = State_Modified;
    }
}

template<typename ESXRecord>
void Record<ESXRecord>::merge()
{
    if (isModified())
    {
        baseRecord = modifiedRecord;
        state = State_Base;
    }
    else if (state == State_Deleted)
    {
        state = State_Erased;
    }
}

#endif // RECORD_H
