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

    // 2. Training set: active players' 2025/26 season + today's value
    const int TRAIN_SEASON = 2025;
    vector<Sample> training = buildTrainingSet(data, TRAIN_SEASON);
    cout << "Training samples: " << training.size() << endl << endl;
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
