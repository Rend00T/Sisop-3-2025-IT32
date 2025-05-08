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

# Soal 2 - RushGo Delivery Management System 

Repositori ini berisi implementasi sistem manajemen pengiriman untuk perusahaan ekspedisi RushGo. Sistem terdiri dari dua program utama:

- `delivery_agent.c` → Mengurus pengiriman **Express** secara otomatis.
- `dispatcher.c` → Menangani pengiriman **Reguler**, pengecekan status, dan daftar pesanan.

---

## ✅ Penjelasan Berdasarkan Soal

---

###  A. Mengunduh File Order dan Menyimpannya ke Shared Memory

- Di awal program (`delivery_agent.c` dan `dispatcher.c`), file CSV diunduh otomatis menggunakan:

```c
system("curl -L -o delivery_order.csv 'https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9'");
```
Setelah diunduh, data dibaca per baris dan disimpan ke struktur shared memory:
```
typedef struct {
    char nama_penerima[50];
    char alamat[100];
    char tipe[10];
    int delivered;
} Order;

typedef struct {
    int total_orders;
    Order orders[MAX_ORDERS];
} SharedData;
```
Shared memory dibuat dengan shmget() dan di-attach dengan shmat():
```
int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
shared_data = (SharedData *)shmat(shmid, NULL, 0);
```

###  B. Pengiriman Bertipe Exprees
Pada file delivery_agent.c, dilakukan pemrosesan otomatis untuk pesanan bertipe Express. Tiga agen (AGENT A, AGENT B, AGENT C) dibuat sebagai thread terpisah:
```c
pthread_t agents[3];
pthread_create(&agents[0], NULL, agent_thread, "AGENT A");
pthread_create(&agents[1], NULL, agent_thread, "AGENT B");
pthread_create(&agents[2], NULL, agent_thread, "AGENT C");
```
Setiap thread menelusuri shared memory, mencari order Express yang belum dikirim:
```c
if (strcmp(order->tipe, "Express") == 0 && order->delivered == 0) {
    order->delivered = 1;
    log_delivery(agent, order->nama_penerima, order->alamat);
}
```
Log pengiriman dicatat ke dalam delivery.log dengan format:
```
[dd/mm/yyyy hh:mm:ss] [AGENT X] Express package delivered to [Nama] in [Alamat]
```

### C. Pengiriman Bertipe Reguler
Fungsi delivery_reguler() akan:
- mencari order dengan tope == "Reguler" dan nama_penerima == nama yang belum dikirim.
- menandainya sebagai telah dikirim dan mencatat ke log
```c
if (strcmp(order->tipe, "Reguler") == 0 &&
    strcmp(order->nama_penerima, nama_user) == 0 &&
    order->delivered == 0) {
    order->delivered = 1;
    log_delivery(nama_user, order->nama_penerima, order->alamat, "Reguler");
}
```
format log akan saperti ini : 
```
[dd/mm/yyyy hh:mm:ss] [AGENT <user>] Reguler package delivered to [Nama] in [Alamat]
```

### C. pengiriman Bertipe Reguler 
Pada file dispatcher.c pengiriman Reguler dilakukan secara manual dengan perintah:
```
./dispatcher -deliver [NamaPenerima]

```
Fungsi delivery_reguler akan :
- Mencari orderan dengan tipe == "Reguler" dan nama_penerima == nama yang belum dikirim
- Menandainya sebagai telah dikirim dan mencatat ke log
```
if (strcmp(order->tipe, "Reguler") == 0 &&
    strcmp(order->nama_penerima, nama_user) == 0 &&
    order->delivered == 0) {
    order->delivered = 1;
    log_delivery(nama_user, order->nama_penerima, order->alamat, "Reguler");
}
```
Format log
```
[dd/mm/yyyy hh:mm:ss] [AGENT <user>] Reguler package delivered to [Nama] in [Alamat]
```

### D. Mengecheck Status Pesanan 
Pengguna dapat mengecheck status pesanan via:
```
./dispatcher -status [NamaPenerima]
```
Fungsi check_status akan membuka delivery.log, lalu mencari apakah nama tersebut sudah benar
```
if (strcmp(nama_penerima, nama_user) == 0) {
    printf("Status for %s: Delivered by %s\n", nama_user, agent);
}
```
jika ditemukan di log, maka status dianggap Pending 
```
Status for [Nama]: Pending
```

### E. Melihat Daftar Semua Pesanan
Pengguna dapat menjalankan melalui 
```
./dispatcher -list
```

Fungsi list_orders() akan mencetak semua pesanan yang ada di shared memory:
```
printf("- %s (%s) [%s] : %s\n",
       order->nama_penerima,
       order->alamat,
       order->tipe,
       order->delivered ? "Delivered" : "Pending");
```
Akhirnya menghasilkan output seperti ini :
```
📋 Daftar Semua Pesanan:
- Budi (Jl. Mawar) [Express] : Delivered
- Siti (Jl. Anggrek) [Reguler] : Pending
```

# Soal 3 - The Lost Dungeon

Repositori ini berisi implementasi **permainan berbasis client-server untuk mensimulasikan petualangan di dalam dungeon misterius**. Sistem terdiri dari dua program utama:

- `dungeon.c` → Berperan sebagai **server** yang menangani semua logika permainan, termasuk pertempuran, toko senjata, dan inventory.
- `player.c` → Berperan sebagai **client** yang digunakan oleh pemain untuk berinteraksi dengan dungeon secara interaktif melalui menu.

