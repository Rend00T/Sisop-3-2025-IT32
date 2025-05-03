#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define PORT 8080
#define LOG_FILE "server/server.log"
#define DATABASE_DIR "server/database/"
#define BUFFER_SIZE 8192

void log_action(const char *source, const char *action, const char *info) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", t);
    fprintf(log, "[%s][%s]: [%s] [%s]\n", source, timebuf, action, info);
    fclose(log);
}

void *handle_client(void *arg) {
    int new_socket = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE] = {0};

    int read_size = read(new_socket, buffer, BUFFER_SIZE);
    if (read_size <= 0) {
        close(new_socket);
        return NULL;
    }

    char *cmd = strtok(buffer, " ");
    if (strcmp(cmd, "DECRYPT") == 0) {
        char *filename = strtok(NULL, "\n");
        if (!filename) {
            write(new_socket, "ERROR Invalid filename\n", 24);
            close(new_socket);
            return NULL;
        }

        char filepath[256];
        snprintf(filepath, sizeof(filepath), "client/secrets/%s", filename);

        FILE *f = fopen(filepath, "r");
        if (!f) {
            write(new_socket, "ERROR File not found\n", 22);
            log_action("Client", "DECRYPT", "Invalid file");
            close(new_socket);
            return NULL;
        }

        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *text = malloc(len + 1);
        fread(text, 1, len, f);
        text[len] = '\0';
        fclose(f);

        log_action("Client", "DECRYPT", text);

        for (int i = 0; i < len / 2; i++) {
            char temp = text[i];
            text[i] = text[len - i - 1];
            text[len - i - 1] = temp;
        }

        size_t out_len = len / 2;
        unsigned char *bin = malloc(out_len);
        for (size_t i = 0; i < out_len; i++) {
            sscanf(text + 2 * i, "%2hhx", &bin[i]);
        }
        free(text);

        time_t now = time(NULL);
        char out_name[64];
        snprintf(out_name, sizeof(out_name), "%ld.jpeg", now);

        char out_path[256];
        snprintf(out_path, sizeof(out_path), DATABASE_DIR "%s", out_name);

        FILE *outf = fopen(out_path, "wb");
        fwrite(bin, 1, out_len, outf);
        fclose(outf);

        log_action("Server", "SAVE", out_name);

        char response[128];
        snprintf(response, sizeof(response), "BRHAZIL %s\n", out_name);
        write(new_socket, response, strlen(response));
        free(bin);

    } else if (strcmp(cmd, "DOWNLOAD") == 0) {
        char *filename = strtok(NULL, "\n");
        if (!filename) {
            write(new_socket, "ERROR Invalid filename\n", 24);
            close(new_socket);
            return NULL;
        }

        char filepath[256];
        snprintf(filepath, sizeof(filepath), DATABASE_DIR "%s", filename);

        FILE *f = fopen(filepath, "rb");
        if (!f) {
            write(new_socket, "ERROR File not found\n", 22);
            log_action("Client", "DOWNLOAD", filename);
            log_action("Server", "UPLOAD", "Failed");
            close(new_socket);
            return NULL;
        }

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        unsigned char *data = malloc(fsize);
        fread(data, 1, fsize, f);
        fclose(f);

        write(new_socket, data, fsize);
        log_action("Client", "DOWNLOAD", filename);
        log_action("Server", "UPLOAD", filename);
        free(data);

    } else if (strcmp(cmd, "EXIT") == 0) {
        log_action("Client", "EXIT", "Client requested to exit");
    } else {
        write(new_socket, "ERROR Unknown command\n", 23);
    }

    close(new_socket);
    return NULL;
}

int main() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    setsid();

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        pthread_t t;
        pthread_create(&t, NULL, handle_client, pclient);
        pthread_detach(t);
    }

    return 0;
}
