#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main(int argc, char *argv[]) {

    if (argc < 3) {
        std::cerr << "Использование: " << argv[0] << " <ip-адрес-сервера> <порт>" << std::endl;
        return 1;
    }

    const char *server_ip = argv[1];
    int port = std::stoi(argv[2]);
    
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Ошибка создания сокета");
        exit(1);
    }

    
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(port); 
    if(inet_pton(AF_INET, server_ip, &(serverAddress.sin_addr)) <= 0) {
        perror("Ошибка адреса сервера");
        exit(1);
    }

   
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Ошибка подключения к серверу");
        exit(1);
    }

    
    
    //while(1)
    char buffer[1024];
    ssize_t bytesRead;
    for(int i = 1; i <= 5; i++)
    {
        const char* message = std::to_string(i).c_str();
        ssize_t bytesWritten = write(clientSocket, message, strlen(message));
        if (bytesWritten == -1) {
            perror("Ошибка записи в сокет");
            exit(1);
        }
        bytesRead = read(clientSocket, buffer, sizeof(buffer));
        write(STDOUT_FILENO, buffer, bytesRead);
        printf("\n");
        sleep(2);
    }

    close(clientSocket);
    return 0;
}
