// data.h: loading the CSV files and building the training set
#pragma once

#include <string>
#include <vector>
#include "types.h"
using namespace std;

// Reads appearances.csv and adds up EPL stats (career + per season).
// Returns false if the file can't be opened.
bool loadAppearances(const string& path, Dataset& data);

// Reads players.csv and keeps everyone who has EPL stats.
// Call loadAppearances FIRST. Returns false if the file can't be opened.
bool loadPlayers(const string& path, Dataset& data);

// Active attackers/midfielders with a real season in trainSeason + today's value
vector<Sample> buildTrainingSet(const Dataset& data, int trainSeason);
