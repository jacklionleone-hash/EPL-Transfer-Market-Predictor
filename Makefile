# Makefile for the EPL Transfer Market Predictor
#
#   make        → build ./predictor
#   make run    → build (if needed) and run it
#   make clean  → delete the built program

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall
SRC      = $(wildcard src/*.cpp)
HEADERS  = $(wildcard src/*.h)
TARGET   = predictor

# Rebuild only when a .cpp or .h file has changed
$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
