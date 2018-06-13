#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

#define MAX_CLIENTS	100
#define BUFLEN 256

//structura creata pentru client(persoana)
//am adaugat logged si block pentru a tine evidenta 
//daca cineva e logat sau blocat
typedef struct {
	char nume[13], prenume[13], numar_card[7], pin[5], parola_secreta[17];
	double sold;
	int logged, block;
} user;

//functie pentru mesaje de eroare
void error(char *msg)
{
    perror(msg);
    exit(1);
}

//functia logout care returneaza -11(comanda invalida)
// sau -12 daca s-a realizat delogarea
//si face logged = 0 pentru utilizatorul care cere logout
//(logout = 0 inseamna ca nu este logat si 1 contrar)
int logout(user users[], char buffer[], int nr_of_clients, int client_sckt_id, int socket_client_id[])
{
	if(strlen(buffer) != 6){
		
		return -11;

	}
	users[socket_client_id[client_sckt_id]].logged = 0;

	return -12;
}

//returneaza un cod in functie de caz
//care va fi folosit intr-o functie care pentru a returna un mesaj
int listsold(user users[], char buffer[], int nr_of_clients, int client_sckt_id, int socket_client_id[])
{
	if(strlen(buffer) != 8)
		return -11;

	if(users[socket_client_id[client_sckt_id]].logged == 0)
		return -1;

	return -13;
}

//verifica daca exista destui bani si daca suma ceruta este multiplu de 10
//si returneaza cod
int getmoney(user users[], char buffer[], int nr_of_clients, int client_sckt_id, int socket_client_id[])
{

	if(users[socket_client_id[client_sckt_id]].logged == 0)
		return -1;

	double money;
	char *aux, money_string[100];
	aux = strtok(buffer, " ");
	aux = strtok(NULL, " ");
	strcpy(money_string, aux);

	money = atof(money_string);

	if(fmod(money, 10.0) != 0)
		return -9;
	else if (money > users[socket_client_id[client_sckt_id]].sold)
		return -8;
	else
		users[socket_client_id[client_sckt_id]].sold -= money;
	
	return -14;
}

//adauga suma de bani in cont si returneaza cod
int putmoney(user users[], char buffer[], int nr_of_clients, int client_sckt_id, int socket_client_id[])
{
	if(users[socket_client_id[client_sckt_id]].logged == 0)
		return -1;
	
	double money;
	char *aux, money_string[100];
	aux = strtok(buffer, " ");
	aux = strtok(NULL, " ");
	strcpy(money_string, aux);

	money = atof(money_string);

	users[socket_client_id[client_sckt_id]].sold += money;

	return -15;
}

//deblocheaza sau nu userul in functie daca este blocat sau nu
int unlock(user users[], char buffer[], int nr_of_clients)
{
	
	char *aux, card_nr[7] = "";
	aux = strtok(buffer, " ");
	aux = strtok(NULL, " ");

	strcpy(buffer, aux);
	buffer[6] = '\0';

	strcpy(card_nr, aux);

	int i;

	for (i = 0; i < nr_of_clients; i++) {
		if(strncmp(card_nr, users[i].numar_card, 6) == 0 && users[i].block >= 3)
			return -16;

		if(strncmp(card_nr, users[i].numar_card, 6) == 0 && users[i].block < 3)
			return -6;

	}

	return -4;
}

