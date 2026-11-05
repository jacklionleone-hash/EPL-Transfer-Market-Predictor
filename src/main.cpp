#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
using namespace std;

// Removes the " at the start and end of a field, if both are there
string stripQuotes(string s) {
    if (s.size() >= 2 && s[0] == '"' && s[s.size() - 1] == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

int main() {
    // open the players file
    ifstream file("data/players.csv");

    if (!file) {
        cerr << "Error: couldn't open data/players.csv" << endl;
        cerr << "(Are you running from the project root?)" << endl;
        return 1;
    }

    string line;
    getline(file, line);  // skip the header (column names)

    int count = 0;
    int eplCount = 0;

    while (getline(file, line)) {
        count++;

        // split the line into fields
        stringstream ss(line);
        string field;
        vector<string> fields;

        while (getline(ss, field, ',')) {
            fields.push_back(stripQuotes(field));
        }

        // skip broken rows so fields[22] can't crash
        if (fields.size() < 26) continue;

        // column 22 = league; GB1 = Premier League
        if (fields[22] == "GB1") {
            eplCount++;

            // print only the first 10 EPL players
            if (eplCount <= 10) {
                cout << fields[3] << " | " << fields[23] << endl;
            }
        }
    }

    cout << endl << "Total players: " << count << endl;
    cout << "EPL players: " << eplCount << endl;

    return 0;
}