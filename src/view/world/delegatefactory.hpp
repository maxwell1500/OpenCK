#ifndef DELEGATE_FACTORY_H
#define DELEGATE_FACTORY_H

#include "genericdelegate.hpp"

#include <QObject>

class Document;

class DelegateFactory
{
public:
    virtual ~DelegateFactory() = default;
    virtual GenericDelegate* makeDelegate(Document& document, QObject* parent) const = 0;
};

#endif // DELEGATE_FACTORY_H