#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFLEN 81

int main(int argc, char* argv[]) {
    int sockMain, length, msgLength;
    struct sockaddr_in servAddr, clientAddr;
    char buf[BUFLEN];
    char connection[BUFLEN];

    // Хранение IP-адресов клиентов и количество их подключений
    struct {
        char ip[INET_ADDRSTRLEN];
        int connections;
    } clients[10]; // Максимальное количество клиентов

    int num_clients = 0;

    if ((sockMain = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("Сервер не может открыть socket для UDP."); 
        exit(1);
    }

    bzero((char*)&servAddr, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servAddr.sin_port = 0;

    if (bind(sockMain, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) { 
        perror("Связывание сервера неудачно."); 
        exit(1);
    }

    length = sizeof(servAddr);
    if (getsockname(sockMain, (struct sockaddr*)&servAddr, &length) < 0) { 
        perror("Вызов getsockname неудачен."); 
        exit(1);
    }

    printf("СЕРВЕР: номер порта - %d\n", ntohs(servAddr.sin_port)); 

    for (;;) {
        length = sizeof(clientAddr);
        bzero(buf, BUFLEN);  

        if ((msgLength = recvfrom(sockMain, buf, BUFLEN, 0, (struct sockaddr *)&clientAddr, &length)) < 0) { 
            perror("Плохой socket клиента."); 
            exit(1);
        }

        // Проверка, есть ли уже такой клиент
        int client_index = -1;
        for (int i = 0; i < num_clients; ++i) {
            if (strcmp(inet_ntoa(clientAddr.sin_addr), clients[i].ip) == 0) {
                client_index = i;
                break;
            }
        }

        // Если клиент новый, отправляем сообщение о подключении и добавляем его в список клиентов
        if (client_index == -1) {
            // Проверяем, если уже максимальное количество клиентов
            if (num_clients >= 10) {
                printf("Сервер полон. Невозможно подключить новых клиентов.\n");
                continue;
            }
            sprintf(connection, "Клиент подключен");
            sendto(sockMain, connection, BUFLEN, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
            
            strcpy(clients[num_clients].ip, inet_ntoa(clientAddr.sin_addr));
            clients[num_clients].connections = 1;
            num_clients++;
        } else {
            // Если клиент уже подключен, пропускаем его запросы
            continue;
        }

        printf("SERVER: IP адрес клиента: %s\n", inet_ntoa(clientAddr.sin_addr)); 
        printf("SERVER: PORT клиента: %d\n", ntohs(clientAddr.sin_port)); 
        printf("SERVER: Длина сообщения - %d\n", msgLength);
        printf("SERVER: Сообщение: %s\n\n", buf); 
        
        buf[3] = '4';
        if (sendto(sockMain, buf, BUFLEN, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
            printf("Не удалось передать сообщение\n");
            exit(1);
        }
    }

    close(sockMain);
    return 0;
}

