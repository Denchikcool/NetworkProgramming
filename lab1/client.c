#include <sys/types.h> 
#include <sys/socket.h> 
#include <stdio.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <errno.h>
#include <stdlib.h>  
#include <string.h>  
#include <time.h>

#define BUFLEN 81

int main(int argc, char* argv[]){
  int sock, message, time_to_sleep;
  struct sockaddr_in servAddr, clientAddr;
  struct hostent *hp;
  char buf[BUFLEN];
  
  //srand(time(NULL));
  //time_to_sleep = 1 + rand()%5;
  
  if(argc < 5)
  {
    printf("Введите udpclient айпишник порт сообщение время задержки\n"); 
    exit(1);
  }
  
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  { 
    perror("Не могу получить socket\n"); 
    exit(1);
  }
  
  bzero((char*)&servAddr, sizeof(servAddr));
  servAddr.sin_family = AF_INET; 
  hp = gethostbyname(argv[1]);
  bcopy(hp->h_addr, (char*)&servAddr.sin_addr, hp->h_length);  
  
  servAddr.sin_port = htons(atoi(argv[2]));
  bzero((char*)&clientAddr, sizeof(clientAddr));
  
  clientAddr.sin_family = AF_INET; 
  clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  clientAddr.sin_port = 0;
  
  time_to_sleep = atoi(argv[4]);
  
  if (bind(sock, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0)  
  { 
    perror("Клиент не получил порт."); 
    exit(1);
  }
  
  printf("CLIENT: Готов к пересылке.\n"); 
  
  for(int i = 0; i < 100; i++)
  {
  
  if(sendto(sock, argv[3], strlen(argv[3]), 0, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) 
  { 
    perror("Проблемы с sendto. \n"); 
    exit(1);
  }
  sleep(time_to_sleep);
  

  bzero(buf, BUFLEN);
  int lenght = sizeof(servAddr);
  if((message = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr*)&servAddr, &lenght)) < 0)
  {
  	printf("Не получилось принять сообщение от пользователя\n");
  	exit(1);
  }
  
  printf("Ваше сообщение, измненное сервером - %s\nЗадержка у этого клиента = %d\n", buf, time_to_sleep);
  }
  
  printf("CLIENT: Пересылка завершена. Счастливо оставаться. \n"); 
  close(sock); 
  return 0;
}

