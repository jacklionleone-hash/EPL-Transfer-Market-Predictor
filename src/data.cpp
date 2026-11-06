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

        // appearances.csv: 2 = player_id, 3 = player_club_id, 5 = date,
        //                  7 = competition, 10 = goals, 11 = assists, 12 = minutes
        if (fields[7] != "GB1") continue;
        eplApps++;
        int id = stoi(fields[2]);
        int clubId = fields[3].empty() ? -1 : stoi(fields[3]);

        int goals   = fields[10].empty() ? 0 : stoi(fields[10]);
        int assists = fields[11].empty() ? 0 : stoi(fields[11]);
        int minutes = fields[12].empty() ? 0 : stoi(fields[12]);

        // career totals
        Stats& career = data.statsById[id];
        career.goals   += goals;
        career.assists += assists;
        career.minutes += minutes;
        career.games++;

        int season = seasonFromDate(fields[5]);

        // per-season totals
        Stats& seasonStats = data.statsBySeason[id][season];
        seasonStats.goals   += goals;
        seasonStats.assists += assists;
        seasonStats.minutes += minutes;
        seasonStats.games++;

        // minutes per club, so we know which team he mainly played for
        if (clubId != -1) data.clubMinutes[id][season][clubId] += minutes;
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

// Finds a column's index in a header line by its name, or -1 if it's not there
static int columnIndex(const vector<string>& header, const string& name) {
    for (int i = 0; i < (int)header.size(); i++) {
        if (header[i] == name) return i;
    }
    return -1;
}

bool loadValuations(const string& path, Dataset& data) {
    ifstream file(path);
    if (!file) {
        cerr << "Error: couldn't open " << path << endl;
        return false;
    }

    // find the columns by NAME, so the order in the file doesn't matter
    string line;
    getline(file, line);
    vector<string> header = splitLine(line);
    int idCol    = columnIndex(header, "player_id");
    int dateCol  = columnIndex(header, "date");
    int valueCol = columnIndex(header, "market_value_in_eur");
    if (idCol == -1 || dateCol == -1 || valueCol == -1) {
        cerr << "Error: player_valuations.csv is missing a column. Header was:" << endl;
        cerr << line << endl;
        return false;
    }

    int kept = 0;
    while (getline(file, line)) {
        vector<string> fields = splitLine(line);
        if ((int)fields.size() != (int)header.size()) continue;
        if (fields[idCol].empty() || fields[valueCol].empty()) continue;
        if (fields[dateCol].size() < 10) continue;

        int id = stoi(fields[idCol]);
        if (data.statsById.count(id) == 0) continue;   // only EPL players

        Valuation v;
        v.date  = fields[dateCol].substr(0, 10);         // "2023-06-01 00:00:00" → "2023-06-01"
        v.value = stoll(fields[valueCol]);
        data.valuations[id].push_back(v);
        kept++;
    }

    cout << "Valuations stored: " << kept << endl;
    return true;
}

bool loadGames(const string& path, Dataset& data) {
    ifstream file(path);
    if (!file) {
        cerr << "Error: couldn't open " << path << endl;
        return false;
    }

    string line;
    getline(file, line);
    vector<string> header = splitLine(line);
    int compCol      = columnIndex(header, "competition_id");
    int dateCol      = columnIndex(header, "date");
    int homeCol      = columnIndex(header, "home_club_id");
    int awayCol      = columnIndex(header, "away_club_id");
    int homeGoalsCol = columnIndex(header, "home_club_goals");
    int awayGoalsCol = columnIndex(header, "away_club_goals");
    if (compCol == -1 || dateCol == -1 || homeCol == -1 || awayCol == -1 ||
        homeGoalsCol == -1 || awayGoalsCol == -1) {
        cerr << "Error: games.csv is missing a column. Header was:" << endl;
        cerr << line << endl;
        return false;
    }

    // club → season → total points / games played
    map<int, map<int, int>> points, played;
    int eplGames = 0;

    while (getline(file, line)) {
        vector<string> f = splitLine(line);
        if ((int)f.size() != (int)header.size()) continue;
        if (f[compCol] != "GB1") continue;
        if (f[dateCol].size() < 10 || f[homeCol].empty() || f[awayCol].empty()) continue;
        if (f[homeGoalsCol].empty() || f[awayGoalsCol].empty()) continue;   // not played yet

        int season = seasonFromDate(f[dateCol]);
        int home = stoi(f[homeCol]), away = stoi(f[awayCol]);
        int hg = stoi(f[homeGoalsCol]), ag = stoi(f[awayGoalsCol]);

        // win = 3 points, draw = 1, loss = 0
        if (hg > ag)       points[home][season] += 3;
        else if (hg < ag)  points[away][season] += 3;
        else { points[home][season] += 1; points[away][season] += 1; }
        played[home][season]++;
        played[away][season]++;
        eplGames++;
    }

    for (auto& club : played) {
        for (auto& s : club.second) {
            data.teamPPG[club.first][s.first] =
                (double)points[club.first][s.first] / s.second;
        }
    }

    cout << "EPL games: " << eplGames << endl;
    return true;
}

