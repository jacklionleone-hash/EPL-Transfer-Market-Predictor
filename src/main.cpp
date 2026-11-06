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

map<int, Stats> statsById;                 // player_id → career EPL totals
map<int, map<int, Stats>> statsBySeason;   // player_id → (season → Stats)

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

// Returns the season with the most goals + assists (min 1500 minutes).
// Returns -1 if the player never had a full-enough season.
int findPeakSeason(int id) {
    int bestSeason = -1;
    int bestGA = -1;

    for (auto entry : statsBySeason[id]) {
        int season = entry.first;
        Stats s = entry.second;

        if (s.minutes < 1500) continue;
        int ga = s.goals + s.assists;

        if (ga > bestGA) {
            bestGA = ga;
            bestSeason = season;
        }
    }
    return bestSeason;
}


// Turns 2018 into "2018/19"
string seasonLabel(int season) {
    return to_string(season) + "/" + to_string((season + 1) % 100);
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

        // appearances.csv: 2 = player_id, 5 = date, 7 = competition,
        //                  10 = goals, 11 = assists, 12 = minutes
        if (fields[7] == "GB1") {
            eplApps++;
            int id = stoi(fields[2]);

            int goals   = fields[10].empty() ? 0 : stoi(fields[10]);
            int assists = fields[11].empty() ? 0 : stoi(fields[11]);
            int minutes = fields[12].empty() ? 0 : stoi(fields[12]);

            // career totals
            statsById[id].goals   += goals;
            statsById[id].assists += assists;
            statsById[id].minutes += minutes;
            statsById[id].games++;

            // which season? date looks like "2015-08-08"
            // Aug–Dec 2015 and Jan–Jun 2016 both count as season 2015
            string date = fields[5];
            int year   = stoi(date.substr(0, 4));
            int month  = stoi(date.substr(5, 2));
            int season = (month >= 7) ? year : year - 1;

            // per-season totals
            statsBySeason[id][season].goals   += goals;
            statsBySeason[id][season].assists += assists;
            statsBySeason[id][season].minutes += minutes;
            statsBySeason[id][season].games++;
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

    // ===== TEST: Eden Hazard (id 50202) season by season =====
    cout << "Eden Hazard by season:" << endl;
    for (auto entry : statsBySeason[50202]) {
        int season = entry.first;
        Stats s = entry.second;
        cout << season << "/" << (season + 1) % 100 << ": "
             << s.goals << " G, " << s.assists << " A, "
             << s.games << " games, " << s.minutes << " min" << endl;
    }
    cout << "Hazard's peak: " << seasonLabel(findPeakSeason(50202)) << endl;
    return 0;
}