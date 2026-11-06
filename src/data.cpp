// data.cpp
#include "data.h"
#include "csv.h"
#include "helpers.h"
#include <fstream>
#include <iostream>

bool loadAppearances(const string& path, Dataset& data) {
    ifstream appFile(path);
    if (!appFile) {
        cerr << "Error: couldn't open " << path << endl;
        cerr << "(Are you running from the project root?)" << endl;
        return false;
    }

    string line;
    getline(appFile, line);  // skip the header
    int eplApps = 0;
    int skippedApps = 0;

    while (getline(appFile, line)) {
        vector<string> fields = splitLine(line);
        if (fields.size() != 13) { skippedApps++; continue; }

        // appearances.csv: 2 = player_id, 5 = date, 7 = competition,
        //                  10 = goals, 11 = assists, 12 = minutes
        if (fields[7] != "GB1") continue;
        eplApps++;
        int id = stoi(fields[2]);

        int goals   = fields[10].empty() ? 0 : stoi(fields[10]);
        int assists = fields[11].empty() ? 0 : stoi(fields[11]);
        int minutes = fields[12].empty() ? 0 : stoi(fields[12]);

        // career totals
        Stats& career = data.statsById[id];
        career.goals   += goals;
        career.assists += assists;
        career.minutes += minutes;
        career.games++;

        // which season? date looks like "2015-08-08"
        // Aug–Dec 2015 and Jan–Jun 2016 both count as season 2015
        string date = fields[5];
        int year   = stoi(date.substr(0, 4));
        int month  = stoi(date.substr(5, 2));
        int season = (month >= 7) ? year : year - 1;

        // per-season totals
        Stats& seasonStats = data.statsBySeason[id][season];
        seasonStats.goals   += goals;
        seasonStats.assists += assists;
        seasonStats.minutes += minutes;
        seasonStats.games++;
    }

    cout << "EPL appearances: " << eplApps << endl;
    cout << "Players with EPL stats: " << data.statsById.size() << endl;
    cout << "Skipped appearance rows: " << skippedApps << endl;
    return true;
}

bool loadPlayers(const string& path, Dataset& data) {
    ifstream file(path);
    if (!file) {
        cerr << "Error: couldn't open " << path << endl;
        return false;
    }

    string line;
    getline(file, line);  // skip the header

    int count = 0;
    int skippedPlayers = 0;

    while (getline(file, line)) {
        count++;
        vector<string> fields = splitLine(line);
        if (fields.size() != 26) { skippedPlayers++; continue; }

        // keep the player only if they have EPL appearances
        int id = stoi(fields[0]);
        if (data.statsById.count(id) == 0) continue;

        // players.csv: 0 = id, 3 = name, 4 = last_season, 10 = birth date,
        //              11 = sub_position, 12 = position,
        //              24 = market value, 25 = highest market value
        Player p;
        p.id           = id;
        p.name         = fields[3];
        p.birthDate    = fields[10];
        p.position     = fields[12];
        p.subPosition  = fields[11];
        p.lastSeason   = fields[4].empty()  ? 0 : stoi(fields[4]);
        p.marketValue  = fields[24].empty() ? 0 : stoll(fields[24]);
        p.highestValue = fields[25].empty() ? 0 : stoll(fields[25]);
        data.players.push_back(p);
    }

    cout << "Total players: " << count << endl;
    cout << "Skipped player rows: " << skippedPlayers << endl;
    cout << "EPL players stored: " << data.players.size() << endl << endl;
    return true;
}

vector<Sample> buildTrainingSet(const Dataset& data, int trainSeason) {
    vector<Sample> training;

    for (const Player& p : data.players) {
        // still active, with a real current value
        if (p.lastSeason < trainSeason) continue;
        if (p.marketValue <= 0) continue;
        if (!isAttackOrMidfield(p)) continue;

        // did they play an EPL season in trainSeason?
        // (.find on a const map: [] would try to create an entry)
        auto playerIt = data.statsBySeason.find(p.id);
        if (playerIt == data.statsBySeason.end()) continue;
        auto seasonIt = playerIt->second.find(trainSeason);
        if (seasonIt == playerIt->second.end()) continue;
        Stats s = seasonIt->second;

        if (s.minutes < 1500) continue;              // real season only

        int age = ageAtSeason(p.birthDate, trainSeason);
        if (age == -1) continue;                     // missing birth date

        Sample sm;
        sm.name    = p.name;
        sm.goals   = s.goals;
        sm.assists = s.assists;
        sm.minutes = s.minutes;
        sm.age     = age;
        sm.value   = p.marketValue;                  // today's price
        training.push_back(sm);
    }
    return training;
}
