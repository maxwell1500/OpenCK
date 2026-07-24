#ifndef MORROWINDGAMEPARSER_H
#define MORROWINDGAMEPARSER_H
#include "igameparser.hpp"
class MorrowindGameParser : public IGameParser {
public:
    MorrowindGameParser();
    void parse(QIODevice* device) override;
};
#endif
