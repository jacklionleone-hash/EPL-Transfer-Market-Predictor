#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
using namespace std;


//gets rid of the quotations
string stripQuotes(string s) {
    if (s.size() >= 2 && s[0] == '"' && s[s.size() - 1] == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}




int main() {
    
    //wrong / no data
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
                fields.push_back(stripQuotes(field));
            }

            cout << fields[3] << " | " << fields[22] << endl;
        }
    }

    cout << endl << "Total players: " << count << endl;

    return 0;
}