# EPL Transfer Market Predictor ⚽

**What would prime Luis Suárez cost in today's transfer market?**

Transfer fees have exploded over the last decade, so a player's old market value doesn't tell you what he'd be worth now. This project learns how **today's** market prices Premier League stats, then applies that to older players' best seasons.

Written in C++ with no external libraries: the CSV parser, the stats aggregation, and the machine learning model (linear regression trained with gradient descent) are all implemented from scratch.

## Sample output

```
Top 20: older players' peak seasons, valued in TODAY's market
1. Luis Suárez (2013/14, age 26) 31 G, 17 A → today: €132M | career peak value: €90M
2. Romelu Lukaku (2016/17, age 23) 25 G, 6 A → today: €112M | career peak value: €100M
3. Dele Alli (2016/17, age 20) 18 G, 9 A → today: €111M | career peak value: €100M
...

===== Player lookup =====

Enter a player name (or 'quit'): vardy
Jamie Vardy's EPL seasons:
  2015/16: 24 G, 8 A, ...
  ...
Enter a season (e.g. 2015 or 2015/16): 2015

Jamie Vardy in 2015/16 (age 28): 24 G, 8 A
→ Estimated value in today's market: €92M
```

## How it works

1. **Load the data.** Reads ~1.9M match appearances and ~50K player profiles from the [Transfermarkt dataset](https://www.kaggle.com/datasets/davidcariboo/player-scores), using a quote-aware CSV parser (some fields contain commas inside quotes).
2. **Aggregate stats.** Adds up every player's Premier League goals, assists and minutes, both for their career and **per season** (a season runs Aug–Jun).
3. **Find each player's peak.** The season with the most goals + assists, with at least 1,500 minutes played.
4. **Build a training set.** Active attackers and midfielders from the 2025/26 season, paired with their current market value. These are examples of what today's market pays.
5. **Train the model.** Linear regression:

   value ≈ w₁·goals + w₂·assists + w₃·minutes + w₄·age + b

   Inputs are standardized (z-scores), and the weights are learned with gradient descent.
6. **Value older players.** Plug an older player's peak-season stats and age into the trained model to estimate what he'd cost today.

### What the model learned

| Stat | Effect on value |
|---|---|
| Goals | ≈ +€2.3M per goal |
| Assists | ≈ +€1.9M per assist |
| Minutes | ≈ +€0.9M per 100 minutes |
| Age | ≈ −€4.0M per year older |

## Setup

**Requirements:** a C++17 compiler (`g++` or `clang++`; on a Mac, run `xcode-select --install`).

1. Clone this repo.
2. Download the [Transfermarkt dataset from Kaggle](https://www.kaggle.com/datasets/davidcariboo/player-scores) (free account required).
3. Unzip it and put the CSV files in a `data/` folder in the project root. At minimum you need `appearances.csv` and `players.csv`:
   ```
   EPL-Transfer-Market-Predictor/
   ├── data/
   │   ├── appearances.csv
   │   └── players.csv
   └── src/
   ```
4. Build and run from the project root:
   ```bash
   g++ -std=c++17 -O2 src/*.cpp -o predictor && ./predictor
   ```
   Loading takes a few seconds (it's reading ~1.9M rows).

## Project structure

| File | What's in it |
|---|---|
| `src/main.cpp` | The main steps: load → train → show results |
| `src/types.h` | Shared structs (`Player`, `Stats`, `Sample`, `Dataset`) |
| `src/csv.*` | Quote-aware CSV line splitting |
| `src/data.*` | Loading the CSVs and building the training set |
| `src/helpers.*` | Seasons, ages, peak-season detection |
| `src/model.*` | Normalization + gradient descent |
| `src/lookup.*` | Top-20 ranking and the interactive player lookup |

## Limitations

- **Only goals, assists, minutes and age.** The model can't see dribbling, ball carrying or defending, so it undervalues players like Eden Hazard and doesn't work for defenders or goalkeepers.
- **No team strength.** A striker scoring 18 for a relegation side looks almost as valuable as one scoring 18 for a title winner.
- **Age is modeled as a straight line**, but real value peaks in the mid-20s and drops sharply after 30.
- **Stats only go back to 2012/13**, so "older players" means the 2012–2019 generation.
- **Small training set** (~130 players from one season).

## Next steps

- [ ] Add age² so the model can learn the "peak then decline" curve
- [ ] Add team strength from `club_games.csv`
- [ ] Use per-90-minute stats
- [ ] Hold out some players to measure real accuracy

## Data

Player data comes from [transfermarkt-datasets](https://github.com/dcaribou/transfermarkt-datasets) by David Cariboo (CC0). The data isn't included in this repo because of its size.
