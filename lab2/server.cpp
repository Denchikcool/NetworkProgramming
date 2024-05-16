#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

void handleClient(int clientSocket) {
    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(clientSocket, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytesRead);
        write(clientSocket, buffer, bytesRead);
        printf("\n");
    }

    if (bytesRead == -1) {
        perror("Ошибка чтения из сокета");
    }

    close(clientSocket);
    exit(0);
}

void sigchldHandler(int signal) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        std::cout << "Завершен дочерний процесс с PID: " << pid << std::endl;
    }
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Ошибка создания сокета");
        exit(1);
    }
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(0);  
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Ошибка привязки сокета");
        exit(1);
    }
    if (listen(serverSocket, 5) == -1) {
        perror("Ошибка прослушивания на сокете");
        exit(1);
    }
    // получаем рандом порт
    socklen_t len = sizeof(serverAddress);
    getsockname(serverSocket, (struct sockaddr*)&serverAddress, &len);
    int port = ntohs(serverAddress.sin_port);
    std::cout << "Порт сервера: " << port << std::endl;
    // получаем айпишник
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(serverAddress.sin_addr), ip, INET_ADDRSTRLEN);
    std::cout << "IP адрес сервера: " << ip << std::endl;

    struct sigaction sa{};
    sa.sa_handler = sigchldHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("Ошибка установки обработчика сигнала SIGCHLD");
        exit(1);
    }

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == -1) {
            perror("Ошибка принятия подключения");
            exit(1);
        }

        pid_t childPid = fork();
        if (childPid == -1) {
            perror("Ошибка создания дочернего процесса");
            exit(1);
        } else if (childPid == 0) {
            close(serverSocket);
            handleClient(clientSocket);
        }

        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}
