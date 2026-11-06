#include <iostream>
#include <fstream>
#include <string>
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

// Splits one CSV line into fields.
// Commas INSIDE quotes don't split, and the quotes get removed.
vector<string> splitLine(const string& line) {
    vector<string> fields;
    string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];

        if (c == '"') {
            // "" inside a quoted field means a literal quote character
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                i++;
            } else {
                inQuotes = !inQuotes;   // entering or leaving a quoted field
            }
        } else if (c == ',' && !inQuotes) {
            fields.push_back(current);  // real separator → field done
            current = "";
        } else if (c != '\r') {         // ignore Windows line-ending junk
            current += c;
        }
    }
    fields.push_back(current);          // last field (no comma after it)
    return fields;
}

int main() {
    string line;   // shared by both parts

    // ===== PART 1: add up EPL stats from appearances.csv =====
    ifstream appFile("data/appearances.csv");
    if (!appFile) {
        cerr << "Error: couldn't open data/appearances.csv" << endl;
        cerr << "(Are you running from the project root?)" << endl;
        return 1;
    }

    getline(appFile, line);  // skip the header
    int eplApps = 0;
    int skippedApps = 0;

    while (getline(appFile, line)) {
        vector<string> fields = splitLine(line);
        if (fields.size() != 13) { skippedApps++; continue; }

        // appearances.csv: 2 = player_id, 7 = competition,
        //                  10 = goals, 11 = assists, 12 = minutes
        if (fields[7] == "GB1") {
            eplApps++;
            int id = stoi(fields[2]);

            statsById[id].goals   += fields[10].empty() ? 0 : stoi(fields[10]);
            statsById[id].assists += fields[11].empty() ? 0 : stoi(fields[11]);
            statsById[id].minutes += fields[12].empty() ? 0 : stoi(fields[12]);
            statsById[id].games++;
        }
    }

    cout << "EPL appearances: " << eplApps << endl;
    cout << "Players with EPL stats: " << statsById.size() << endl;
    cout << "Skipped appearance rows: " << skippedApps << endl;

    // ===== PART 2: load every player who has played in the EPL =====
    ifstream file("data/players.csv");
    if (!file) {
        cerr << "Error: couldn't open data/players.csv" << endl;
        return 1;
    }

    getline(file, line);  // skip the header

    int count = 0;
    int skippedPlayers = 0;
    vector<Player> players;

    while (getline(file, line)) {
        count++;
        vector<string> fields = splitLine(line);
        if (fields.size() != 26) { skippedPlayers++; continue; }

        // keep the player if they have any EPL appearances
        int id = stoi(fields[0]);
        if (statsById.count(id) > 0) {
            Player p;
            p.id           = id;
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
    cout << "Skipped player rows: " << skippedPlayers << endl;
    cout << "EPL players stored: " << players.size() << endl << endl;

    // ===== TEST: players with 50+ EPL goals =====
    for (Player p : players) {
        Stats s = statsById[p.id];
        if (s.goals >= 50) {
            cout << p.name << " | " << s.goals << " G, "
                 << s.assists << " A, " << s.games << " games" << endl;
        }
    }

    return 0;
}