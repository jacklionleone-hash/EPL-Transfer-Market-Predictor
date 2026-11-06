#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <array>
#include <cmath>
#include <algorithm>
using namespace std;

// Number of input features the model uses: goals, assists, minutes, age
const int NF = 4;
using Features = array<double, NF>;

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

// One training example: a player's stats in a season + their value
struct Sample {
    string name;
    double goals, assists, minutes, age;   // inputs (x's)
    double value;                          // what we predict (y)
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

// Age at the middle of a season (around Jan 1).
// Returns -1 if birth date is missing.
int ageAtSeason(string birthDate, int season) {
    if (birthDate.empty()) return -1;          // guard: no birth date
    int birthYear = stoi(birthDate.substr(0, 4));
    return season - birthYear;
}

// Turns one season of stats + age into the model's input list
Features makeFeatures(const Stats& s, int age) {
    return { (double)s.goals, (double)s.assists, (double)s.minutes, (double)age };
}

// Standardize: x' = (x - mean) / std, using the TRAINING set's mean and std
Features normalize(const Features& x, const Features& mean, const Features& sd) {
    Features out;
    for (int j = 0; j < NF; j++) {
        out[j] = (x[j] - mean[j]) / sd[j];
    }
    return out;
}

// The model: value = w1*x1 + w2*x2 + w3*x3 + w4*x4 + b  (in € millions)
double predict(const Features& x, const Features& w, double b) {
    double y = b;
    for (int j = 0; j < NF; j++) {
        y += w[j] * x[j];
    }
    return y;
}

int main() {
    string line;   // shared by all parts

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

    // ===== PART 3: training set = active players + their 2025/26 season =====
    const int TRAIN_SEASON = 2025;
    vector<Sample> training;

    for (Player p : players) {
        // still active, with a real current value
        if (p.lastSeason < TRAIN_SEASON) continue;
        if (p.marketValue <= 0) continue;

        // attackers and midfielders only (for now)
        if (p.position != "Attack" && p.position != "Midfield") continue;

        // did they play a 2025/26 EPL season? (.count so we don't create an empty one)
        if (statsBySeason[p.id].count(TRAIN_SEASON) == 0) continue;
        Stats s = statsBySeason[p.id][TRAIN_SEASON];

        // real season only
        if (s.minutes < 1500) continue;

        int age = ageAtSeason(p.birthDate, TRAIN_SEASON);
        if (age == -1) continue;              // missing birth date

        Sample sm;
        sm.name    = p.name;
        sm.goals   = s.goals;
        sm.assists = s.assists;
        sm.minutes = s.minutes;
        sm.age     = age;
        sm.value   = p.marketValue;           // today's price
        training.push_back(sm);
    }

    cout << "Training samples: " << training.size() << endl << endl;
    int n = training.size();
    if (n == 0) {
        cerr << "No training samples, can't train." << endl;
        return 1;
    }

    // ===== PART 4: normalize the training data =====
    // X = inputs, y = values in € millions
    vector<Features> X(n);
    vector<double> y(n);
    for (int i = 0; i < n; i++) {
        X[i] = { training[i].goals, training[i].assists,
                 training[i].minutes, training[i].age };
        y[i] = training[i].value / 1000000.0;
    }

    // mean of each feature: add up, divide by n
    Features mean{};
    for (int i = 0; i < n; i++)
        for (int j = 0; j < NF; j++)
            mean[j] += X[i][j] / n;

    // std dev of each feature: average squared distance from mean, then sqrt
    Features sd{};
    for (int i = 0; i < n; i++)
        for (int j = 0; j < NF; j++)
            sd[j] += (X[i][j] - mean[j]) * (X[i][j] - mean[j]) / n;
    for (int j = 0; j < NF; j++) {
        sd[j] = sqrt(sd[j]);
        if (sd[j] == 0) sd[j] = 1;   // avoid dividing by zero
    }

    vector<Features> Xn(n);
    for (int i = 0; i < n; i++) Xn[i] = normalize(X[i], mean, sd);

    // ===== PART 5: train with gradient descent =====
    Features w{};          // all weights start at 0
    double b = 0;
    double alpha = 0.05;   // learning rate (step size)
    int epochs = 5000;     // how many times to loop over all players

    for (int epoch = 0; epoch <= epochs; epoch++) {
        Features gradW{};
        double gradB = 0;
        double loss = 0;

        for (int i = 0; i < n; i++) {
            double err = predict(Xn[i], w, b) - y[i];   // how wrong for player i
            loss += err * err / n;
            for (int j = 0; j < NF; j++)
                gradW[j] += 2.0 * err * Xn[i][j] / n;
            gradB += 2.0 * err / n;
        }

        // step AGAINST the gradient (minus!)
        for (int j = 0; j < NF; j++) w[j] -= alpha * gradW[j];
        b -= alpha * gradB;

        if (epoch % 1000 == 0) {
            cout << "epoch " << epoch << " | avg error: €"
                 << sqrt(loss) << "M" << endl;
        }
    }

    // What the model learned (per 1 standard deviation of each stat)
    string names[NF] = { "goals", "assists", "minutes", "age" };
    cout << endl << "Learned weights (€M per 1 std dev):" << endl;
    for (int j = 0; j < NF; j++) {
        cout << "  " << names[j] << ": " << w[j]
             << "   (1 std dev = " << sd[j] << ")" << endl;
    }
    cout << "  baseline: " << b << endl << endl;

    // ===== PART 6: value older players' peak seasons in today's market =====
    struct Result {
        string name, season;
        Stats s;
        int age;
        double todayValue, peakValueThen;
    };
    vector<Result> results;

    for (Player p : players) {
        if (p.position != "Attack" && p.position != "Midfield") continue;

        int peak = findPeakSeason(p.id);
        if (peak == -1 || peak > 2018) continue;   // "older" = peak 2012/13 – 2018/19

        int age = ageAtSeason(p.birthDate, peak);
        if (age == -1) continue;

        Stats s = statsBySeason[p.id][peak];
        Features x = normalize(makeFeatures(s, age), mean, sd);

        Result r;
        r.name = p.name;
        r.season = seasonLabel(peak);
        r.s = s;
        r.age = age;
        r.todayValue = predict(x, w, b);
        r.peakValueThen = p.highestValue / 1000000.0;
        results.push_back(r);
    }

    // sort highest predicted value first
    sort(results.begin(), results.end(), [](const Result& a, const Result& b) {
        return a.todayValue > b.todayValue;
    });

    cout << "Top 20: older players' peak seasons, valued in TODAY's market" << endl;
    for (int i = 0; i < 20 && i < (int)results.size(); i++) {
        Result r = results[i];
        cout << i + 1 << ". " << r.name << " (" << r.season << ", age " << r.age << ") "
             << r.s.goals << " G, " << r.s.assists << " A"
             << " → today: €" << (int)r.todayValue << "M"
             << " | career peak value: €" << (int)r.peakValueThen << "M" << endl;
    }

    return 0;
}