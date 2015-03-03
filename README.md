# bidding_game
The Bidding Game, a game for 4 players, using C POSIX, to interract between 4 threads. Server sets up an initial balance and every player bids without knowledge of the others' bids. Every round ends with a winner getting closer to the prize. Terrain consists of a 9x9 array with prize being in arr[4,4](middle). Ties are resolved by an advantage system.
