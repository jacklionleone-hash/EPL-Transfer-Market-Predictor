// model.cpp
#include "model.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Age where value tends to peak. Centering here makes (age - PEAK_AGE)² small
// near the peak and big for very young or old players.
const double PEAK_AGE = 26.0;

vector<double> makeFeatures(const ModelConfig& cfg, double goals, double assists,
                            double minutes, double age, double teamPPG) {
    vector<double> x = { goals, assists, minutes, age };
    if (cfg.useAge2) x.push_back((age - PEAK_AGE) * (age - PEAK_AGE));
    if (cfg.useTeam) x.push_back(teamPPG);
    return x;
}

vector<string> featureNames(const ModelConfig& cfg) {
    vector<string> names = { "goals", "assists", "minutes", "age" };
    if (cfg.useAge2) names.push_back("(age-26)^2");
    if (cfg.useTeam) names.push_back("team pts/game");
    return names;
}

static vector<double> featuresOf(const ModelConfig& cfg, const Sample& s) {
    return makeFeatures(cfg, s.goals, s.assists, s.minutes, s.age, s.teamPPG);
}

// Standardize: x' = (x - mean) / std
static vector<double> normalize(const vector<double>& x,
                                const vector<double>& mean, const vector<double>& sd) {
    vector<double> out(x.size());
    for (size_t j = 0; j < x.size(); j++) {
        out[j] = (x[j] - mean[j]) / sd[j];
    }
    return out;
}

// output = w·x + b, with x already normalized
static double linear(const vector<double>& x, const vector<double>& w, double b) {
    double y = b;
    for (size_t j = 0; j < x.size(); j++) {
        y += w[j] * x[j];
    }
    return y;
}

double Model::predictValue(double goals, double assists, double minutes,
                           double age, double teamPPG) const {
    vector<double> raw = makeFeatures(cfg, goals, assists, minutes, age, teamPPG);

    // clamp each input into the range seen in training
    // (e.g. a 100-point team gets treated like the best team the model saw)
    for (size_t j = 0; j < raw.size(); j++) {
        raw[j] = max(minX[j], min(raw[j], maxX[j]));
    }

    double out = linear(normalize(raw, mean, sd), w, b);
    double value = cfg.logTarget ? exp(out) : out;   // undo the log if we used one

    // never predict more than the most valuable player in the training data
    return min(value, maxValue);
}

double Model::predictValue(const Sample& s) const {
    return predictValue(s.goals, s.assists, s.minutes, s.age, s.teamPPG);
}

void Model::printWeights() const {
    vector<string> names = featureNames(cfg);
    cout << "Learned weights for \"" << cfg.name << "\":" << endl;
    for (size_t j = 0; j < w.size(); j++) {
        cout << "  " << names[j] << ": " << w[j]
             << "   (1 std dev = " << sd[j] << ")";
        if (cfg.logTarget) {
            // with log(value), +1 std dev multiplies the value by e^w
            double pct = (exp(w[j]) - 1) * 100;
            cout << "  → +1 std dev changes value by " << (pct >= 0 ? "+" : "") << (int)round(pct) << "%";
        }
        cout << endl;
    }
    cout << "  baseline: " << b
         << (cfg.logTarget ? "  (= €" + to_string((int)round(exp(b))) + "M for an average player)" : "")
         << endl;
    cout << "  predictions capped at €" << (int)round(maxValue)
         << "M (highest value in the training data)" << endl << endl;
}

Model train(const vector<Sample>& training, const ModelConfig& cfg,
            double alpha, int epochs) {
    Model m;
    m.cfg = cfg;
    int n = training.size();
    int nf = featuresOf(cfg, training[0]).size();

    // X = inputs, y = values in € millions (or the log of that)
    vector<vector<double>> X(n);
    vector<double> y(n);
    for (int i = 0; i < n; i++) {
        X[i] = featuresOf(cfg, training[i]);
        double millions = training[i].value / 1000000.0;
        y[i] = cfg.logTarget ? log(millions) : millions;
    }

    // range of each feature, and the highest value, for clamping later
    m.minX = X[0];
    m.maxX = X[0];
    m.maxValue = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < nf; j++) {
            m.minX[j] = min(m.minX[j], X[i][j]);
            m.maxX[j] = max(m.maxX[j], X[i][j]);
        }
        m.maxValue = max(m.maxValue, training[i].value / 1000000.0);
    }

    // mean of each feature: add up, divide by n
    m.mean.assign(nf, 0);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < nf; j++)
            m.mean[j] += X[i][j] / n;

    // std dev of each feature: average squared distance from mean, then sqrt
    m.sd.assign(nf, 0);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < nf; j++)
            m.sd[j] += (X[i][j] - m.mean[j]) * (X[i][j] - m.mean[j]) / n;
    for (int j = 0; j < nf; j++) {
        m.sd[j] = sqrt(m.sd[j]);
        if (m.sd[j] == 0) m.sd[j] = 1;   // avoid dividing by zero
    }

    vector<vector<double>> Xn(n);
    for (int i = 0; i < n; i++) Xn[i] = normalize(X[i], m.mean, m.sd);

    // gradient descent
    m.w.assign(nf, 0);
    m.b = 0;
    for (int epoch = 0; epoch < epochs; epoch++) {
        vector<double> gradW(nf, 0);
        double gradB = 0;

        for (int i = 0; i < n; i++) {
            double err = linear(Xn[i], m.w, m.b) - y[i];   // how wrong for player i
            for (int j = 0; j < nf; j++)
                gradW[j] += 2.0 * err * Xn[i][j] / n;
            gradB += 2.0 * err / n;
        }

        // step AGAINST the gradient (minus!)
        for (int j = 0; j < nf; j++) m.w[j] -= alpha * gradW[j];
        m.b -= alpha * gradB;
    }
    return m;
}

double averageError(const Model& model, const vector<Sample>& samples) {
    if (samples.empty()) return 0;
    double total = 0;
    for (const Sample& s : samples) {
        double predicted = model.predictValue(s);
        double actual = s.value / 1000000.0;
        total += fabs(predicted - actual);
    }
    return total / samples.size();
}