//verific daca clientul este blocat, logat deja
//sau daca s-a gresit pinul (in acest caz block creste si 
//la block >= 3 cardul este blocat)
int login(user users[], char buffer[], int nr_of_clients, int client_sckt_id, int socket_client_id[])
{
	int i;
	char *aux, numar_card[7], pin[5];
	
	if(strlen(buffer) != 17)
		return -11;

	aux = strtok(buffer, " ");
	aux = strtok(NULL, " ");
	strcpy(numar_card, aux);


	if(strlen(numar_card) != 6 ) {
		return -11;
	}

	aux = strtok(NULL, " ");
	strcpy(pin, aux);

	if(strlen(pin) != 4 ) {
		return -11;
	}

	

	for(i = 0; i < nr_of_clients; i++) {
		if(strncmp(numar_card, users[i].numar_card, 6) == 0 
			&& strncmp(pin, users[i].pin, 4) == 0) {

			if(users[i].block >= 3)
			return -5;

			if (users[i].logged == 1)
				return -2;
			else {
				socket_client_id[client_sckt_id] = i;
				users[i].logged = 1;
				return i;
			}
		}
			

	if(strncmp(numar_card, users[i].numar_card, 6) == 0 
		&& strncmp(pin, users[i].pin, 4) != 0) {
		users[i].block += 1;

		if(users[i].block >= 3)
			return -5;

		return -3;
		}
	}

	return -4;
}

//functia de message handling care pune un anumit string in send_message in functie de caz
//si in cazul codului -17 deblocheaza clientul
void respons_message_handle(char buffer[], int code, char* send_message, user users[], int client_sckt_id, int socket_client_id[])
{

	if(code >= 0) {
		strcat(send_message, "Welcome ");
		strcat(send_message, users[code].nume);
		strcat(send_message, " ");
		strcat(send_message, users[code].prenume);
		send_message[strlen(send_message)] = '\0';
	}

	if(code == -1) {
		strncpy(send_message, "-1 : Clientul nu este autentificat", 34);
		send_message[34] = '\0';
	}

	if(code == -2) {
		strncpy(send_message, "-2 : Sesiune deja deschisa", 26);
		send_message[26] = '\0';
	}

	if(code == -3) {
		strncpy(send_message, "-3 : Pin gresit", 15);
		send_message[15] = '\0';
	}

	if(code == -4) {
		strncpy(send_message, "-4 : Numar card inexistent", 26);
		send_message[26] = '\0';
	}

	if(code == -5) {
		strncpy(send_message, "-5 : Card blocat", 16);
		send_message[16] = '\0';
	}

	if(code == -6) {
		strncpy(send_message, "-6 : Operatie esuata", 20);
		send_message[20] = '\0';
	}

	if(code == -7) {
		strncpy(send_message, "-7 : Deblocare esuata", 21);
		send_message[21] = '\0';
	}

	if(code == -8) {
		strncpy(send_message, "-8 : Fonduri insuficiente", 26);
		send_message[26] = '\0';
	}

	if(code == -9) {
		strncpy(send_message, "-9 : Suma nu e multiplu de 10", 29);
		send_message[29] = '\0';
	}

	if(code == -11) {
		strncpy(send_message, "Credentiale gresite", 19);
		send_message[19] = '\0';
	}

	if(code == -12) {
		strncpy(send_message, "Deconectare de la bancomat", 26);
		send_message[26] = '\0';
	}

	if(code == -13) {
		
		sprintf(send_message, "%.3f", users[socket_client_id[client_sckt_id]].sold);
		send_message[strlen(send_message) - 1] = '\0';
	}

	if(code == -14) {	
		char *aux, money_string[100];
		aux = strtok(buffer, " ");
		aux = strtok(NULL, " ");
		strcpy(money_string, aux);
		
		strncat(send_message, "Suma ", 5);
		strncat(send_message, money_string, strlen(money_string));
		strncat(send_message, " a fost retrasa cu succes", 25);
	}

	if(code == -15) {
		strncpy(send_message, "Suma depusa cu succes", 21);
		send_message[21] = '\0';
	}

	if(code == -16) {
		strncpy(send_message, "Trimite parola secreta", 22);
		send_message[22] = '\0';
	}
	
	if(code == -17) {
		strncpy(send_message, "Client deblocat", 16);
		send_message[16] = '\0';

		users[client_sckt_id].block = 0;
	}
}

