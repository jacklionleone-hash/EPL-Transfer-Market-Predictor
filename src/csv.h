// csv.h: reading CSV lines
#pragma once

#include <string>
#include <vector>
using namespace std;

// Splits one CSV line into fields.
// Commas INSIDE quotes don't split, and the quotes get removed.
vector<string> splitLine(const string& line);
