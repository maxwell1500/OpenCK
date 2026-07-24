#ifndef GAMEPARSERFACTORY_H
#define GAMEPARSERFACTORY_H
#include "igameparser.hpp"
class GameParserFactory {
public:
    static IGameParser* createParser(const QString& type);
};
#endif
