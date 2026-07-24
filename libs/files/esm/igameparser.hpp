#ifndef IGAMEPARSER_H
#define IGAMEPARSER_H
#include <QIODevice>
class IGameParser {
public:
    virtual ~IGameParser() {}
    virtual void parse(QIODevice* device) = 0;
};
#endif
