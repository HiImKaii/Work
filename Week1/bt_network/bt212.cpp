#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

const int PORT = 8080;
const int BUFFER_SIZE = 4096;

void* xu_ly(void* val)
{
    int client_fd = (int)(intptr_t)val;

    char buffer[BUFFER_SIZE] = {0};
    read(client_fd, buffer, BUFFER_SIZE);
    cout << " Dang xu ly cli_fd: " << client_fd << endl;
    cout << buffer << endl;

    string response = "HTTP/1.0 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "Hello\r\n";

    write(client_fd, response.c_str(), response.length());
    close(client_fd);

    return NULL;
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in dia_chi;
    socklen_t len_dia_chi = sizeof(dia_chi);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Đảm bảo server tắt ngay để tái sử dụng PORT;

    dia_chi.sin_family = AF_INET;
    dia_chi.sin_addr.s_addr = INADDR_ANY;
    dia_chi.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&dia_chi, sizeof(dia_chi)) < 0)
    {
        return 1;
    }

    if (listen(server_fd, 10) < 0)
    {
        return 1;
    }

    cout << "Listening in PORT: " << PORT << endl;

    while(true)
    {
        client_fd = accept(server_fd, (struct sockaddr*)&dia_chi, &len_dia_chi);
        if (client_fd < 0)
        {
            continue;
        }
        cout << "Ket noi moi : " << client_fd << endl;

        pthread_t tid;
        pthread_create(&tid, NULL, xu_ly, (void*)(intptr_t)client_fd);
        pthread_detach(tid);
    }

    close(server_fd);

    return 0;
}