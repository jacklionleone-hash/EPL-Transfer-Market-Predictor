// model.cpp
#include "model.h"
#include <cmath>
#include <iostream>

Features makeFeatures(const Stats& s, int age) {
    return { (double)s.goals, (double)s.assists, (double)s.minutes, (double)age };
}

// Standardize: x' = (x - mean) / std
static Features normalize(const Features& x, const Features& mean, const Features& sd) {
    Features out;
    for (int j = 0; j < NF; j++) {
        out[j] = (x[j] - mean[j]) / sd[j];
    }
    return out;
}

// value = w·x + b, with x already normalized
static double linear(const Features& x, const Features& w, double b) {
    double y = b;
    for (int j = 0; j < NF; j++) {
        y += w[j] * x[j];
    }
    return y;
}

double Model::predictValue(const Stats& s, int age) const {
    Features x = normalize(makeFeatures(s, age), mean, sd);
    return linear(x, w, b);
}

void Model::printWeights() const {
    string names[NF] = { "goals", "assists", "minutes", "age" };
    cout << endl << "Learned weights (€M per 1 std dev):" << endl;
    for (int j = 0; j < NF; j++) {
        cout << "  " << names[j] << ": " << w[j]
             << "   (1 std dev = " << sd[j] << ")" << endl;
    }
    cout << "  baseline: " << b << endl << endl;
}

Model train(const vector<Sample>& training, double alpha, int epochs) {
    Model m;
    int n = training.size();

    // X = inputs, y = values in € millions
    vector<Features> X(n);
    vector<double> y(n);
    for (int i = 0; i < n; i++) {
        X[i] = { training[i].goals, training[i].assists,
                 training[i].minutes, training[i].age };
        y[i] = training[i].value / 1000000.0;
    }

    // mean of each feature: add up, divide by n
    for (int i = 0; i < n; i++)
        for (int j = 0; j < NF; j++)
            m.mean[j] += X[i][j] / n;

    // std dev of each feature: average squared distance from mean, then sqrt
    for (int i = 0; i < n; i++)
        for (int j = 0; j < NF; j++)
            m.sd[j] += (X[i][j] - m.mean[j]) * (X[i][j] - m.mean[j]) / n;
    for (int j = 0; j < NF; j++) {
        m.sd[j] = sqrt(m.sd[j]);
        if (m.sd[j] == 0) m.sd[j] = 1;   // avoid dividing by zero
    }

    vector<Features> Xn(n);
    for (int i = 0; i < n; i++) Xn[i] = normalize(X[i], m.mean, m.sd);

    // gradient descent
    for (int epoch = 0; epoch <= epochs; epoch++) {
        Features gradW{};
        double gradB = 0;
        double loss = 0;

        for (int i = 0; i < n; i++) {
            double err = linear(Xn[i], m.w, m.b) - y[i];   // how wrong for player i
            loss += err * err / n;
            for (int j = 0; j < NF; j++)
                gradW[j] += 2.0 * err * Xn[i][j] / n;
            gradB += 2.0 * err / n;
        }

        // step AGAINST the gradient (minus!)
        for (int j = 0; j < NF; j++) m.w[j] -= alpha * gradW[j];
        m.b -= alpha * gradB;

        if (epoch % 1000 == 0) {
            cout << "epoch " << epoch << " | avg error: €"
                 << sqrt(loss) << "M" << endl;
        }
    }
    return m;
}
