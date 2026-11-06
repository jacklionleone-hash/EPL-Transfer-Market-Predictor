// model.h: the linear regression model and its training
#pragma once

#include <array>
#include <string>
#include <vector>
#include "types.h"
using namespace std;

// Number of input features: goals, assists, minutes, age
const int NF = 4;
using Features = array<double, NF>;

// Turns one season of stats + age into the model's input list
Features makeFeatures(const Stats& s, int age);

// A trained model: value (€M) = w1*x1 + ... + w4*x4 + b,
// where each x is standardized with the TRAINING set's mean and std dev
struct Model {
    Features mean{};
    Features sd{};
    Features w{};
    double b = 0;

    // Estimated value in € millions for one season + age
    double predictValue(const Stats& s, int age) const;

    // Prints what each weight means
    void printWeights() const;
};

// Normalizes the samples, then runs gradient descent
Model train(const vector<Sample>& training, double alpha, int epochs);
