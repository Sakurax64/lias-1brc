# lias-1brc
## Rules and limits
The ruleset is similar to the [official 1brc ruleset](https://1brc.dev/#rules-and-limits) but with a few minor tweaks.
- No external library dependencies may be used. You're limited to the standard libraries of your language.
- Don't copy-paste a library into your solution as a cheat.
- The computation must happen at application runtime; you cannot process the measurements file at build time
- Input value ranges are as follows:
  - Station name: a non-null string containing the English name of a city (i.e. Cologne, Brussels, Zurich). Station names consist only of ASCII letters (a-z, A-Z), spaces, and hyphens.
  - Temperature value: non null number between -99.9 (inclusive) and 99.9 (inclusive), always with one fractional digit
- There is a maximum of 10,000 unique station names.
- Implementations must not rely on specifics of a given data set. Any valid station name as per the constraints above and any data distribution (number of measurements per station) must be supported.
- Your implementation must use a pre-generated dataset, not generate it itself.
- The output format must be `<station>=<min>/<max>/<avg>`

The end of the challenge will depend on how many people I can actually get to join this (lol).

## Entering the challenge
1. Clone the repo and run the generation script (coming soon).
2. Create your submission in `/submissions/<your-submission-name>/`. If your submission contains multiple files or needs something special (i.e. a specific runtime version), please include a short readme.
3. Open a PR.
4. Wait for me to run your submission and put it on the leaderboard.

## Benchmarking
- The benchmarks will be run on the latest Arch Linux release, on an i7 8700k and 32GB DDR4 RAM.
  - If I upgrade my hardware before the challenge ends, I will disclose it and rerun all benchmarks on the new hardware.
- The benchmark result for each submission will be the average execution time of 3 runs.
- Compilation/build time is not included in the benchmark.
