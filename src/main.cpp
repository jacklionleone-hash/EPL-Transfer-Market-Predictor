#include <iostream>
#include <fstream>
#include <string>
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
        cout << line << endl;
    }
}

    cout << endl << "Total players: " << count << endl;

    return 0;
}