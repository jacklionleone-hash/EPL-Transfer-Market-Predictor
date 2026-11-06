// main.cpp: Premier League player value predictor
//
// Build (from the project root):
//   g++ -std=c++17 -O2 src/*.cpp -o predictor && ./predictor
//
// Files:
//   types.h          shared structs (Player, Stats, Sample, Dataset)
//   csv.h/.cpp       splitting CSV lines
//   helpers.h/.cpp   seasons, ages, peak season, names
//   data.h/.cpp      loading the CSVs + building the training set
//   model.h/.cpp     normalization + gradient descent
//   lookup.h/.cpp    top-20 ranking + interactive player lookup

#include <iostream>
#include "types.h"
#include "data.h"
#include "model.h"
#include "lookup.h"
using namespace std;

int main() {
    Dataset data;

    // 1. Load the data (appearances FIRST, so we know who played in the EPL)
    if (!loadAppearances("data/appearances.csv", data)) return 1;
    if (!loadPlayers("data/players.csv", data)) return 1;
    if (!loadValuations("data/player_valuations.csv", data)) return 1;

    // 2. Training set: every season from FIRST to LAST, with each player's
    //    end-of-season value converted to LAST season's money
    const int FIRST_TRAIN_SEASON = 2019;   // 2019/20
    const int LAST_TRAIN_SEASON  = 2025;   // 2025/26 = "today"
    vector<Sample> training = buildTrainingSet(data, FIRST_TRAIN_SEASON, LAST_TRAIN_SEASON);
    cout << "Training samples: " << training.size() << endl;
    for (int season = FIRST_TRAIN_SEASON; season <= LAST_TRAIN_SEASON; season++) {
        int n = 0;
        for (const Sample& sm : training) if (sm.season == season) n++;
        cout << "  " << season << "/" << (season + 1) % 100 << ": " << n << endl;
    }
    cout << endl;
    if (training.empty()) {
        cerr << "No training samples, can't train." << endl;
        return 1;
    }

    // 3. Train the model
    Model model = train(training, 0.05, 5000);
    model.printWeights();

    // 4. Results
    printTopOlderPlayers(data, model, 20);
    runLookup(data, model);

    return 0;
}