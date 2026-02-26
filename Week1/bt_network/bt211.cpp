#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

const int PORT = 11111;
const int BUFFER_SIZE = 4096;

using namespace std;

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0); // Tầng 4; SOCK_STREAM = yêu cầu sủ dụng TCP
    if (server_fd < 0)
    {
        return 1;
    }

    // Tầng mạng, nơi cấu hình cho Layer 3;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Lấy tất cả gói tin có địa chỉ IP này: LAN, WIFI, LOCALHOST

    address.sin_port = htons(PORT); // Chuyển đổi số PORT sang dạng mà mạng hiểu

    if (bind (server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) // Socket vào IP và PORT
    {
        return 1;
    }

    if (listen(server_fd, 3) < 0) // Socket muốn nhận tín hiệu và chỉ chấp nhận 3 yêu cầu trong hàng đợi
    {
        return 1;
    }
    cout << "Listening" << endl;

    // Nhận kết nối từ trình duyệt và phản hồi
    while (true)
    {
        cout << "Đang chạy " << endl;
        client_fd = accept(server_fd, (struct sockaddr *)&address, &addr_len);
        if (client_fd < 0)
        {
            continue;
        }

        cout << "Sever: " << server_fd << " ; Client: " << client_fd << endl;

        char buffer[BUFFER_SIZE] = {0};
        read(client_fd, buffer, BUFFER_SIZE);
        cout << buffer << endl;

        string response = "HTTP/1.0 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "\r\n"
                            "Hello";
                            
        write(client_fd, response.c_str(), response.length());

        close(client_fd);
    }
    close(server_fd);

    return 0;
}