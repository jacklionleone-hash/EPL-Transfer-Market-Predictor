// lookup.h: showing the model's results
#pragma once

#include "types.h"
#include "model.h"

// Ranks older players' (peak 2012/13 – 2018/19) best seasons in today's market
void printTopOlderPlayers(const Dataset& data, const Model& model, int howMany);

// Asks for a player and a season with cin, then prints the estimate. Loops until 'quit'.
void runLookup(const Dataset& data, const Model& model);
