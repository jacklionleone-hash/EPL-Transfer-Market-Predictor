// csv.cpp
#include "csv.h"

vector<string> splitLine(const string& line) {
    vector<string> fields;
    string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];

        if (c == '"') {
            // "" inside a quoted field means a literal quote character
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                i++;
            } else {
                inQuotes = !inQuotes;   // entering or leaving a quoted field
            }
        } else if (c == ',' && !inQuotes) {
            fields.push_back(current);  // real separator → field done
            current = "";
        } else if (c != '\r') {         // ignore Windows line-ending junk
            current += c;
        }
    }
    fields.push_back(current);          // last field (no comma after it)
    return fields;
}
