#include <iostream>
#include <string>

#include <vector>
#include <sstream>

#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h> // open()
#include <unistd.h> // dup2(); close();

#include <algorithm> // find;

using namespace std;

// Cấu hình màu:
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

// Ham tach chuoi
vector<string> tach_chuoi(const string& line)
{
    vector<string> tokens;
    istringstream iss(line); // Chuyển chuỗi thành luồng;
    string token;

    while (iss >> token) // Thay vì cin >> thì ở đây dùng iss >>;
    {
        tokens.push_back(token);
    }

    return tokens;
}

void execute(const vector<string>& tokens)
{
    int stdin_fd = -1, stdout_fd = -1, stderr_fd = -1;
    vector<string> cmd;

    for (int i = 0; i < tokens.size(); i++)
    {
        /*
            O_WRONLY: Open va Write
            O_CREAT: Tao file
            O_TRUNC: Xoa noi dung cu neu file da ton tai
            0644: Quyen file: Chu: doc/ghi; Khach: chi doc
            O_RDONLY: chi doc
            O_: cờ của hàm open.
            0: hệ bát phân
            4: đọc, 2: ghi, 1: thực thi.
            => 0644: hệ 8 + đọc ghi + chỉ đọc + chỉ đọc = | chủ | nhóm | người khác |
        */
        if (tokens[i] == ">" && i + 1 < tokens.size())
        {
            stdout_fd = open(tokens[i+1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            i++;
        }
        else if (tokens[i] == "<" && i+1 < tokens.size())
        {
            stdin_fd = open(tokens[i+1].c_str(), O_RDONLY);
            i++;
        }
        else if (tokens[i] == "2>" && i+1 < tokens.size())
        {
            stderr_fd = open(tokens[i+1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            i++;
        }
        else 
        {
            cmd.push_back(tokens[i]);
        }
    }

    // Lệnh build-in
    if (cmd[0] == "exit")
    {
        cout << YELLOW "Exited." RESET << endl;
        exit(0);
    }
    if (cmd[0] == "cd")
    {
        if (cmd.size() < 2)
        {
            cerr << RED "Muốn đi đâu???" RESET << endl; // Nhập thiếu đường dẫn.
        }
        else
        {
            int tmp = chdir(cmd[1].c_str());
            if (tmp != 0)
            /*
            chdir: systemcall để đổi thư mục hiện tại - hàm chính khi gọi lệnh cd;
            c_str(): chuyển string sang char* vì hàm chdir yêu cầu thế;
            */
            {
                cerr << RED "Không thể chuyển thư mục: " RESET << cmd[1] << endl;
            }
            else
            {
                cout << "Di chuyển thành công đến: " <<BOLD GREEN "/" << cmd[1] << RESET << endl;
            }
        }
        return;
    }

    vector<char*> args;
    for (auto& t : cmd)
    {
        args.push_back(const_cast<char*>(t.c_str()));
    }
    args.push_back(nullptr); // execvp yeu cau 1 mang co phan tu cuoi la nullptr;

    /*
    hàm fork tạo ra 1 proc con thuộc proc cha, 
    sau khi tạo xong nó gán cho proc con 1 pid khác 0;
    khi này proc chính của hàm main có pid ví dụ 9876, có giá trị biến pid là 1234;
    giá trị 1234 này là pid của proc con;
    trong proc con biến pid có giá trị là 0, mặc định là như thế do hàm fork quy định.
    */
    pid_t pid = fork();
    if (pid == 0)
    {
        /*
        Áp dụng điều hướng trước khi fork();
        dup2(fd, number) có nghĩa là đem toàn bộ input vào đầu vào
        hoặc đem toàn bộ output mà chương trình trả về ném vào file.
        */
        if (stdin_fd != -1)
        {
            dup2(stdin_fd, STDIN_FILENO); // STDIN_FILENO = STD IN File Number = 0;
        }
        if (stdout_fd != -1)
        {
            dup2(stdout_fd, STDOUT_FILENO);
        }
        if (stderr_fd != -1)
        {
            dup2(stderr_fd, STDERR_FILENO);
        }

        execvp(args[0], args.data());

        cerr << RED " Command not found: " RESET << args[0] << endl;
        exit(1);
    }
    else
    {
        waitpid(pid, nullptr, 0);
    }
}

void exepip(vector<string>& left, vector<string>& right)
{
    int pipefd[2];
    pipe(pipefd); // tao pipe

    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        vector<char*> args;
        for (auto& t : left)
        {
            args.push_back(const_cast<char*> (t.c_str()));
        }
        args.push_back(nullptr);
        execvp(args[0], args.data());
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);

        vector<char*> args;
        for (auto& t : right)
        {
            args.push_back(const_cast<char*> (t.c_str()));
        }
        args.push_back(nullptr);
        execvp(args[0], args.data());
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
}

int main() 
{
    string line;

    while (true)
    {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        cout << BOLD GREEN << cwd << "$> " RESET;

        getline(cin, line);

        // Tach chuoi;
        vector<string> tokens = tach_chuoi(line);

        auto it = find(tokens.begin(), tokens.end(), "|");
        if (it != tokens.end())
        {
            vector<string> left(tokens.begin(), it);
            vector<string> right(it + 1, tokens.end());
            exepip(left, right);
        }
        else 
        {
            execute(tokens);
        }
    }

    return 0;
}