// data.h: loading the CSV files and building the training set
#pragma once

#include <map>
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

// Reads player_valuations.csv (value history) for players with EPL stats.
// Call loadAppearances FIRST. Returns false if the file can't be opened.
bool loadValuations(const string& path, Dataset& data);

// A player's market value at the END of a season (latest valuation between
// Jan 1 and Jul 31 of the following year), or -1 if there isn't one.
long long endOfSeasonValue(const Dataset& data, int playerId, int season);

// Market inflation: for each season, how much to multiply its values by to
// put them in targetSeason's money (e.g. 2019 → 1.6 means values grew 60%)
map<int, double> computeInflation(const Dataset& data, int firstSeason, int targetSeason);

// Active attackers/midfielders with a real EPL season, for every season from
// firstSeason to lastSeason, paired with their end-of-season value converted
// to lastSeason's money
vector<Sample> buildTrainingSet(const Dataset& data, int firstSeason, int lastSeason);