#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
using namespace std;

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

struct Stats {
    int goals = 0;
    int assists = 0;
    int minutes = 0;
    int games = 0;
};

map<int, Stats> statsById;   // player_id → their EPL totals

// Removes the " at the start and end of a field, if both are there
string stripQuotes(string s) {
    if (s.size() >= 2 && s[0] == '"' && s[s.size() - 1] == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// Splits one CSV line into its fields (quotes removed)
vector<string> splitLine(string line) {
    stringstream ss(line);
    string field;
    vector<string> fields;
    while (getline(ss, field, ',')) {
        fields.push_back(stripQuotes(field));
    }
    return fields;
}

int main() {
    // ===== PART 1: load EPL players from players.csv =====
    ifstream file("data/players.csv");
    if (!file) {
        cerr << "Error: couldn't open data/players.csv" << endl;
        cerr << "(Are you running from the project root?)" << endl;
        return 1;
    }

    string line;
    getline(file, line);  // skip the header

    int count = 0;
    vector<Player> players;

    while (getline(file, line)) {
        count++;
        vector<string> fields = splitLine(line);
        if (fields.size() < 26) continue;

        // players.csv: column 22 = league, GB1 = Premier League
        if (fields[22] == "GB1") {
            Player p;
            p.id           = stoi(fields[0]);
            p.name         = fields[3];
            p.birthDate    = fields[10];
            p.position     = fields[12];
            p.subPosition  = fields[11];
            p.lastSeason   = fields[4].empty()  ? 0 : stoi(fields[4]);
            p.marketValue  = fields[24].empty() ? 0 : stoll(fields[24]);
            p.highestValue = fields[25].empty() ? 0 : stoll(fields[25]);
            players.push_back(p);
        }
    }

    cout << "Total players: " << count << endl;
    cout << "EPL players stored: " << players.size() << endl;

    // ===== PART 2: add up EPL stats from appearances.csv =====
    ifstream appFile("data/appearances.csv");
    if (!appFile) {
        cerr << "Error: couldn't open data/appearances.csv" << endl;
        return 1;
    }

    getline(appFile, line);  // skip the header
    int eplApps = 0;

    while (getline(appFile, line)) {
        vector<string> fields = splitLine(line);
        if (fields.size() < 13) continue;

        // appearances.csv: 2 = player_id, 7 = competition,
        //                  10 = goals, 11 = assists, 12 = minutes
        if (fields[7] == "GB1") {
            eplApps++;
            int id = stoi(fields[2]);

            statsById[id].goals   += stoi(fields[10]);
            statsById[id].assists += stoi(fields[11]);
            statsById[id].minutes += stoi(fields[12]);
            statsById[id].games++;
        }
    }

    cout << "EPL appearances: " << eplApps << endl;
    cout << "Players with EPL stats: " << statsById.size() << endl << endl;

    // ===== TEST: stored players with 50+ EPL goals =====
    for (Player p : players) {
        Stats s = statsById[p.id];
        if (s.goals >= 50) {
            cout << p.name << " | " << s.goals << " G, "
                 << s.assists << " A, " << s.games << " games" << endl;
        }
    }

    return 0;
}