#include "baseidtable.hpp"

BaseIdTable::BaseIdTable(unsigned int features, QObject* parent) :
    QAbstractItemModel(parent),
    features(features)
{

}

unsigned int BaseIdTable::getFeatures() const
{
    return features;
}