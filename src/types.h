// types.h: the data structures the whole program shares
#pragma once

#include <string>
#include <vector>
#include <map>
using namespace std;

// One player from players.csv
struct Player {
    int id;
    string name;
    string birthDate;
    string position;
    string subPosition;
    int lastSeason;
    long long marketValue;
    long long highestValue;
};

// Goals/assists/minutes/games added up over some set of games
struct Stats {
    int goals = 0;
    int assists = 0;
    int minutes = 0;
    int games = 0;
};

// One training example: a player's stats in a season + their value
struct Sample {
    string name;
    double goals, assists, minutes, age;   // inputs (x's)
    double value;                          // what we predict (y), in €
};

// season → Stats for one player
using SeasonMap = map<int, Stats>;

// Everything loaded from the CSV files
struct Dataset {
    vector<Player> players;              // everyone who played in the EPL
    map<int, Stats> statsById;           // player_id → career EPL totals
    map<int, SeasonMap> statsBySeason;   // player_id → (season → Stats)
};
