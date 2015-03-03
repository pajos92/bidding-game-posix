#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

#define RCVBUFSIZE 80   /* Size of receive buffer */

int startsWith(const char*, const char*);
void splitBySharp(char[]);
int splitByBlank(char[]);

/* Variables Game Related Start*/

	char	objectx[10],objecty[10];
	int 	item_posx=4,item_posy=4;
	int 	player_advantage;
	
	char	*game[9][9];
	
	int 	total_winner=0;
	int 	round_num=1;
	
	int 	player_num;
	int		p1bid,p2bid,p3bid,p4bid,advantage,win;
	int 	myBalance;
	
/* Variables Game Related End*/

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in ServAddr;     /* server address */
    unsigned short ServPort;         /* server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char command[20];                /* String to send to server */
    char buffer[RCVBUFSIZE];     /* Buffer for receiving commands */
    unsigned int commandLen;      /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv() 
                                        and total bytes read */

		
	char start_packet[13] = "STARTINGGAME";
	char hello_packet[6] = "HELLO";
	char bid_packet[4] = "BID";
	char okbid[3] = "OK";
	char invalidbid[8] = "INVALID";
	
	
	int i=0,j=0;
	
    if ((argc < 2) || (argc > 3))    /* Test for correct number of arguments */
    {
		fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n",argv[0]);
		exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */

    ServPort = atoi(argv[2]); /* Use given port */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("socket() failed\n");
	exit(-1);
    }

    /* Construct the server address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
    ServAddr.sin_family      = AF_INET;             /* Internet address family */
    ServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    ServAddr.sin_port        = htons(ServPort); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0) {
        printf("connect() failed");
	exit(-1);
    }
	
	//initialize the game array as graphics to player
	for(i=0;i<9;i++){
		for(j=0;j<9;j++){
			game[i][j] = "-";
		}
	}
	
	game[0][0] = "1";
	game[0][8] = "2";
	game[8][0] = "3";
	game[8][8] = "4";
	
	game[item_posx][item_posy] = "O";
	
	//Let's start
	//Read the HELLO to send to server so the game starts
	printf("Enter 'HELLO name' for server acceptance.\n");
	fflush(NULL);
	fgets(command,20,stdin);
	commandLen = strlen(command); 

	//Send HELLO to the server
	if (send(sock, command, commandLen, 0) != commandLen) {
		printf("send() sent a different number of bytes than expected\n");
	}

	//Receive the WELCOME from server
	if ((bytesRcvd = recv(sock, buffer, RCVBUFSIZE, 0)) <= 0) 
	{
		printf("recv() failed or connection closed prematurely.\n");
		exit(-1);
	}
	buffer[bytesRcvd] = '\0';  // Terminate the string! 
	printf("%s", buffer);      // Print the echo buffer 
	fflush(NULL);
	printf("\n");
	fflush(NULL);
	//Received WELCOME 	
	
	/* Get the player number from welcome msg */
	player_num = splitByBlank(buffer);


	//Waiting for the STARTINGGAME command from server
	while (1){
		bytesRcvd = recv(sock, buffer, RCVBUFSIZE, 0);
		if (bytesRcvd == 0)
				continue;
		buffer[bytesRcvd] = '\0';  // Terminate the string! 
		printf("%s", buffer);      // Print the echo buffer 
		fflush(NULL);
		printf("\n");
		fflush(NULL);
		break;
	}
	
	myBalance = splitByBlank(buffer);
	
	while (!total_winner)
	{		
		//Received STARTINGGAME 		
		// ====================== GAME STATE 1 =======================================
		int validBid = 0;
		while (!validBid){
		
			printf("Enter bid: (e.g 'BID 100')\n");
			fflush(NULL);
			fgets(command,20,stdin);
			commandLen = strlen(command); 
				
			//Send the string to the server
			if (send(sock, command, commandLen, 0) != commandLen) {
				printf("send() sent a different number of bytes than expected\n");
			}

			//Receive the OKBID OR INVALIDBID
			if ((bytesRcvd = recv(sock, buffer, RCVBUFSIZE, 0)) <= 0) 
			{
				printf("recv() failed or connection closed prematurely\n");
				exit(-1);
			}
			buffer[bytesRcvd] = '\0';  // Terminate the string! 
			printf("%s", buffer);      // Print the echo buffer 
			fflush(NULL);
			printf("\n");
			fflush(NULL);
			if(startsWith(okbid,buffer))
				validBid = 1;
		}
		
		printf("Waiting for other players to bid...\n");
		fflush(NULL);
		
		//Waiting for the STATUS PACKET of the game
		while (1){
			bytesRcvd = recv(sock, buffer, RCVBUFSIZE, 0);
			if (bytesRcvd == 0)
					continue;
			buffer[bytesRcvd] = '\0';  // Terminate the string! 
			system ("clear");				//clear screen to see the game status clearly
			sleep(1); //more suspense
			break;
		}
		
		/* Place "-" to the old spot coordinates */
		game[item_posx][item_posy] = "-";
		/* Get all game status by spliting the buffer */
		splitBySharp(buffer);
		
		/* Calculate my balance after biding */ 
		switch(player_num){
			case 1:
				myBalance -= p1bid;
				break;
			case 2:
				myBalance -= p2bid;
				break;
			case 3:
				myBalance -= p3bid;
				break;
			case 4:
				myBalance -= p4bid;
				break;			
		}
		
		printf("ROUND %d - PLAYER %d - MY BALANCE %d\n", round_num, player_num, myBalance);      // Print the buffer
		fflush(NULL);
		printf("P1 BID : %d\tP2 BID : %d\tP3 BID : %d\tP4 BID : %d\n",p1bid,p2bid,p3bid,p4bid);
		fflush(NULL);
		printf("ADVANTAGE %d\nWIN %d\n",advantage,win);
		fflush(NULL);
		
		/* Place the item in the new spot coordinates*/
		game[item_posx][item_posy] = "O";
		
		/* Print the stage */
		for(i=0;i<9;i++){
			for(j=0;j<9;j++){
				printf("%s",game[i][j]);
			}
			printf("\n");
		}		
		round_num++;
	}
	printf("GAME ENDED.\nWINNER %d\n",total_winner);
	fflush(NULL);
    close(sock);
    exit(0);
}

