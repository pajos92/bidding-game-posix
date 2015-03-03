/**
*
* @author Panos Matsaridis
*
*/

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_HANDLER_THREADS 5

#ifndef max
	#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

void* thread_handler(void*);

/*  Player number that has been accepted 
	by the server,after sent "HELLO name"*/
	int 	activePlayers = 0;
	char 	activePlayers_str[2];
	int 	player_thread[4];

/*  State of game
	0 : Server is on "hello" waiting mode
	1 : Server has started the game and accepting bids
	2 : Server found a winner - terminating game
*/	
	int gamestate = 0;

	
/* Game Related Variables Start*/

	/* Starting number of balance for all players stored in argument 2 */
	int 	startingGold;
	char 	startingGoldStr[5];
	/* Array of the total balance of all players */
	int 	balance[4];
	char 	balance_str[4][5];	
	/* The current bids of the players */
	int 	curr_bid[4];
	char	curr_bid_str[4][5];
	
	/* Keep track off all the bids of the game 
		Max. 100 rounds arbitrarily */
	int 	game_bids[100][4];
	/* Counter for rounds played */
	int 	round_num = 1;

	/*
		Player with advantage in every round 
		is in position arr[3] of this array(last position).
		When round ends array will shift right 
		e.g [1,4,3,2] on round 2, 
		so the advantage goes to player 2.
		When we face tie, system will look up 
		at the positions of those who tie.
		Player as near to position 3
		(or position with the bigger number in here) 
		will win the advantage.
	*/
	int 	advantage_array[4] = {4,3,2,1};
	
	/* Number of player who has the advantage
		to send to the clients */
	int		player_advantage;	
	char 	player_advantage_str[2];
	
	/* Flag to check that all players have completed bidding */
	int 	round_completed_bids[4]; 
	/* The winner of a specific round */
	int 	round_winner;
	char	round_winner_str[2];
	/* The winner of the game */
	int 	total_winner = 0;
	char  	total_winner_str[2];
	/* The valuable item position in a 9x9 array */
	int 	item_posx = 4, item_posy = 4;
	char	item_posx_str[2], item_posy_str[2];
	
/* Game Related Variables End*/



/* 
	Function that compares strings 
	param 1 : text that is being searched
	param 2 : text to search inside param 1 
*/
int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

/*	
	Finds the bid in the string buffer that is received from the client
	param 1 : buffer to get the bid from
	param 2 : total player balance for checking reasons
	param 3 : actual player who owns the balance
*/
int checkBid(char str[], int player_balance , int player){

	int t=0;
	const char s[2] = " ";
	char *token;
	char *tokens_array[2];
	int bid = 0;
			   
	/* get the first token */
	token = strtok(str, s);
			   
	/* walk through other tokens */
	while( token != NULL ) 
	{
		tokens_array[t] = token;
		token = strtok(NULL, s);
		t++;
	}		
	
	bid = atoi(tokens_array[1]);
	curr_bid[player] = bid;
	sprintf(curr_bid_str[player], "%d", curr_bid[player]);
	
	/* Keep the track off all rounds' bids to save to disk in the future */
	game_bids[round_num - 1][player] = bid;
	
	/* Negative values would actually raise total balance
		so lets protect the system from smart@$$ behaviour */
	if(bid < 0)
		return 0;
		
	/*	
		Check if player has the necessary balance
		if so, subtract it from the total balance 
		and return true.
	*/
	if(bid <= player_balance)
	{
		balance[player] -= bid;
		sprintf(balance_str[player], "%d", balance[player]);
		return 1;
	}
		
	return 0;
}

