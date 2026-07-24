#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "../doc/messages.hpp"
#include "../world/data.hpp"

#include <QString>

class Validator
{
public:
    virtual ~Validator() = default;
    virtual QString name() const = 0;
    virtual void validate(const Data& data, Messages& messages) = 0;
};

#endif // VALIDATOR_H