/* Function that compares strings 
param 1 : text that is being searched
param 2 : text to search inside param 1 
*/
int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

void splitBySharp(char str[]){

	int t=0;
	const char s[2] = "#";
	char *token;
	char *tokens_array[9];
	int bid = 0;
	
	char winner[6] = "WINNE";
			   
	/* get the first token */
	token = strtok(str, s);
			   
	/* walk through other tokens */
	while( token != NULL ) 
	{
		tokens_array[t] = token;
		token = strtok(NULL, s);
		t++;
	}
	
	//proceed to split by " "
	item_posx = splitByBlank(tokens_array[0]);		
	item_posy = splitByBlank(tokens_array[1]);	

	p1bid = splitByBlank(tokens_array[2]);	 
	p2bid = splitByBlank(tokens_array[3]);	 
	p3bid = splitByBlank(tokens_array[4]);	 
	p4bid = splitByBlank(tokens_array[5]);	
	advantage = splitByBlank(tokens_array[6]);	 
	win = splitByBlank(tokens_array[7]);
	
	//also check if there is total winner every time the packet is analyzed
	//total winnner comes to 9th position spliting by #
	if(tokens_array[8] != NULL){
		if(startsWith(winner,tokens_array[8])){
			total_winner = splitByBlank(tokens_array[8]);
		}
	}
}

/*
	Function to split the buffer by " "
	param 1 : buffer
*/
int splitByBlank(char str[]){

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
	
	return atoi(tokens_array[1]);
}
