#ifndef SKYRIMGAMEPARSER_H
#define SKYRIMGAMEPARSER_H
#include "igameparser.hpp"
class SkyrimGameParser : public IGameParser {
public:
    SkyrimGameParser();
    void parse(QIODevice* device) override;
};
#endif
