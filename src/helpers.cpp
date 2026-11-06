// helpers.cpp
#include "helpers.h"
#include <cctype>

int findPeakSeason(const SeasonMap& seasons) {
    int bestSeason = -1;
    int bestGA = -1;

    for (auto entry : seasons) {
        int season = entry.first;
        Stats s = entry.second;

        if (s.minutes < 1500) continue;     // skip short seasons
        int ga = s.goals + s.assists;

        if (ga > bestGA) {                  // keep the best so far
            bestGA = ga;
            bestSeason = season;
        }
    }
    return bestSeason;
}

string seasonLabel(int season) {
    return to_string(season) + "/" + to_string((season + 1) % 100);
}

int seasonFromDate(const string& date) {
    int year  = stoi(date.substr(0, 4));
    int month = stoi(date.substr(5, 2));
    return (month >= 7) ? year : year - 1;
}

int ageAtSeason(const string& birthDate, int season) {
    if (birthDate.empty()) return -1;               // guard: no birth date
    int birthYear = stoi(birthDate.substr(0, 4));   // "1991-01-07 ..." → 1991
    return season - birthYear;
}

string toLower(string s) {
    for (char& c : s) c = tolower((unsigned char)c);
    return s;
}

bool isAttackOrMidfield(const Player& p) {
    return p.position == "Attack" || p.position == "Midfield";
}