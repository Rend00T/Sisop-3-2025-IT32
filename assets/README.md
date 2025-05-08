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
### Main
Buat memulai proses daemon, menghubungkan Client, dan menerima perintah/input dari Client.
```
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
```
## B. Image Client
Client berfungsi ngirim data file ke Server, sekaligus jadi tampilan utama program agar pengguna bisa melakukan decrypt file.
### Void Menu
Untuk tampilan menu.
```
void tampilkan_menu() {
    printf("\n===== MENU MENU BUKAN MENU MAKAN =====\n");
    printf("1. Kirim file untuk didekripsi\n");
    printf("2. Ambil file jpeg dari server\n");
    printf("3. Keluar\n");
    printf("Pilihanmu: ");
}
```
### Main
Untuk mengirim perintah ke Server
- Perintah 1: Mengambil data dari input, lalu mengirimkannya ke Server.
- Perintah 2: Meminta file jpeg dari Server, untuk disimpan ke Client.
- Perintah 3: Menutup program.
Error handling:
- Jika Server belum berjalan, maka Perintah 1 dan 2 tidak bisa dijalankan, akan ada pemberitahuan "Tidak bisa terhubung ke Server!".
- Jika input tidak dikenal, maka akan ada pemberitahuan "Input tidak dikenal. Coba lagi ya!".
```
int main() {
    char pilihan[128];
    while (1) {
        tampilkan_menu();
        fgets(pilihan, sizeof(pilihan), stdin);
        int opsi = atoi(pilihan);

        if (opsi == 1) {
            printf("Masukkan nama file input-nya: ");
            fgets(pilihan, sizeof(pilihan), stdin);
            pilihan[strcspn(pilihan, "\n")] = 0;

            int soket = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in alamat_server;
            alamat_server.sin_family = AF_INET;
            alamat_server.sin_port = htons(PORT);
            inet_pton(AF_INET, "127.0.0.1", &alamat_server.sin_addr);

            if (connect(soket, (struct sockaddr *)&alamat_server, sizeof(alamat_server)) < 0) {
                printf("Tidak bisa terhubung ke server!\n");
                continue;
            }

            char pesan[256];
            snprintf(pesan, sizeof(pesan), "DECRYPT %s\n", pilihan);
            send(soket, pesan, strlen(pesan), 0);

            char penampung[UKURAN_PENAMPUNG] = {0};
            int terbaca = read(soket, penampung, UKURAN_PENAMPUNG);
            printf("Respon: %s\n", penampung);

            close(soket);

        } else if (opsi == 2) {
            printf("Nama file jpeg yang mau diambil: ");
            fgets(pilihan, sizeof(pilihan), stdin);
            pilihan[strcspn(pilihan, "\n")] = 0;

            int soket = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in alamat_server;
            alamat_server.sin_family = AF_INET;
            alamat_server.sin_port = htons(PORT);
            inet_pton(AF_INET, "127.0.0.1", &alamat_server.sin_addr);

            if (connect(soket, (struct sockaddr *)&alamat_server, sizeof(alamat_server)) < 0) {
                printf("Tidak bisa terhubung ke server!\n");
                continue;
            }

            char pesan[256];
            snprintf(pesan, sizeof(pesan), "DOWNLOAD %s\n", pilihan);
            send(soket, pesan, strlen(pesan), 0);

            unsigned char penampung[UKURAN_PENAMPUNG];
            int terbaca = read(soket, penampung, UKURAN_PENAMPUNG);

            if (terbaca > 0 && strncmp((char *)penampung, "ERROR", 5) != 0) {
                char lokasi[256];
                snprintf(lokasi, sizeof(lokasi), "client/%s", pilihan);
                FILE *berkas = fopen(lokasi, "wb");
                fwrite(penampung, 1, terbaca, berkas);
                fclose(berkas);
                printf("Berhasil disimpan di %s\n", lokasi);
            } else {
                printf("Respon: %s\n", penampung);
            }

            close(soket);

        } else if (opsi == 3) {
            int soket = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in alamat_server;
            alamat_server.sin_family = AF_INET;
            alamat_server.sin_port = htons(PORT);
            inet_pton(AF_INET, "127.0.0.1", &alamat_server.sin_addr);
            if (connect(soket, (struct sockaddr *)&alamat_server, sizeof(alamat_server)) >= 0) {
                send(soket, "EXIT\n", 5, 0);
                close(soket);
            }
            printf("Program dihentikan.\n");
            break;
        } else {
            printf("Input tidak dikenal. Coba lagi ya!\n");
        }
    }
    return 0;
}
```

# Soal 2
Solved by. 061_Raihan Fahri Ghazali 
## A. Download and Shared Memory

