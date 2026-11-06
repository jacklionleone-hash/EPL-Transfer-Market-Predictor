// helpers.h: small utility functions for seasons, ages and names
#pragma once

#include <string>
#include "types.h"
using namespace std;

// Season with the most goals + assists (min 1500 minutes), or -1 if none
int findPeakSeason(const SeasonMap& seasons);

// Turns 2018 into "2018/19"
string seasonLabel(int season);

// Which season a date belongs to: "2015-08-08" and "2016-02-01" are both 2015.
// (Aug–Dec 2015 and Jan–Jun 2016 are the 2015/16 season.)
int seasonFromDate(const string& date);

// Age at the middle of a season (around Jan 1). -1 if birth date is missing.
int ageAtSeason(const string& birthDate, int season);

// Lowercase copy of a string, so "kane" matches "Harry Kane"
string toLower(string s);

// True for the positions the model is trained on
bool isAttackOrMidfield(const Player& p);