#ifndef OBLIVIONGAMEPARSER_H
#define OBLIVIONGAMEPARSER_H
#include "igameparser.hpp"
class OblivionGameParser : public IGameParser {
public:
    OblivionGameParser();
    void parse(QIODevice* device) override;
};
#endif
