#include<stdio.h>
#include<termios.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>
#include<pthread.h>
#include<netinet/in.h>
#define PORT 8080

int connected;
int connectedwrite;
int accwrite;
int filewrite;
int accountn;

typedef struct gamedata
{
   int player, enemy;
   int* healthbar1;
   int* healthbar2;
}gamedata;

typedef struct playerdata
{
   int player;
   int skip;
}playerdata;

typedef struct user
{
   char username[50];
   char pass[50];
}user;

user accounts[150];

void* server(void* arg)
{
   playerdata d = *(playerdata*)arg;
   int player = d.player;
   int skip = d.skip;
   int mode;

   if (!skip)
   {
	screen1:;

	int i = 2, a;
	char data[150];
	memset(data, 0, sizeof(data));

	int mode;
	char username[50], pass[50];
	read(player, data, sizeof(data));
	mode = data[0] - '0';

	for (i = 2; i < strlen(data); i++)
	{
	   if (data[i] == '	')  break;
	   username[i-2] = data [i];
	}
	
	username[i-2] = '\0';
	a = ++i;

	for (i = 2; i < strlen(data); i++)
	{
	   if (data[i] == '	')  break;
	   pass[i-a] = data[i];
	}
	
	if (mode == 0) //mode 0 menunjukkan bahwa akan login
	{
	   int success = 0;
	   while (accwrite);

	   for (i = 0; i  < accountn; i++)
	   {
		if (strcmp(username, accounts[i].username) == 0 && strcmp(pass, accounts[i].pass) == 0)
		{
		   success = 1;
		   break;
		}
	   }
	   
	   send(player, &success, sizeof(success), 0);

	   if (success) 
	   {
		printf("Authenticator Success\n");
	   }
	   else
	   {
		printf("Authenticator Failed\n");
		goto screen1;
	   }
	}
	
	else if (mode == 1)  //mode 0 menunjukkan bahwa akan regiatrasi
	{
	   accwrite = 1;
	   user newaccount;
	   strcpy(newaccount.username, username);
	   strcpy(newaccount.pass, pass);
	   accounts[accountn] = newaccount;

	   while (filewrite)
	   filewrite = 1;
	   FILE *f = fopen("akun.txt", "a");
	   fprintf(f, "%s	%s\n", username, pass);
	   fclose(f);
	   
	   filewrite = 0;
	   accountn++;
	   accwrite = 0;
	   for (i = 0; i < accountn; i++)
	   {
		printf("%s	%s\n", accounts[i].username, accounts[i].pass);
	   }
	}

   }
   
   skip = 0;
   read(player, &mode, sizeof(mode));

   if (mode == 0)  //dalam loop ini menunjukkan akan menuju ke finding match
   {
	while (connectedwrite);
	connectedwrite = 1;
	connected++;
	
	connectedwrite = 0;
	while (connected < 2);
   }
   else if (mode == 1)  //dalam loop ini menunjukkan akan logout
   {
	goto screen1; 
   }
   pthread_exit(0);
}

void* ingame(void* arg)
{
   int start;
   char jum;
   gamedata file = *(gamedata*)arg;

   int player = file.player;
   int enemy = file.enemy;
   int* kita = file.healthbar1;
   int* musuh = file.healthbar2;

   start = 1;
   send(player, &start, sizeof(start), 0);
   while (1)
   {
	read(player, &jum, sizeof(jum));
	*musuh -= 10;
	send(enemy, musuh, sizeof(*musuh), 0);
   }	
}


int main()
{
   int i, doc, player[2], index = 0, opt = 1;
   struct sockaddr_in address;
   int addrlen = sizeof(address);
   
   if ((doc = socket(AF_INET, SOCK_STREAM, 0)) == 0)
   {
	perror("Socket Fail\n");
	exit(EXIT_FAILURE);
   }

   if (setsockopt(doc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
   {
	perror("Setsockopt Fail\n");
	exit(EXIT_FAILURE);
   }

   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(PORT);

   if (bind(doc, (struct sockaddr *)&address, sizeof(address)) < 0)
   {
	perror("Bind Data Fail\n");
	exit(EXIT_FAILURE);
   }

   if (listen(doc, 3) < 0)
   {
	perror("Listen Fail\n");
	exit(EXIT_FAILURE);
   }

   FILE *f = fopen("akun.txt", "a+");
   char username[50], pass[50];

   while (fscanf(f, "%[^\t]\t%[^\n]\n", username, pass) != EOF)
   {
	user newaccount;
	strcpy(newaccount.username, username);
	strcpy(newaccount.pass, pass);
	accounts[index] = newaccount;
	index++;
   }

   fclose(f);
   accountn = index;
   accwrite = 0;
   filewrite = 0;

   connected = 0; 
   connectedwrite = 0;
   pthread_t serverThreads[2];

   for (i = 0; i < 2; i++)
   {
	player[i] = accept(doc, (struct sockaddr *)&address, (socklen_t*)&addrlen);
	
	if (player[i] < 0)
	{
	   perror("Cannot Accept Client Data!!!\n");
	   i--;
	   continue;
	}

	printf("Client Accepted.\n");
	playerdata cl;
	cl.player = player[i];
	cl.skip = 0;
	pthread_create(&serverThreads[i], NULL, server, (void*)&cl);
   }

   while (1)
   {
	pthread_join(serverThreads[0], NULL);
	pthread_join(serverThreads[1], NULL);

	int healthbar1 = 100, healthbar2 = 100;
	gamedata no1, no2;
	no1.player = player[0];
	no1.enemy = player[1];
	no1.healthbar1 = &healthbar1;
	no1.healthbar2 = &healthbar2;
	no2.player = player[1];
	no2.enemy = player[0];
	no2.healthbar1 = &healthbar2;
	no2.healthbar2 = &healthbar1;

	pthread_create(&serverThreads[0], NULL, ingame, (void*)&no1);
	pthread_create(&serverThreads[1], NULL, ingame, (void*)&no2);

	while (1)
	{
	   if (healthbar1 <= 0 || healthbar2 <= 0)
	   {
		//printf("Game Over!!!\n");
		break;
	   }
	}
	
	pthread_cancel(serverThreads[0]);
	pthread_cancel(serverThreads[1]);

	int chicken = 1, defeat = 0, stop = -1337;
	send(player[0], &stop, sizeof(stop), 0);
	send(player[1], &stop, sizeof(stop), 0);
	connected -= 2;

	if (healthbar2 <= 0)
	{
	   send(player[1], &defeat, sizeof(defeat), 0);
	   send(player[0], &chicken, sizeof(chicken), 0);
	   
	}
	else if (healthbar1 <= 0)
	{
	   send(player[0], &defeat, sizeof(defeat), 0);
	   send(player[1], &chicken, sizeof(chicken), 0);
	}

	playerdata old, new;
	old.player = player[0];
	old.skip = 1;
	new.player = player[1];
	new.skip = 1;

	pthread_create(&serverThreads[0], NULL, server, (void*)&old);
	pthread_create(&serverThreads[1], NULL, server, (void*)&new);
   }
}