/*
	Function that moves the position of the item depending on the max bid
	Also handles the round_winner and total_winner if there is one
	params : all the 4 bids of the players
	Returns : total_winner , 0 by default if there isn't one
*/
int grid_handler () 
{
	/* array that stores the players ids in case of a tie */
	int players_tie[4];
	
	/* returns the max bid value of all bids */
	int max_bid = max( max(curr_bid[0],curr_bid[1]), max(curr_bid[2],curr_bid[3]) );
	
	/* max flag to use in advantage_array */
	int max_advantage_pos;
	
	/* how many players have the same max bid? */
	int tie_number = 0;
	
	int i=0,j=0;
	
	/* Count how many and who have bided the same amount 
		to compare the advantages of */
	for (i=0;i<activePlayers;i++){
		if(curr_bid[i] == max_bid){
			players_tie[tie_number] = i+1;
			tie_number++;
		}
	}
	/* Case we have only 1 high bider 
		then this is the round winner */
	if(tie_number == 1){
		round_winner = players_tie[0];
	}
	/* If not, get the max position of biders 
		in the advantage_array. 
		Higher position means closer to the advantage. */
	else {
		max_advantage_pos = -1;	
		for(i=0;i<tie_number;i++){
			for(j=0;j<activePlayers;j++){
				if(players_tie[i] == advantage_array[j]){
					if( j > max_advantage_pos ){
						max_advantage_pos = j;
					}
				}
			}
		}
		round_winner = advantage_array[max_advantage_pos];
	}
	
	/* Move the valuable item 
		depending on the round winner */
	switch(round_winner){
		case 1:
			if(item_posy != 0)
				item_posy--;
			else
				item_posx--;
			break;
		case 2:
			if(item_posy != 8)
				item_posy++;
			else
				item_posx--;
			break;
		case 3:
			if(item_posy != 0)
				item_posy--;
			else
				item_posx++;
			break;
		case 4:
			if(item_posy != 8)
				item_posy++;
			else
				item_posx++;
			break;
		default:
			printf("Max bid is not one of the current player bids?.\n");
			fflush(NULL);
	}

	/* Check for a total winner */
	if (item_posx == 0 && item_posy == 0){
		total_winner = 1;
		sprintf(total_winner_str, "%d" , total_winner);
	}
	if (item_posx == 0 && item_posy == 8){
		total_winner = 2;
		sprintf(total_winner_str, "%d" , total_winner);
	}
	if (item_posx == 8 && item_posy == 0){
		total_winner = 3;
		sprintf(total_winner_str, "%d" , total_winner);
	}
	if (item_posx == 8 && item_posy == 8){
		total_winner = 4;
		sprintf(total_winner_str, "%d" , total_winner);
	}
	return total_winner;
	
}

/* Function to save the date to .txt
	When game ends it is called to 
	full the scores.txt file
	with whole game session bids. */
int saveToFile()
{
	FILE *f = fopen("scores.txt", "w");
	int i;
	
	if (f == NULL)
	{
		printf("Error opening file!\n");
		return 1;
	}
	
	fprintf(f, "======GAME BID TRACK======\n");
	for(i=0;i<round_num;i++){
		fprintf(f, "(ROUND %d) \t P1 : %d \t P2 : %d \t P3 : %d \t P4 : %d\n", i+1, game_bids[i][0], game_bids[i][1], game_bids[i][2], game_bids[i][3]);
	}
	fprintf(f, "TOTAL WINNER : PLAYER %d\n",total_winner);
	fprintf(f, "====== END OF GAME ======\n");

	fclose(f);
	return 0;
}

int main(int argc, char *argv[])
{
    int host_port;
    struct sockaddr_in my_addr;
    int hsock;
    int * p_int ;
    int err;
    socklen_t addr_size = 0;
    int* cs;
    struct sockaddr_in sadr;
    pthread_t thread_id=0;
	int i;                                
	
	if (argc != 3)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Incorrect usage!Try: %s <server_port> <starting_gold>\n", argv[0]);
        exit(1);
    }

	host_port = atoi(argv[1]);  	/* First arg:  local port */
	startingGold = atoi(argv[2]); 	/*Second arg: starting gold */
	
	/* Initialize some player variables */
	for(i=0; i<4; i++) {
		balance[i] = startingGold;
		round_completed_bids[i] = 0;
	}
	
	player_advantage = 1;
	sprintf(player_advantage_str, "%d", player_advantage);
	
	round_winner = 1;
	sprintf(round_winner_str, "%d", round_winner);
	
    hsock = socket(AF_INET, SOCK_STREAM, 0);
    p_int = (int*)malloc(sizeof(int));
    *p_int = 1;

    setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int));
    setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int));
    free(p_int);

    my_addr.sin_family = AF_INET ;
    my_addr.sin_port = htons(host_port);
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY ;

    bind( hsock, (struct sockaddr *)&my_addr, sizeof(my_addr));
    listen( hsock, 10);
    addr_size = sizeof( struct sockaddr_in);
	printf("Server started and listening on port %d\nStarting balance has been set to %d\n",host_port,startingGold);
	fflush(NULL);
		
	while(1){
	
		cs = (int*)malloc(sizeof(int));
		if((*cs = accept( hsock, (struct sockaddr*)&sadr, &addr_size))!= -1)
		{	
			pthread_create(&thread_id,0,&thread_handler, (void*)cs );
			pthread_detach(thread_id);
		}
		else
		{
			fprintf(stderr, "Error accepting %d\n", errno);
		}
	}
}