Selain itu, sistem juga menggunakan `shop.c` dan `shop.h` sebagai modul pendukung untuk menyimpan daftar senjata dan efek passive yang tersedia di toko.

### A. Entering the Dungeon
Dungeon.c  akan berfungsi sebagai RPC server, menerima koneksi dari satu atau lebih client (player.c). player.c membuka koneksi ke dungeon.c menggunakan socket
```
int server_fd = socket(...); 
bind(...);
listen(...);
accept(...);
pthread_create(...);
```
Mendirikan server dungeon menggunakan socket TCP dan multithreading agar bisa melayani banyak client.
```
int sock = socket(...);
connect(...);
```
Menghubugkan Client ke dungeon server

### B. Sightseeing (Menu Interaktif)
Saat opsi "Show Player Stats" dipilih, player.c mengirim "SHOW_STATS" ke server. dungeon.c akan memprosesnya dan mengembalikan status player.
```
// player.c
send_command(sock, "SHOW_STATS");
```
```
// dungeon.c
else if (strcmp(buffer, "SHOW_STATS") == 0)
    get_player_stats(response);
```
```
// dungeon.c
void get_player_stats(char *response) {
    int idx = player.inventory[player.equipped];
    Weapon w = shop_weapons[idx];
    snprintf(response, 1024,
        "\n=== \033[1;36mPLAYER STATS\033[0m ===\n"
        "\033[1;33mGold\033[0m: %d | \033[1;32mEquipped Weapon\033[0m: %s | \033[1;31mBase Damage\033[0m: %d | \033[1;34mKills\033[0m: %d",
        player.gold, w.name, w.damage, player.kills);
}
```
Code diatas akan Menampilkan status player: Gold, senjata yang dipakai, base damage, dan jumlah musuh yang dikalahkan.

### C. Weapon Shop
Saat opsi "Shop" dipilih, player mengirim "SHOP" lalu memilih senjata untuk dibeli dengan "BUY_x".

```
// player.c
send_command(sock, "SHOP");
int w;
scanf("%d", &w);
sprintf(command, "BUY_%d", w);
send_command(sock, command);
```

```
// dungeon.c
else if (strcmp(buffer, "SHOP") == 0) {
    for (int i = 0; i < MAX_WEAPONS; i++) {
        snprintf(temp, sizeof(temp), "[%d] %s - Price: %d gold, Damage: %d", i + 1, shop_weapons[i].name, shop_weapons[i].price, shop_weapons[i].damage);
    }
}
```

```
// dungeon.c
else if (strncmp(buffer, "BUY_", 4) == 0) {
    int id = atoi(buffer + 4) - 1;
    if (player.gold >= shop_weapons[id].price) {
        player.gold -= shop_weapons[id].price;
        player.inventory[player.inventory_count++] = id;
    }
}
```
Menampilkan daftar senjata yang bisa dibeli beserta logic pembeliannya, menggunakan gold milik player.

### D. Handy Inventory
Saat opsi "View Inventory & Equip Weapon" dipilih, server akan menampilkan semua senjata yang dimiliki dan memberi opsi mengganti senjata aktif.
```
// player.c
send_command(sock, "INVENTORY");
int sel;
scanf("%d", &sel);
sprintf(command, "EQUIP_%d", sel);
send_command(sock, command);
```

```
// dungeon.c
else if (strcmp(buffer, "INVENTORY") == 0)
    show_inventory(response);

else if (strncmp(buffer, "EQUIP_", 6) == 0) {
    int eq = atoi(buffer + 6);
    player.equipped = eq;
}
```
```
// dungeon.c
void show_inventory(char *response) {
    for (int i = 0; i < player.inventory_count; i++) {
        Weapon w = shop_weapons[player.inventory[i]];
        snprintf(temp, sizeof(temp), "[%d] %s", i, w.name);
        if (i == player.equipped) strcat(response, " \033[1;33m(EQUIPPED)\033[0m");
    }
}
```
Menampilkan senjata yang dimiliki player dan mengganti senjata aktif.

### E. Battle Mode 
Saat opsi "Battle Mode" dipilih, client mengirim "BATTLE", lalu dapat menyerang ("attack") atau keluar ("exit").
```
// player.c
send_command(sock, "BATTLE");
while (1) {
    fgets(input, sizeof(input), stdin);
    send(sock, input, strlen(input), 0);
}
```

```
// dungeon.c
else if (strcmp(buffer, "BATTLE") == 0)
    start_new_enemy(response);
else if (strcmp(buffer, "attack") == 0 && in_battle)
    handle_attack(response);
else if (strcmp(buffer, "exit") == 0 && in_battle)
    in_battle = false;
```

```
// dungeon.c
void handle_attack(char *response) {
    int damage = w.damage + (rand() % 10);
    if (strstr(w.passive, "Crit")) damage *= 2;
    else if (strstr(w.passive, "Insta")) damage = enemy_hp;
    enemy_hp -= damage;
    if (enemy_hp <= 0) {
        player.gold += reward;
        player.kills++;
        start_new_enemy(next_enemy);
    }
}
```
Mode pertempuran dengan HP musuh acak. Player memberikan damage berdasarkan senjata dan passive. Musuh baru akan muncul setelah menang.
