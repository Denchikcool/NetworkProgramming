#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define NUM 5
#define CLIENTS 5

typedef struct Info {
	int *sock;
	int port;
	char *ip_address;
} Info;

int server_sock;
pthread_mutex_t mutex_file;

int create_a_socket(int *sock) {
	struct sockaddr_in server;

	if ( (*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		return 1; 
	}

	server.sin_family = AF_INET;
	server.sin_port = 0;
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(*sock, (struct sockaddr *) &server, sizeof(server)) == -1) {
		perror("cannot bind socket"); 
		return 1;
	}

	socklen_t len = sizeof(server);
	getsockname(*sock, &server, &len);

	printf("Server port: %d\n", ntohs(server.sin_port));

	return 0;
}


void client_handle(int *client_sock, char *ip_address, int port) {
	int end_buffer;
	char buffer[1024];
	int local_write_to_file = 1;  
  
	srand(time(NULL));
	int temp_letter = rand() % 26 + 65;
	char char_to_send = (char)temp_letter;

	for (int i = 0; i < NUM; i++) {
		bzero(&buffer, sizeof(buffer));
		end_buffer = recv(*client_sock, &buffer, sizeof(buffer), 0);  

		pthread_mutex_lock(&mutex_file);

		if (end_buffer <= 0) {
			local_write_to_file = 0; 
		}
    
		if (local_write_to_file) {
			FILE *output_file = fopen("./information.txt", "a");
			if (output_file != NULL) {
				fprintf(output_file, "Client address: %s\n", ip_address);
				fprintf(output_file, "Client port: %d\n", port);
        
				if (end_buffer > 0) {
					buffer[end_buffer] = '\0';
					fprintf(output_file, "Client: %s\n", buffer);
					buffer[0] = char_to_send;
					fprintf(output_file, "Changed to: %s\n", buffer);
				}
        
				fprintf(output_file, "=========================================\n");

				fclose(output_file);
			}
		}

		pthread_mutex_unlock(&mutex_file);
    
		if (local_write_to_file) {
			send(*client_sock, &buffer, sizeof(buffer), 0);
		}
	}
}

void sig_handler(int signo) {
	if (signo == SIGINT) {
		pthread_exit(0);
		pthread_mutex_destroy(&mutex_file);

		close(server_sock);

		exit(1);
	}
}

void *thread_func(void* args) {
	Info* temp_struct = (Info*)args;

	client_handle(temp_struct->sock, temp_struct->ip_address, temp_struct->port);

	free(temp_struct->sock);
	free(temp_struct->ip_address);
	free(temp_struct);

}

int main(int argc, char *argv[]) {
	int client_sock;
	int *p_client_sock;
	int client_len;
	struct sockaddr_in client;
	pthread_t thread;
	Info *client_info;

	if (create_a_socket(&server_sock) == 1) {
		return 1;
	}

    if ((listen(server_sock, CLIENTS)) != 0) { 
        perror("listen failed"); 
        return 1; 
    }

	client_len = sizeof(client);

	signal(SIGINT, sig_handler);
	signal(SIGPIPE, SIG_IGN);

	pthread_mutex_init(&mutex_file, NULL);

	while (1) {
	
		
		client_sock = accept(server_sock, (struct sockaddr*)&client, &client_len);
		
		client_info = (Info*)calloc(1, sizeof(Info));

		p_client_sock = (int*)calloc(1, sizeof(int));
		*p_client_sock = client_sock;

		client_info->sock = p_client_sock;
		client_info->port = client.sin_port;
		client_info->ip_address = (char*)calloc(strlen(inet_ntoa(client.sin_addr)), sizeof(char));
		strcpy(client_info->ip_address, inet_ntoa(client.sin_addr));
		
		pthread_create(&thread, NULL, &thread_func, (void*)client_info);
		pthread_detach(thread);
	}

	return 0;
}