double teamStrength(const Dataset& data, int playerId, int season) {
    auto pIt = data.clubMinutes.find(playerId);
    if (pIt == data.clubMinutes.end()) return -1;
    auto sIt = pIt->second.find(season);
    if (sIt == pIt->second.end()) return -1;

    // the club he played the most minutes for ("keep the best so far" again)
    int bestClub = -1, bestMinutes = -1;
    for (auto entry : sIt->second) {
        if (entry.second > bestMinutes) {
            bestMinutes = entry.second;
            bestClub = entry.first;
        }
    }

    auto cIt = data.teamPPG.find(bestClub);
    if (cIt == data.teamPPG.end()) return -1;
    auto ppgIt = cIt->second.find(season);
    if (ppgIt == cIt->second.end()) return -1;
    return ppgIt->second;
}

long long endOfSeasonValue(const Dataset& data, int playerId, int season) {
    auto it = data.valuations.find(playerId);
    if (it == data.valuations.end()) return -1;

    // season 2022 = 2022/23, so look for the last value between Jan 1 and Jul 31, 2023.
    // Dates in "YYYY-MM-DD" form compare correctly as plain strings.
    string from = to_string(season + 1) + "-01-01";
    string to   = to_string(season + 1) + "-07-31";

    string bestDate = "";
    long long bestValue = -1;
    for (const Valuation& v : it->second) {
        if (v.date >= from && v.date <= to && v.date > bestDate) {
            bestDate = v.date;
            bestValue = v.value;
        }
    }
    return bestValue;
}

map<int, double> computeInflation(const Dataset& data, int firstSeason, int targetSeason) {
    // average end-of-season value of everyone who played an EPL game that season
    map<int, double> total, count;
    for (const Player& p : data.players) {
        auto playerIt = data.statsBySeason.find(p.id);
        if (playerIt == data.statsBySeason.end()) continue;

        for (int season = firstSeason; season <= targetSeason; season++) {
            if (playerIt->second.count(season) == 0) continue;
            long long v = endOfSeasonValue(data, p.id, season);
            if (v <= 0) continue;
            total[season] += v;
            count[season] += 1;
        }
    }

    map<int, double> avg, factor;
    for (int season = firstSeason; season <= targetSeason; season++) {
        avg[season] = count[season] > 0 ? total[season] / count[season] : 0;
    }
    for (int season = firstSeason; season <= targetSeason; season++) {
        // factor = how many times bigger today's average is than that season's
        factor[season] = avg[season] > 0 ? avg[targetSeason] / avg[season] : 1.0;
    }
    return factor;
}

vector<Sample> buildTrainingSet(const Dataset& data, int firstSeason, int lastSeason) {
    map<int, double> inflation = computeInflation(data, firstSeason, lastSeason);

    cout << "Market inflation (multiply old values by this to get "
         << seasonLabel(lastSeason) << " money):" << endl;
    for (auto entry : inflation) {
        cout << "  " << seasonLabel(entry.first) << ": x" << entry.second << endl;
    }
    cout << endl;

    vector<Sample> training;

    for (const Player& p : data.players) {
        if (!isAttackOrMidfield(p)) continue;

        // .find on a const map: [] would try to create an entry
        auto playerIt = data.statsBySeason.find(p.id);
        if (playerIt == data.statsBySeason.end()) continue;

        for (int season = firstSeason; season <= lastSeason; season++) {
            auto seasonIt = playerIt->second.find(season);
            if (seasonIt == playerIt->second.end()) continue;
            Stats s = seasonIt->second;

            if (s.minutes < 1500) continue;              // real season only

            int age = ageAtSeason(p.birthDate, season);
            if (age == -1) continue;                     // missing birth date

            long long value = endOfSeasonValue(data, p.id, season);
            if (value <= 0) continue;                    // no valuation that summer

            double ppg = teamStrength(data, p.id, season);
            if (ppg < 0) continue;                       // unknown team

            Sample sm;
            sm.name     = p.name;
            sm.playerId = p.id;
            sm.season   = season;
            sm.teamPPG  = ppg;
            sm.goals   = s.goals;
            sm.assists = s.assists;
            sm.minutes = s.minutes;
            sm.age     = age;
            sm.value   = value * inflation[season];      // in today's money
            training.push_back(sm);
        }
    }
    return training;
}