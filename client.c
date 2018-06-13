#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include <arpa/inet.h>


#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockTCP, sockUDP, n, i, logged = 0;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char numar_card[7], *aux;
    char pid[10], buffer[BUFLEN], *utilizator_logat, buffer_aux[BUFLEN];
    char fisier_out[50], wr[50], send_unlock[14] = "", recv_sv[24] = ""; 
  
    FILE *fw;
    if (argc < 3) {
       fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
       exit(0);
    }  
    
	sockTCP = socket(AF_INET, SOCK_STREAM, 0); //creez socket pentru TCP
    sockUDP = socket(AF_INET, SOCK_DGRAM, 0);  //creez socket pentru UDP

    if (sockTCP < 0) 
        error("ERROR opening socket");

    if (sockUDP < 0) 
        error("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    
    // ma conectez la server
    if (connect(sockTCP,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");   

	fd_set read_fs, temp_fs;
    FD_SET(sockTCP, &read_fs);
    FD_SET(STDIN_FILENO, &read_fs);
    FD_SET(sockUDP, &read_fs);

    strcpy(fisier_out, "client-");     
    sprintf(pid, "%d", getpid());      
    strcat(fisier_out, pid);
    strcat(fisier_out, ".log");
    fw = fopen(fisier_out, "w"); // deschid fisierul in care citesc
    
    int fd_max = sockUDP;

    while(1){
		temp_fs = read_fs;
		select(fd_max + 1, &temp_fs, NULL, NULL, NULL); // apelul select pentru multiplexare
	
		memset(buffer, 0 , BUFLEN);

		for(i = 0; i<= fd_max; i++) {
			if(FD_ISSET(i, &temp_fs) ) {

				if(i == STDIN_FILENO) {
					//citesc de la tastatura

    				memset(buffer, 0 , BUFLEN);

    				fgets(buffer, BUFLEN-1, stdin);


                    buffer[strlen(buffer)-1] = '\0';                 

                    //daca citesc unlock atunci trimit "unlock <numar_card>"
                    //numar_card fiind numarul de card al ultimului apel de login
                    if(strncmp(buffer, "unlock", 6) == 0 && strlen(buffer) == 6) {
                        
                        
                        strcat(send_unlock, "unlock ");
                        strcat(send_unlock, numar_card);

                        fprintf(fw, "%s", "unlock");                        
                        fprintf(fw, "%s", "\n");

                        sendto(sockUDP, send_unlock, 13, 0,
                            (struct sockaddr*) &serv_addr, sizeof(serv_addr));
                        break;
                        
                    }
                    
                    //daca ultimul string primit de la UDP este cel de jos
                    //atunci o sa trimit ca parola ce citesc de la tastatura
                    if(strncmp(recv_sv,"Trimite parola secreta", 22) == 0) {
                        
                        fprintf(fw, "%s", buffer);
                        fprintf(fw, "%s", "\n");
                        

                        sendto(sockUDP, buffer, 6, 0,
                            (struct sockaddr*) &serv_addr, sizeof(serv_addr));
                        recv_sv[0] = '\0';

                        break;
                    }

                    //daca citesc quit inchid fisierul, trimit quit la server si dau return 0
                    if (strncmp(buffer, "quit", 4) == 0){
                        fprintf(fw, "%s", "quit");
                        fclose(fw);                        
                        return 0;
                    }                    

                    //verific daca cumva este cineva logat deja
                    //si afisez in functie de caz
                    if (strncmp(buffer, "login", 5) == 0 && logged == 1 
                        && strlen(buffer) == 17) { 

                        printf("-2 : Sesiune deja existenta\n");
                        fprintf(fw, "%s", "-2 : Sesiune deja existenta\n\n");

                    }
                    else {
                        fprintf(fw, "%s\n",buffer);

                        if(strncmp(buffer, "logout", 6) == 0 && strlen(buffer) == 6
                            && logged == 0) {
                            printf("-1 : Clientul nu este autentificat\n");
                            fprintf(fw, "%s", "-1 : Clientul nu este autentificat\n\n");
                        }
                        else 
                            n = send(sockTCP,buffer,strlen(buffer), 0); //trimit comanda catre server
    				    
                    }
                    if (n < 0) 
        				error("ERROR writing to socket");

                    //cand citesc login ii retin numarul cardului in numar_card
                    if (strncmp(buffer, "login", 5) == 0 && logged == 0 
                        && strlen(buffer) == 17) {

                        memset(numar_card, 0, 7);
                        aux = strtok(buffer, " ");
                        aux = strtok(NULL, " ");
                        strcpy(numar_card, aux);
                       

                    }
		
				} if (i == sockTCP) { 
                    // cand primesc raspuns de la server pe TCP
					n = recv(i, buffer, sizeof(buffer), 0); 
					if (n <= 0)
                        error("ERROR in recv");
                    if (n > 0) {

                        strcpy(wr, buffer);

                        //cat timp utilizatorul e logat se va scrie in fisierul acestuia
                        if(strncmp(buffer, "Credentiale gresite", 19) != 0) {  
						    fprintf(fw, "%s", "ATM> ");
                            fprintf(fw, "%s", wr); //raspunsurile de la server
                            fprintf(fw, "%s", "\n\n");
                            
                        }
                        memset(wr, 0, 50);

                        //afisez in consola ce scriu in fisier
                        printf("ATM> %s\n", buffer);

                        //daca am primit confirmare de logare de la server
                        //initializez logged cu 1 pentru logat
                        if(strncmp(buffer, "Welcome", 7) == 0) { 
                                                
                            logged = 1;     //semnalez ca sesiunea este deschisa
                        }

                        //daca primesc confirmare de deconectare 
                        //initializez logged cu 0 pentru deconectat
                        if(strncmp(buffer, "Deconectare", 11) == 0) {  
                            printf("s-a deconectat\n"); 
                            logged = 0;     //semnalez ca sesiunea se inchide
                        }
                    }

                    //serverul imi trimite mesaj quit ca isi termina activitatea
                    if(strncmp(buffer, "quit", 4) == 0){
                        printf("Serverul isi incheie activitatea\n");
                        fprintf(fw, "%s", "Serverul isi incheie activitatea");
                        fclose(fw);
                        return 0;
                    } 

                    printf("\n");
					
				} 
                    if (i == sockUDP) {

                        int servlen = sizeof(serv_addr);
                        //primesc raspuns de la server pe UDP
                        if (recvfrom(sockUDP, buffer, BUFLEN, 0, (struct sockaddr*)
                                &serv_addr, &servlen) < 0)
                            perror("ERROR in recv");
                  
                        printf("UNLOCK> %s\n",buffer);

                       //scriu in fisier raspunsul
                        fprintf(fw, "%s", "UNLOCK> ");
                        fprintf(fw, "%s", buffer);
                        fprintf(fw, "%s", "\n\n");

                        //daca primesc de la server stringul "Trimite parola secreta"
                        //salvez in recv_sv stringul pentru a putea fi folosit 
                        //mai sus pentru a stii cand sa trimit parola secreta
                        if (strncmp(buffer, "Trimite", 7) == 0){
                            strcpy(recv_sv, buffer);  
                        }
                      

                }
                buffer[0] = '\0';	
			}
		} 	
    }
    fclose(fw);
    return 0;
}


