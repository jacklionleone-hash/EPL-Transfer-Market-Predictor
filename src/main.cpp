#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
using namespace std;

int main() {
    ifstream file("data/players.csv");

    if (!file) {
        cerr << "Error: couldn't open data/players.csv" << endl;
        cerr << "(Are you running from the project root?)" << endl;
        return 1;
    }

    string line;
    getline(file, line);  // first line = column names
    cout << "HEADER: " << line << endl << endl;

       int count = 0;

    while (getline(file, line)) {
        count++;

        if (count <= 10) {
            stringstream ss(line);
            string field;
            vector<string> fields;

            while (getline(ss, field, ',')) {
                fields.push_back(field);
            }

            cout << fields[3] << " | " << fields[22] << endl;
        }
    }

    cout << endl << "Total players: " << count << endl;

    return 0;
}