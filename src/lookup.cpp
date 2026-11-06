// lookup.cpp
#include "lookup.h"
#include "data.h"
#include "helpers.h"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>

// All EPL seasons for one player (empty if none)
static SeasonMap seasonsFor(const Dataset& data, int id) {
    auto it = data.statsBySeason.find(id);
    if (it == data.statsBySeason.end()) return {};
    return it->second;
}

void printTopOlderPlayers(const Dataset& data, const Model& model, int howMany) {
    struct Result {
        string name, season;
        Stats s;
        int age;
        double ppg, todayValue, peakValueThen;
    };
    vector<Result> results;

    for (const Player& p : data.players) {
        if (!isAttackOrMidfield(p)) continue;

        SeasonMap seasons = seasonsFor(data, p.id);
        int peak = findPeakSeason(seasons);
        if (peak == -1 || peak > 2018) continue;   // "older" = peak 2012/13 – 2018/19

        int age = ageAtSeason(p.birthDate, peak);
        if (age == -1) continue;

        double ppg = teamStrength(data, p.id, peak);
        if (ppg < 0) continue;

        Result r;
        r.name = p.name;
        r.season = seasonLabel(peak);
        r.s = seasons[peak];
        r.age = age;
        r.ppg = ppg;
        r.todayValue = model.predictValue(r.s.goals, r.s.assists, r.s.minutes, age, ppg);
        r.peakValueThen = p.highestValue / 1000000.0;
        results.push_back(r);
    }

    // highest predicted value first
    sort(results.begin(), results.end(), [](const Result& a, const Result& b) {
        return a.todayValue > b.todayValue;
    });

    cout << "Top " << howMany << ": older players' peak seasons, valued in TODAY's market" << endl;
    for (int i = 0; i < howMany && i < (int)results.size(); i++) {
        const Result& r = results[i];
        cout << i + 1 << ". " << r.name << " (" << r.season << ", age " << r.age << ") "
             << r.s.goals << " G, " << r.s.assists << " A, team "
             << fixed << setprecision(2) << r.ppg << defaultfloat << setprecision(6) << " pts/game"
             << " → today: €" << (int)r.todayValue << "M"
             << " | career peak value: €" << (int)r.peakValueThen << "M" << endl;
    }
}

void runLookup(const Dataset& data, const Model& model) {
    cout << endl << "===== Player lookup =====" << endl;

    while (true) {
        // --- ask for a player ---
        cout << endl << "Enter a player name (or 'quit'): ";
        string query;
        if (!getline(cin, query)) break;          // Ctrl+D ends the program
        if (toLower(query) == "quit") break;
        if (query.empty()) continue;

        // every player whose name contains what was typed
        vector<Player> matches;
        for (const Player& p : data.players) {
            if (toLower(p.name).find(toLower(query)) != string::npos) {
                matches.push_back(p);
            }
        }

        if (matches.empty()) {
            cout << "No player found. (Tip: accents count, e.g. type \"Ag\" for Agüero.)" << endl;
            continue;
        }

        // an exact name match wins
        for (const Player& p : matches) {
            if (toLower(p.name) == toLower(query)) {
                matches = { p };
                break;
            }
        }

        // more than one match → let the user pick
        Player chosen = matches[0];
        if (matches.size() > 1) {
            cout << "Found " << matches.size() << " players:" << endl;
            int shown = min((int)matches.size(), 15);
            for (int i = 0; i < shown; i++) {
                cout << "  " << i + 1 << ". " << matches[i].name
                     << " (" << matches[i].position << ")" << endl;
            }
            if ((int)matches.size() > shown) {
                cout << "  ...and more. Type a more specific name to narrow it down." << endl;
            }
            cout << "Pick a number: ";
            string pick;
            if (!getline(cin, pick)) break;
            int k = atoi(pick.c_str());
            if (k < 1 || k > shown) {
                cout << "Invalid choice." << endl;
                continue;
            }
            chosen = matches[k - 1];
        }

        // --- show their EPL seasons ---
        SeasonMap seasons = seasonsFor(data, chosen.id);
        cout << endl << chosen.name << "'s EPL seasons:" << endl;
        for (auto entry : seasons) {
            Stats s = entry.second;
            cout << "  " << seasonLabel(entry.first) << ": "
                 << s.goals << " G, " << s.assists << " A, "
                 << s.minutes << " min" << endl;
        }
        int peak = findPeakSeason(seasons);
        if (peak != -1) cout << "  (peak season: " << seasonLabel(peak) << ")" << endl;

        // --- ask for a season ---
        cout << "Enter a season (e.g. 2015 or 2015/16): ";
        string seasonInput;
        if (!getline(cin, seasonInput)) break;
        if (seasonInput.size() < 4) {
            cout << "Invalid season." << endl;
            continue;
        }
        int season = atoi(seasonInput.substr(0, 4).c_str());

        if (seasons.count(season) == 0) {
            cout << chosen.name << " didn't play in the EPL in "
                 << seasonLabel(season) << "." << endl;
            continue;
        }

        int age = ageAtSeason(chosen.birthDate, season);
        if (age == -1) {
            cout << "Missing birth date for " << chosen.name << ", can't predict." << endl;
            continue;
        }

        double ppg = teamStrength(data, chosen.id, season);
        if (ppg < 0) {
            cout << "Couldn't find " << chosen.name << "'s team that season, can't predict." << endl;
            continue;
        }

        // --- predict ---
        Stats s = seasons[season];
        double value = model.predictValue(s.goals, s.assists, s.minutes, age, ppg);

        cout << endl << chosen.name << " in " << seasonLabel(season)
             << " (age " << age << "): " << s.goals << " G, "
             << s.assists << " A, " << s.minutes << " min, team "
             << fixed << setprecision(2) << ppg << defaultfloat << setprecision(6)
             << " pts/game" << endl;
        cout << "→ Estimated value in today's market: €"
             << (value < 0 ? 0 : (int)value) << "M" << endl;

        if (s.minutes < 1500) {
            cout << "  (careful: under 1500 minutes, so this estimate is shaky)" << endl;
        }
        if (!isAttackOrMidfield(chosen)) {
            cout << "  (careful: model was trained on attackers/midfielders only)" << endl;
        }
    }

    cout << "Bye!" << endl;
}