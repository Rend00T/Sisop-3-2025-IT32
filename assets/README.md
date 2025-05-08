# Sisop-3-2025-IT32
- Ananda Fitri Wibowo (5027241057)
- Raihan Fahri Ghazali (5027241061)

| No | Nama                   | NRP         |
|----|------------------------|-------------|
| 1  | Ananda Fitri Wibowo    | 5027241057  |
| 2  | Raihan Fahri Ghazali   | 5027241061  |

# Soal 1
Solved by. 057_Ananda Fitri Wibowo
## A. Image Server
Server ini berfungsi untuk decrypt file dan ngirim filenya ke client.
### Define
- Di sini saya define port 8080 buat koneksi server dan client.
- Log file untuk nyimpen file log.
- Database untuk folder database yang isinya file-file jpeg hasil decrypt.
```
#define PORT 8080
#define LOG_FILE "server/server.log"
#define DATABASE_DIR "server/database/"
#define BUFFER_SIZE 8192
```

### Void Logging
Untuk mencatat aktivitas server dan client ke file log.
```
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
```

### Void Handle Client
Untuk merespon aktivitas/permintaan dari client.
#### Menerima Socket dan Data dari Client
```
    int new_socket = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE] = {0};

    int read_size = read(new_socket, buffer, BUFFER_SIZE);
    if (read_size <= 0) {
        close(new_socket);
        return NULL;
    }
```
#### Respon untuk Opsi 1 di Menu
Respon untuk decrypt file dari Client, menyimpannya (file jpeg) ke Database, menulis aktivitas ke dalam Log.
```
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
    }
```
#### Respon untuk Perintah 2 di Menu
Mengirim file jpeg dari database ke Client, menulis aktivitas ke Log.
```
     else if (strcmp(cmd, "DOWNLOAD") == 0) {
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

    }
```
#### Respon untuk Opsi 3 di Menu
Respon untuk keluar dari program Client (menu).
```
     else if (strcmp(cmd, "EXIT") == 0) {
        log_action("Client", "EXIT", "Client requested to exit");
    }
```
#### Error Handling
Respon untuk perintah/opsi tidak dikenal.
```
     else {
        write(new_socket, "ERROR Unknown command\n", 23);
    }
```
#### Keluar dari Program
```
    close(new_socket);
    return NULL;
```
### 
