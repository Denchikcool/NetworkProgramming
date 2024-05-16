#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <time.h>
using namespace std;

#define BUFLEN 1024

int main(int argc, char *argv[])
{
    int sock, length;
    struct sockaddr_in servAddr, clientAdd;
    char buf[BUFLEN];
    char names[BUFLEN];
    bool continueGame = true;
    
    fd_set input_set;
    struct timeval timeout;

    if (argc < 4)
    {
        printf(" ВВЕСТИ Client IP порт свое имя\n");
        exit(1);
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("He могу получить socket\n");
        exit(1);
    }

    bzero((char *)&servAddr, sizeof(servAddr));
    inet_pton(AF_INET, argv[1], &servAddr.sin_addr);
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[2]));
    length = sizeof(servAddr);
    
    bzero((char*)&clientAdd, sizeof(clientAdd));
    
    clientAdd.sin_family = AF_INET;
    clientAdd.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAdd.sin_port = 0;
    
    if(bind(sock, (struct sockaddr *)&clientAdd, sizeof(clientAdd))){
    	perror("Клиент не имеет порта");
    	exit(1);
    }

    if (sendto(sock, (const char *)argv[3], strlen(argv[3]) + 1, 0, (struct sockaddr *)&servAddr, length) < 0)
    {
        perror("Проблемы с sendto . \n");
        exit(1);
    }

    while (continueGame)
    {
    	bzero(names, sizeof(BUFLEN));
    	bzero(buf, sizeof(BUFLEN));
    	if (recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *)&clientAdd, (socklen_t *)&length) < 0)
    	{
        	perror("Плохой socket клиента.");
        	exit(1);
    	}
    	
    	
    	if ((strcmp(buf, "Вы решили завершить игру. До свидания!") == 0) || 
        (strcmp(buf, "Лобби заполнено. Невозможно подключиться.") == 0) ||
        (strcmp(buf, "Вы решили закончить игру, досвидания!") == 0))
        {
            printf("%s\n", buf);
            break;
        }
        

        if(strcmp(buf, "Добро пожаловать в игровое лобби по игре BlackJack! Введите символ, хотите ли вы играть? (y/n): ") == 0){
        	printf("%s\n", buf);
        	bzero(buf, sizeof(BUFLEN));
		
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		
		FD_ZERO(&input_set);
		FD_SET(fileno(stdin), &input_set);
		
		int ready = select(fileno(stdin) + 1, &input_set, NULL, NULL, &timeout);
		
		if(ready == -1){
			perror("Error with select()");
			exit(EXIT_FAILURE);
		}
		else if(ready == 0){
			if (recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *)&clientAdd, (socklen_t *)&length) < 0){
				perror("Плохой socket клиента.");
        			exit(1);
			}

			if(strcmp(buf, "Вы находились на сервере более 5 секунд без действий. До свидания!") == 0){
				printf("%s\n", buf);
			}
			bzero(buf, sizeof(BUFLEN));
			exit(EXIT_SUCCESS);
		}
		else{
			fgets(buf, BUFLEN, stdin);
			buf[strcspn(buf, "\n")] = '\0';
			if(sendto(sock, buf, BUFLEN, 0, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0){
				perror("Не удалось отправить серверу сообщение.");
				exit(1);
			}
		}
		
        }
        if(strcmp(buf, "Вы подключились на сервер. Ожидайте") == 0){	
        	printf("%s\n", buf);
        }
        if(strstr(buf, "Вы подключились на сервер.") != NULL){	
        	printf("%s\n", buf);
        }
        if(strcmp(buf, "Игра началась!") == 0){
        	printf("\033[H\033[J");
        	printf("%s\n", buf);
        }
        if(strcmp(buf, "Вы хотите сыграть ещё раз? (y/n)") == 0){
        	printf("%s\n", buf);
        	bzero(buf, sizeof(BUFLEN));
        	fgets(buf, BUFLEN, stdin);
		buf[strcspn(buf, "\n")] = '\0';
		if(sendto(sock, buf, strlen(buf) + 1, 0, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0){
			perror("Не удалось отправить серверу сообщение.");
			exit(1);
		}
        }
        if((strstr(buf, " перебрал карты!") != NULL) || (strstr(buf, " победил!") != NULL) || (strstr(buf, " проиграл!") != NULL) || (strstr(buf, " сыграл вничью!") != NULL)){
        	printf("%s\n", buf);
        }
        if((strstr(buf, "@ ") != NULL) || (strstr(buf, "- ") != NULL) || (strstr(buf, "XX") != NULL)){
        	printf("%s\n", buf);
        }
        if(strstr(buf, " , ты хочешь сделать ход? (y/n): ")){
        	printf("%s\n", buf);
        	bzero(buf, sizeof(BUFLEN));
        	fgets(buf, BUFLEN, stdin);
		buf[strcspn(buf, "\n")] = '\0';
		if(sendto(sock, buf, BUFLEN, 0, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0){
			perror("Не удалось отправить серверу сообщение.");
			exit(1);
		}
        }
        
        if(strcmp(buf, "Вы решили закончить игру, до свидания!") == 0){
        	printf("%s\n", buf);
        	break;
        }
        if(strcmp(buf, "Ожидайте подключения других игроков!") == 0){
        	printf("%s\n", buf);
        }
        if(strcmp(buf, "Лобби закрывается. Никто не хочет играть.") == 0){
        	printf("%s\n", buf);
        	break;
        }
        
        if(strcmp(buf, "Все решили играть снова, начнем!") == 0){
        	printf("\033[H\033[J");
        	printf("%s\n", buf);
        }
        
        if(strstr(buf, "От сервера отключился игрок: ") != NULL){
        	printf("%s\n", buf);
        }
        
        if(buf[0] == '9') {
    		char playerName[BUFLEN];
    		char* ptr = strstr(buf, "9");
    		if (ptr != NULL) {
        		snprintf(playerName, sizeof(playerName), "%s", ptr + 1);
        		printf("На сервер подключился новый игрок: %s\n", playerName);
    		}
	}
	if(buf[0] == '8') {
    		char playerName[BUFLEN];
    		char* ptr = strstr(buf, "8");
    		if (ptr != NULL) {
        		snprintf(playerName, sizeof(playerName), "%s", ptr + 1);
        		printf("От сервера отключился игрок: %s\n", playerName);
    		}
	}
        
    }

    close(sock);
    return 0;
}