void* thread_handler(void* lp){

    int *csock = (int*)lp;
    char buffer[80];
    int buffer_len = 80 ;
    int bytecount;
	int i;
	int sum_round = 0;
	int temp;
	
	/* Packets for comparing/sending reasons */
	char hello_packet[6] = "HELLO";
	char bid_packet[4] = "BID";
	char ok_bid[6] = "OKBID\0";
	char invalid_bid[11] = "INVALIDBID\0";

	printf("Starting new thread with id %d\n", *csock);
    fflush(NULL);
	
	
	while ((bytecount = recv(*csock, buffer, buffer_len, 0)) > 0) 
	{
		/* Case a HELLO packet received */
		if(startsWith(hello_packet,buffer) && gamestate == 0)
		{
			/* Save the thread socket in an array for future use */
			player_thread[activePlayers] = *csock;
			activePlayers++;
			printf("Hello received from thread %d\n", *csock);
			fflush(NULL);
			
			/* Set buffer to 0 and send the WELCOME packet */
			memset(buffer, 0, buffer_len);	
			strcat(buffer, "WELCOME ");
			sprintf(activePlayers_str, "%d", activePlayers);
			strcat(buffer, activePlayers_str);
			strcat(buffer, "\0");
			bytecount = send(*csock, buffer, buffer_len, 0);
			printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, *csock);
			fflush(NULL);
			memset(buffer, 0, buffer_len);
			
			/* If we reach 4 players proceed to game state 1 */
			if(activePlayers == 4)
			{
				gamestate++;
				printf("Active Players : %d\n",activePlayers);
				fflush(NULL);
				printf("Game State : %d\n",gamestate);
				fflush(NULL);
				
				/* Send the STARTINGGAME to all threads */
				memset(buffer, 0, buffer_len);	
				strcat(buffer, "STARTINGGAME ");
				sprintf(startingGoldStr, "%d", startingGold);
				strcat(buffer, startingGoldStr);
				strcat(buffer, "\0");
				for(i=0;i<activePlayers;i++)
				{
					bytecount = send(player_thread[i], buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, player_thread[i]);
					fflush(NULL);					
				}
				memset(buffer, 0, buffer_len);
		
			}

		}
		/* Case a BID packet received */
		else if(startsWith(bid_packet,buffer) && gamestate == 1)
		{
			/* Depending on thread id, check the bid 
				then send OKBID or INVALIDBID
				case OKBID: flag completed bid */
			if( *csock == player_thread[0]) 
			{			
				if(checkBid( buffer, balance[0], 0 ) ) {
					memset(buffer, 0, buffer_len);	
					strcat(buffer, "OKBID\0");
					bytecount = send(*csock, buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, *csock);
					fflush(NULL);
					memset(buffer, 0, buffer_len);
					/* flag array to check for completed bids of all players before end the round */
					round_completed_bids[0] = 1;
				}else {
					memset(buffer, 0, buffer_len);	
					strcat(buffer, "INVALIDBID\0");
					bytecount = send(*csock, buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, *csock);
					fflush(NULL);
					memset(buffer, 0, buffer_len);
				}
			}
			else if( *csock == player_thread[1]) 
			{
				if(checkBid( buffer, balance[1], 1 ) ) {
					memset(buffer, 0, buffer_len);	
					strcat(buffer, "OKBID\0");
					bytecount = send(*csock, buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, *csock);
					fflush(NULL);
					memset(buffer, 0, buffer_len);
					//flag array to check for completed bids of all players before end the round
					round_completed_bids[1] = 1;
				}else {
					memset(buffer, 0, buffer_len);	
					strcat(buffer, "INVALIDBID\0");
					bytecount = send(*csock, buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, *csock);
					fflush(NULL);
					memset(buffer, 0, buffer_len);
				}
			}
			else if( *csock == player_thread[2]) 
			{
				if(checkBid( buffer, balance[2], 2 ) ) {
					memset(buffer, 0, buffer_len);	
					strcat(buffer, "OKBID\0");
					bytecount = send(*csock, buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, *csock);
					fflush(NULL);
					memset(buffer, 0, buffer_len);
					//flag array to check for completed bids of all players before end the round
					round_completed_bids[2] = 1;
				}else {
					memset(buffer, 0, buffer_len);	
					strcat(buffer, "INVALIDBID\0");
					bytecount = send(*csock, buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, *csock);
					fflush(NULL);
					memset(buffer, 0, buffer_len);
				}
			}
			else if( *csock == player_thread[3]) 
			{
				if(checkBid( buffer, balance[3], 3 ) ) {
					memset(buffer, 0, buffer_len);	
					strcat(buffer, "OKBID\0");
					bytecount = send(*csock, buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, *csock);
					fflush(NULL);
					memset(buffer, 0, buffer_len);
					//flag array to check for completed bids of all players before end the round
					round_completed_bids[3] = 1;
				}else {
					memset(buffer, 0, buffer_len);	
					strcat(buffer, "INVALIDBID\0");
					bytecount = send(*csock, buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, *csock);
					fflush(NULL);
					memset(buffer, 0, buffer_len);
				}				
			}
			else {
					printf("Player with thread id %d tried to send a bid.\n", *csock);
					fflush(NULL);
					free(csock);
					return NULL;
			}	
			
			//sum the completed bids every time someone makes one
			sum_round=0;
			for(i=0;i<activePlayers;i++){
				sum_round += round_completed_bids[i];
			}
			
			//if sum=activePlayers then voilà,got our end round
			//send them what's what
			if(sum_round == activePlayers) {
			
				/*call the grid_handler 
					to calculate the outcome */
				grid_handler();
									
				/* Shift right the advantage array(cycle) 
					Calculate the new advantage */
				temp = advantage_array[3];
				for(i=1;i<activePlayers;i++){
					advantage_array[4-i] = advantage_array[3-i];
				}
				advantage_array[0] = temp;
								
				/*packet to send 
				 =OBJECTX posX#OBJECTY posY#P1 bid1#P2 bid2#P3 bid3#P4 bid4#ADVANTAGE pnumber#WIN pnumber */
				memset(buffer, 0, buffer_len);	
				
				//construct the whole status packet to send to clients
				strcat(buffer, "OBJECTX ");
				sprintf(item_posx_str, "%d", item_posx);
				strcat(buffer, item_posx_str);
				strcat(buffer, "#");
				
				strcat(buffer, "OBJECTY ");
				sprintf(item_posy_str, "%d", item_posy);
				strcat(buffer, item_posy_str);
				strcat(buffer, "#");
				
				strcat(buffer, "P1 ");
				sprintf(curr_bid_str[0], "%d", curr_bid[0]);
				strcat(buffer, curr_bid_str[0]);
				strcat(buffer, "#");
				
				strcat(buffer, "P2 ");
				sprintf(curr_bid_str[1], "%d", curr_bid[1]);
				strcat(buffer, curr_bid_str[1]);
				strcat(buffer, "#");
				
				strcat(buffer, "P3 ");
				sprintf(curr_bid_str[2], "%d", curr_bid[2]);
				strcat(buffer, curr_bid_str[2]);
				strcat(buffer, "#");
				
				strcat(buffer, "P4 ");
				sprintf(curr_bid_str[3], "%d", curr_bid[3]);
				strcat(buffer, curr_bid_str[3]);
				strcat(buffer, "#");
				
				strcat(buffer, "ADVANTAGE ");
				sprintf(player_advantage_str, "%d", advantage_array[3]);
				strcat(buffer, player_advantage_str);
				strcat(buffer, "#");
				
				strcat(buffer, "WIN ");
				sprintf(round_winner_str, "%d", round_winner);
				strcat(buffer, round_winner_str);
				
				if(total_winner){
					strcat(buffer, "#");
					strcat(buffer, "WINNER ");
					sprintf(total_winner_str, "%d", total_winner);
					strcat(buffer, total_winner_str);
					
				}
				strcat(buffer, "\0");
				
				sleep(6); //suspense for a while
				
				//Send packet to every thread
				for(i=0;i<activePlayers;i++)
				{
					bytecount = send(player_thread[i], buffer, buffer_len, 0);
					printf("Sent %d bytes(%s) to thread %d\n", bytecount, buffer, player_thread[i]);
					fflush(NULL);				
				}
				
				memset(buffer, 0, buffer_len);
				
				//In case we have a total winner raise the game state, save the data and break the while recv
				if(total_winner > 0){
					gamestate++;
					saveToFile();
					break;
				}
				
				/* zero the completed bids' flags
					to start a new round */
				for(i=0;i<activePlayers;i++){
					round_completed_bids[i] = 0;
				}	
				
				/* One round ended so let's
					raise the round counter */
				round_num++;
			}
			
		/* Case a non HELLO/BID packet received */
		}else {
			printf("Invalid command received.\n");
			memset(buffer, 0, buffer_len);	
			strcat(buffer, "NOT ACCEPTED COMMAND || GAME STARTED ALREADY.\n");
			bytecount = send(*csock, buffer, buffer_len, 0);
			memset(buffer, 0, buffer_len);
		}
	}
	
	/* Check for an end game 
		If so, release thread */
	if(gamestate==2){
		printf("Game State = 2.\nReleasing thread %d\n", *csock);
		fflush(NULL);
		free(csock);
		return NULL;
	}

    return NULL;
}
