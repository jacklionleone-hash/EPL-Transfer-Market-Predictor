# Premier League Player Value Predictor (C++)

## What this is
Personal side project: predict Premier League players' market values
from their stats using linear regression + gradient descent, written
from scratch in C++ (no ML libraries).

## Data
Kaggle "Football Data from Transfermarkt" (davidcariboo), CSVs in data/:
- players.csv: profiles, position, market value (target y)
- appearances.csv: per-game goals/assists/minutes (features x); ~1.9M rows,
  read line by line and aggregate per player_id
- player_valuations.csv, transfers.csv, clubs.csv, competitions.csv
- Premier League competition_id = GB1
- data/ is gitignored (files are too big for GitHub)

## Build & run
From the project root:
g++ -std=c++17 src/main.cpp -o predictor && ./predictor

## Plan
1. Read players.csv, print first 10 rows + row count
2. Player struct + vector<Player>
3. Aggregate EPL stats from appearances.csv
4. Simple 1-feature regression, then multi-feature (normalize first)
5. Test on held-out players

## How to help me
- I'm learning C++ (CS16). Give hints and point me in the right
  direction, and don't write the code for me unless I ask.
- Don't over-explain basics; I learn fast.
