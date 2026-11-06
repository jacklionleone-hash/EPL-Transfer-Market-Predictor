// model.h: the linear regression model, its training, and testing
#pragma once

#include <string>
#include <vector>
#include "types.h"
using namespace std;

// Which upgrades a model uses, so we can compare versions side by side
struct ModelConfig {
    string name;
    bool logTarget;   // predict log(value) instead of value → stats add a % instead of a fixed €
    bool useAge2;     // add (age - 26)² → lets value peak mid-20s and drop after
    bool useTeam;     // add team points per game → a goal for a top team counts differently
};

// The inputs for one season, in the order the model uses them.
// Always: goals, assists, minutes, age. Then (age-26)² and team PPG if turned on.
vector<double> makeFeatures(const ModelConfig& cfg, double goals, double assists,
                            double minutes, double age, double teamPPG);

// Names of the features above, for printing
vector<string> featureNames(const ModelConfig& cfg);

// A trained model. Each input is standardized with the TRAINING set's mean
// and std dev, then: output = w1*x1 + w2*x2 + ... + b
// (output = value in €M, or log(value in €M) if cfg.logTarget)
struct Model {
    ModelConfig cfg;
    vector<double> mean, sd, w;
    double b = 0;

    // Range of each input and of the value in the training data.
    // Inputs get clamped into this range and predictions are capped at maxValue,
    // so the model never extrapolates past what it actually learned from.
    vector<double> minX, maxX;
    double maxValue = 0;   // € millions

    // Estimated value in € millions
    double predictValue(double goals, double assists, double minutes,
                        double age, double teamPPG) const;
    double predictValue(const Sample& s) const;

    // Prints what each weight means
    void printWeights() const;
};

// Normalizes the samples, then runs gradient descent
Model train(const vector<Sample>& training, const ModelConfig& cfg,
            double alpha, int epochs);

// Average error in € millions: mean of |predicted - actual| over the samples
double averageError(const Model& model, const vector<Sample>& samples);