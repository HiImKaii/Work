#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>   // Cho open()

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_PIPES 10

// ============================================================================
// PHẦN 1: CÁC HÀM CƠ BẢN
// ============================================================================

/**
 * Hàm parse_input: Tách chuỗi lệnh thành mảng các đối số
 * Ví dụ: "ls -l -a" -> args[0]="ls", args[1]="-l", args[2]="-a", args[3]=NULL
 */
int parse_input(char *line, char **args) {
    int i = 0;
    args[i] = strtok(line, " \t");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " \t");
    }
    return i; // Trả về số lượng đối số
}

/**
 * Hàm execute_simple: Thực thi một lệnh đơn giản (không có redirection hay pipe)
 * Đây là nền tảng cơ bản nhất của Shell
 */
void execute_simple(char **args) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    
    if (pid == 0) {
        // TIẾN TRÌNH CON: Thực thi lệnh
        execvp(args[0], args);
        // Nếu đến được đây nghĩa là lệnh không tồn tại
        perror(args[0]);
        exit(EXIT_FAILURE);
    } else {
        // TIẾN TRÌNH CHA: Đợi con chạy xong
        wait(NULL);
    }
}

// ============================================================================
// PHẦN 2: HỖ TRỢ REDIRECTION (>, <, >>, 2>)
// ============================================================================

/**
 * Hàm find_redirection: Tìm vị trí của các ký tự redirection trong mảng args
 * Trả về loại redirection tìm thấy, hoặc 0 nếu không có
 */
typedef struct {
    int type;           // 0: không có, 1: >, 2: >>, 3: <, 4: 2>
    int position;       // Vị trí trong mảng args
    char *filename;     // Tên file để redirect
} Redirection;

Redirection find_redirection(char **args, int arg_count) {
    Redirection redir = {0, -1, NULL};
    
    for (int i = 0; i < arg_count; i++) {
        if (args[i] == NULL) break;
        
        if (strcmp(args[i], ">>") == 0) {
            redir.type = 2;
            redir.position = i;
            redir.filename = args[i + 1];
            break;
        } else if (strcmp(args[i], ">") == 0) {
            redir.type = 1;
            redir.position = i;
            redir.filename = args[i + 1];
            break;
        } else if (strcmp(args[i], "<") == 0) {
            redir.type = 3;
            redir.position = i;
            redir.filename = args[i + 1];
            break;
        } else if (strcmp(args[i], "2>") == 0) {
            redir.type = 4;
            redir.position = i;
            redir.filename = args[i + 1];
            break;
        }
    }
    
    return redir;
}

/**
 * Hàm execute_with_redirection: Thực thi lệnh có kèm redirection
 */
void execute_with_redirection(char **args, Redirection redir) {
    // Cắt bỏ phần redirection ra khỏi args
    args[redir.position] = NULL;
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    
    if (pid == 0) {
        // TIẾN TRÌNH CON
        int fd;
        
        switch (redir.type) {
            case 1: // > (ghi đè)
                fd = open(redir.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open"); exit(EXIT_FAILURE); }
                dup2(fd, STDOUT_FILENO);  // Thay STDOUT bằng file
                close(fd);
                break;
                
            case 2: // >> (thêm vào cuối)
                fd = open(redir.filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd < 0) { perror("open"); exit(EXIT_FAILURE); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
                break;
                
            case 3: // < (đọc từ file)
                fd = open(redir.filename, O_RDONLY);
                if (fd < 0) { perror("open"); exit(EXIT_FAILURE); }
                dup2(fd, STDIN_FILENO);   // Thay STDIN bằng file
                close(fd);
                break;
                
            case 4: // 2> (redirect STDERR)
                fd = open(redir.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open"); exit(EXIT_FAILURE); }
                dup2(fd, STDERR_FILENO);  // Thay STDERR bằng file
                close(fd);
                break;
        }
        
        execvp(args[0], args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    } else {
        // TIẾN TRÌNH CHA
        wait(NULL);
    }
}

// ============================================================================
// PHẦN 3: HỖ TRỢ PIPELINE (|)
// ============================================================================

/**
 * Hàm count_pipes: Đếm số lượng pipe trong chuỗi lệnh
 */
int count_pipes(char *line) {
    int count = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '|') count++;
    }
    return count;
}

/**
 * Hàm execute_pipeline: Thực thi chuỗi lệnh có pipe
 * Ví dụ: "ls -l | grep txt | wc -l"
 */
void execute_pipeline(char *line) {
    char *commands[MAX_PIPES + 1];
    char *args[MAX_ARGS];
    int num_cmds = 0;
    
    // Tách chuỗi theo dấu |
    commands[num_cmds] = strtok(line, "|");
    while (commands[num_cmds] != NULL && num_cmds < MAX_PIPES) {
        num_cmds++;
        commands[num_cmds] = strtok(NULL, "|");
    }
    
    // Tạo mảng pipe
    // Với n lệnh, cần n-1 pipe
    int pipes[MAX_PIPES][2];
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe failed");
            return;
        }
    }
    
    // Fork và thực thi từng lệnh
    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            return;
        }
        
        if (pid == 0) {
            // TIẾN TRÌNH CON
            
            // Nếu KHÔNG phải lệnh đầu tiên, đọc từ pipe trước đó
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            
            // Nếu KHÔNG phải lệnh cuối cùng, ghi vào pipe tiếp theo
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Đóng tất cả các pipe trong tiến trình con
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Parse và thực thi lệnh hiện tại
            parse_input(commands[i], args);
            execvp(args[0], args);
            perror(args[0]);
            exit(EXIT_FAILURE);
        }
    }
    
    // TIẾN TRÌNH CHA: Đóng tất cả pipe
    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Đợi tất cả các tiến trình con hoàn thành
    for (int i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}

// ============================================================================
// HÀM MAIN - VÒNG LẶP CHÍNH CỦA SHELL
// ============================================================================

int main() {
    char line[MAX_LINE];
    char line_copy[MAX_LINE];
    char *args[MAX_ARGS];
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║       MINI SHELL - Aime Group Training                       ║\n");
    printf("║  Gõ lệnh Linux hoặc 'exit' để thoát                          ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    while (1) {
        // In dấu nhắc
        printf("\033[1;32mmyshell>\033[0m ");
        fflush(stdout);
        
        // Đọc lệnh
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            printf("\n");
            break;  // Ctrl+D để thoát
        }
        
        // Xóa ký tự xuống dòng
        line[strcspn(line, "\n")] = '\0';
        
        // Bỏ qua nếu dòng trống
        if (strlen(line) == 0) {
            continue;
        }
        
        // Lệnh built-in: exit
        if (strcmp(line, "exit") == 0) {
            printf("Tạm biệt! 👋\n");
            break;
        }
        
        // Lưu một bản copy để kiểm tra pipe
        strcpy(line_copy, line);
        
        // =============================================
        // KIỂM TRA VÀ XỬ LÝ PIPELINE
        // =============================================
        if (count_pipes(line) > 0) {
            execute_pipeline(line);
            continue;
        }
        
        // =============================================
        // PARSE VÀ KIỂM TRA REDIRECTION
        // =============================================
        int arg_count = parse_input(line, args);
        
        if (args[0] == NULL) {
            continue;
        }
        
        Redirection redir = find_redirection(args, arg_count);
        
        if (redir.type != 0) {
            // Có redirection
            execute_with_redirection(args, redir);
        } else {
            // Lệnh đơn giản
            execute_simple(args);
        }
    }
    
    return 0;
}
