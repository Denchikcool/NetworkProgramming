#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define NUM 5

int main(void) {
	int sock;
	int port;
	int end_buffer;
	int r;
	char buffer[1024];
	char server_address[1024];
	pthread_t send_handler;
	pthread_t recieve_handler;
	struct sockaddr_in server;
	socklen_t server_len;

	printf("Input ip address: ");
	fgets(server_address, sizeof(server_address), stdin);
	printf("Input server port: ");
	scanf("%d", &port);

	server_address[strcspn(server_address, "\n")] = 0; // delete newline from msg

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		return 1; 
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	//server.sin_addr.s_addr = inet_addr("127.0.0.1");
	inet_pton(AF_INET, server_address, &(server.sin_addr));

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        perror("connection with the server failed");
        return 1;
    }

	srand(time(NULL));
	r = (rand() % 5) + 1;

	for (int i = 0; i < NUM; i++) {
		bzero(&buffer, sizeof(buffer));
		sprintf(buffer, "%d", r);
		send(sock, &buffer, sizeof(buffer), 0);

		bzero(&buffer, sizeof(buffer));
		end_buffer = recv(sock, &buffer, sizeof(buffer), 0);

		buffer[end_buffer] = '\0';

		printf("Server: %s\n", buffer);

		sleep(r);
	}

	close(sock);

	return 0;
}

