# Bidding Game - POSIX (C)

>The Bidding Game, a game consisted of (4) clients and (1) server, using C POSIX, to interract between 4 threads. 

#### Server initially sets up an ip,port and balance for the game to run. 
```
./server 127.0.0.1 9999 1000
```

#### Every client connects to this particular ip address:port and when (4) clients connect, the game starts. 
```
./client 127.0.0.1:9999
```

### Explanation
* _Everyone starts with the balance set by the server and takes their place on the terrain. Terrain consists of an 9x9 array with every player settled on every corner. The so called desired object/prize is located in the middle of the terrain, or terrain[4,4]._
* _Every player makes a bid on every round without any knowledge of their opponents' choices or strategy._
* _When everyone finishes bidding, a round ends and a winner for this rounds is announced. This player comes one step closer to the prize until someone actually matches the prize's position._
* _Ties are being held by an advantage point, which changes on every round consecutively for every player._
* _Player who's closer to the advantage point **wins** the tie._