int main(int argc, char *argv[])
{
    int sockTCP, sockUDP, newsockTCP, portno, clilen, nr_of_clients, respons, deblocat = 0;
    char *user_information, buffer[BUFLEN], buffer2[BUFLEN], read_buff[BUFLEN], snd_msg[31], numar_card[7];
    struct sockaddr_in serv_addr, cli_addr;
    int n, i, j;
    FILE *f;
    char rcv_unlock[17];

    if (argc < 3) {
        fprintf(stderr,"Usage : %s port user_data_file\n", argv[0]);
        exit(1);
    }

    f = fopen(argv[2], "r");

    fgets (read_buff, BUFLEN, f);
    nr_of_clients = atoi(read_buff); //numarul maxim de clienti

    user users[nr_of_clients]; //structura de useri si informatiile lor
    
    memset(rcv_unlock, 0, 14);
		
	//citesc din fisier si adaug in structura informatiile clientilor
    for (i = 0; i < nr_of_clients; i++){
    	fgets (read_buff, BUFLEN, f);
    	
    	user_information = strtok (read_buff, " ");
    	strcpy(users[i].nume, user_information);

    	

    	user_information = strtok (NULL, " ");
    	strcpy(users[i].prenume, user_information);

    	user_information = strtok (NULL, " ");
    	strcpy(users[i].numar_card, user_information);
    	
    	user_information = strtok (NULL, " ");
    	strcpy(users[i].pin, user_information);

    	user_information = strtok (NULL, " ");
    	strcpy(users[i].parola_secreta, user_information);

    	user_information = strtok (NULL, " ");
    	users[i].sold = atof(user_information);

    	users[i].logged = 0;
    	users[i].block = 0;
    	

    }
    	 
    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;	//multime folosita temporar 
    int fdmax;		//valoare maxima file descriptor din multimea read_fds
       
    //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
     
    sockTCP = socket(AF_INET, SOCK_STREAM, 0); //creez socketul de TCP
    if (sockTCP < 0) 
        error("ERROR opening socket");

    sockUDP = socket(AF_INET, SOCK_DGRAM, 0);  //creez socketul de UDP
    if (sockUDP < 0) 
        error("ERROR opening socket");
    	
    portno = atoi(argv[1]); //portul

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
    serv_addr.sin_port = htons(portno);
     
    //bind pentru UDP
    if (bind(sockUDP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
        error("ERROR on binding");

    //bind pentru TCP
    if (bind(sockTCP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
        error("ERROR on binding");
     
    listen(sockTCP, MAX_CLIENTS);

    //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockTCP, &read_fds); //adaug socketul de TCP in multimea de file descriptori cititi
    FD_SET(sockUDP, &read_fds); //la fel si UDP
    FD_SET(STDIN_FILENO, &read_fds); // si STDIN

    fdmax = sockUDP;
    int min_clients = fdmax + 1; //minimul de unde incepe socketul primului client
    int clients = fdmax + 1; // si maximul care va creste de fiecare data cand cineva se conecteaza
    
    int socket_client_id[fdmax]; //cand cineva se logheaza pe un anumit socket salvez al catelea este
    							 //in structura mea

    // main loop
	while (1) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) //apelul select pentru multiplexare
			error("ERROR in select");
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == STDIN_FILENO) { //citesc de la tastatura
          			memset(buffer, 0 , BUFLEN);
          			fgets(buffer, BUFLEN-1, stdin);
          			buffer[4] = '\0';          			          			

					//cand citesc quit trimit "quit" la toti clientii si inchid toate conexiunile          			
					if(strncmp(buffer,"quit",4)==0) {
						for (j = min_clients; j < clients; j++) {
            				send(j,buffer,strlen(buffer), 0);
          				}
          				close(sockUDP);
						close(sockTCP);	
						return 0;
					}
					
        		}
				
				if (i == sockTCP) {
					//primesc mesaj de la un client pe TCP
					clilen = sizeof(cli_addr);
					if ((newsockTCP = accept(sockTCP, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockTCP, &read_fds);
						if (newsockTCP > fdmax) { 
							fdmax = newsockTCP;
						}
					}
					printf("S-a conectat clientul %d\n", newsockTCP);
					clients++;
					
					memset(buffer, 0 ,BUFLEN);
					sprintf(buffer, "S-a conectat %d\n", newsockTCP);
		
				}
				else if(i == sockUDP) {
					memset(rcv_unlock, 0 ,17);
					//primesc mesaj de la un client pe UDP
              		if (recvfrom(sockUDP, rcv_unlock, 17, 0, (struct sockaddr*)
							&cli_addr, &clilen) <= 0)
              			error("ERROR in receive");
              		else{    

	              		char send_unlock[26];
	              		strcpy(send_unlock, rcv_unlock);

	              		//primesc parola_secreta si deblochez sau nu in functie parola 
	              		//este buna sau nu
	              		if(strncmp(rcv_unlock, "unlock", 6) != 0) {
	              			for(j = 0; j < nr_of_clients; j++) {
	              				
	              				if(strncmp(users[j].numar_card, numar_card, 6) == 0
	              					&& strcmp(send_unlock, users[j].parola_secreta) == 0) {
	              					deblocat = 1;
	              					respons_message_handle(buffer2, -17, send_unlock, users, j, socket_client_id);
	              					
	              					if (sendto(sockUDP, send_unlock, 17, 0,
	                            			(struct sockaddr*) &cli_addr, sizeof(cli_addr)) <= 0)
	              						error("ERROR in send");

	              					break;
	              				} 
	              			}
	              			if(deblocat == 0) {
		              			respons_message_handle(buffer2, -7, send_unlock, users, j, socket_client_id);
		              					
		              			if (sendto(sockUDP, send_unlock, 22, 0,
		                            	(struct sockaddr*) &cli_addr, sizeof(cli_addr)) <= 0)
		              				error("ERROR in send");
		              		}
	              		}else {
	              			//primesc unlock de la client si tratrez cu functia unlock
		              		respons = unlock(users, send_unlock, nr_of_clients);
		              		strcpy(numar_card, send_unlock); //salvez numarul cardului dupa o comanda unlock
		              		
		              		respons_message_handle(buffer2, respons, send_unlock, users, i, socket_client_id);
		              		printf("UNLOCK> %s\n",send_unlock);

		        			

		              		if (sendto(sockUDP, send_unlock, 26, 0,
		                            (struct sockaddr*) &cli_addr, sizeof(cli_addr)) <= 0)
		              			error("ERROR in send");

	              		}
              		
              		}
              	} 
              	else
				{
					//am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiunea serverului: recv()
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("Clientul %d s-a deconectat\n", i);
						} else {
							error("ERROR in recv");
						}
						close(i); 
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
					} 
					
					else { //recv intoarce >0
						//tratez fiecare comanda in parte cu functia corespunzatoare
						//respons_message_handle imi creeaza mesajul de trimis
						//dupa care trimit la client
						printf ("ATM>  %s\n", buffer);
						strcpy(buffer2, buffer);
						
						if (strncmp(buffer, "login", 5) == 0) 
							respons = login(users, buffer, nr_of_clients, i, socket_client_id);
						else
						if (strncmp(buffer, "logout", 6) == 0) 
							respons = logout(users, buffer, nr_of_clients, i, socket_client_id);
						else
						if (strncmp(buffer, "listsold", 8) == 0) 
							respons = listsold(users, buffer, nr_of_clients, i, socket_client_id);
						else
						if (strncmp(buffer, "getmoney", 8) == 0) 
							respons = getmoney(users, buffer, nr_of_clients, i, socket_client_id);
						else
						if (strncmp(buffer, "putmoney", 8) == 0) 
							respons = putmoney(users, buffer, nr_of_clients, i, socket_client_id);
						else
						if (strncmp(buffer, "quit", 4) == 0)
							 
							i = i;
						else
							respons = -11;

						memset(snd_msg, 0 ,30);
						respons_message_handle(buffer2, respons, snd_msg, users, i, socket_client_id);
						

						send(i, snd_msg, strlen(snd_msg), 0); // trimit raspuns inapoi clientului

					}

              	}


						
			}
		} 
	}		  
return 0; 
}


