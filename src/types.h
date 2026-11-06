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
    int season;                            // which season the stats are from
    double goals, assists, minutes, age;   // inputs (x's)
    double value;                          // what we predict (y), in €, adjusted to today's market
};

// season → Stats for one player
using SeasonMap = map<int, Stats>;

// One market value record: date "YYYY-MM-DD" + value in €
struct Valuation {
    string date;
    long long value;
};

// Everything loaded from the CSV files
struct Dataset {
    vector<Player> players;                    // everyone who played in the EPL
    map<int, Stats> statsById;                 // player_id → career EPL totals
    map<int, SeasonMap> statsBySeason;         // player_id → (season → Stats)
    map<int, vector<Valuation>> valuations;    // player_id → value history
};