// main.cpp: Premier League player value predictor
//
// Build and run (from the project root):
//   make run
//
// Files:
//   types.h          shared structs (Player, Stats, Sample, Dataset)
//   csv.h/.cpp       splitting CSV lines
//   helpers.h/.cpp   seasons, ages, peak season, names
//   data.h/.cpp      loading the CSVs, team strength, inflation, training set
//   model.h/.cpp     features, normalization, gradient descent, error
//   lookup.h/.cpp    top-20 ranking + interactive player lookup

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include "types.h"
#include "data.h"
#include "model.h"
#include "lookup.h"
using namespace std;

int main() {
    Dataset data;

    // ===== 1. Load the data (appearances FIRST, so we know who played in the EPL) =====
    if (!loadAppearances("data/appearances.csv", data)) return 1;
    if (!loadPlayers("data/players.csv", data)) return 1;
    if (!loadValuations("data/player_valuations.csv", data)) return 1;
    if (!loadGames("data/games.csv", data)) return 1;
    cout << endl;

    // ===== 2. Training data: every season from FIRST to LAST =====
    const int FIRST_TRAIN_SEASON = 2019;   // 2019/20
    const int LAST_TRAIN_SEASON  = 2025;   // 2025/26 = "today"
    vector<Sample> samples = buildTrainingSet(data, FIRST_TRAIN_SEASON, LAST_TRAIN_SEASON);
    cout << "Samples (player-seasons): " << samples.size() << endl << endl;
    if (samples.size() < 20) {
        cerr << "Not enough samples to train." << endl;
        return 1;
    }

    // ===== 3. Split into train (80%) and test (20%) BY PLAYER =====
    // If Salah 2021 were in train and Salah 2022 in test, the model would
    // basically have seen the answer. So each player goes entirely in one side.
    set<int> idSet;
    for (const Sample& s : samples) idSet.insert(s.playerId);
    vector<int> ids(idSet.begin(), idSet.end());

    mt19937 rng(42);                           // fixed seed → same split every run
    shuffle(ids.begin(), ids.end(), rng);
    set<int> testIds(ids.begin(), ids.begin() + ids.size() / 5);

    vector<Sample> trainSet, testSet;
    for (const Sample& s : samples) {
        if (testIds.count(s.playerId)) testSet.push_back(s);
        else trainSet.push_back(s);
    }
    cout << "Train: " << trainSet.size() << " samples | Test: " << testSet.size()
         << " samples (" << testIds.size() << " players the model never sees)" << endl << endl;

    // ===== 4. Compare versions of the model on the TEST set =====
    vector<ModelConfig> versions = {
        { "1. original",          false, false, false },
        { "2. + log(value)",      true,  false, false },
        { "3. + age curve",       true,  true,  false },
        { "4. + team strength",   true,  true,  true  },
    };

    // reference point: always guess the average value
    double avgValue = 0;
    for (const Sample& s : trainSet) avgValue += s.value / 1000000.0 / trainSet.size();
    double guessError = 0;
    for (const Sample& s : testSet) guessError += fabs(avgValue - s.value / 1000000.0) / testSet.size();

    cout << "Average error on players the model has NEVER seen:" << endl;
    cout << "  " << left << setw(22) << "0. always guess avg" << " €" << fixed
         << setprecision(1) << guessError << "M" << endl;
    for (const ModelConfig& cfg : versions) {
        Model m = train(trainSet, cfg, 0.05, 5000);
        cout << "  " << left << setw(22) << cfg.name << " €"
             << averageError(m, testSet) << "M" << endl;
    }
    cout << defaultfloat << setprecision(6) << endl;

    // ===== 5. Final model: best version, trained on ALL samples =====
    Model model = train(samples, versions.back(), 0.05, 5000);
    model.printWeights();

    // ===== 6. Results =====
    printTopOlderPlayers(data, model, 20);
    runLookup(data, model);

    return 0;
}