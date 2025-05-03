#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

#define MAX_ORDERS 100
#define MAX_LINE_LENGTH 256
#define SHM_KEY 1234

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

void log_delivery(const char *agent, const char *nama, const char *alamat, const char *tipe) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) {
        perror("❌ Gagal membuka delivery.log");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] %s package delivered to %s in %s\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec,
            agent, tipe, nama, alamat);

    printf("✅ Log dicatat: [%s] %s package delivered to %s\n", agent, tipe, nama);
    fclose(log);
}

void deliver_reguler(const char *nama_user) {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shmid < 0) {
        perror("❌ Gagal mendapatkan shared memory");
        exit(EXIT_FAILURE);
    }

    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("❌ Gagal attach shared memory");
        exit(EXIT_FAILURE);
    }

    int found = 0;
    for (int i = 0; i < shared_data->total_orders; i++) {
        Order *order = &shared_data->orders[i];
        if (strcmp(order->tipe, "Reguler") == 0 &&
            strcmp(order->nama_penerima, nama_user) == 0 &&
            order->delivered == 0) {

            order->delivered = 1;
            log_delivery(nama_user, order->nama_penerima, order->alamat, "Reguler");
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("❌ Tidak ditemukan order Reguler untuk %s atau sudah dikirim.\n", nama_user);
    }
}

void check_status(const char *nama_user) {
    FILE *log = fopen("delivery.log", "r");
    if (!log) {
        perror("❌ Gagal membuka delivery.log");
        return;
    }

    char line[256];
    int found = 0;

    while (fgets(line, sizeof(line), log)) {
        if (strstr(line, nama_user)) {
            char timestamp[100], agent[100], jenis[20], label[20], delivered[20], to[20], nama_penerima[50], in[20], alamat[100];

            sscanf(line,
                   "[%[^]]] [%[^]]] %s %s %s %s %[^ ] in %[^\n]",
                   timestamp, agent, jenis, label, delivered, to, nama_penerima, alamat);

            if (strcmp(nama_penerima, nama_user) == 0) {
                printf("Status for %s: Delivered by %s\n", nama_user, agent);
                found = 1;
                break;
            }
        }
    }

    fclose(log);

    if (!found) {
        printf("Status for %s: Pending\n", nama_user);
    }
}

void list_orders() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shmid < 0) {
        perror("❌ Gagal mendapatkan shared memory");
        exit(EXIT_FAILURE);
    }

    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("❌ Gagal attach shared memory");
        exit(EXIT_FAILURE);
    }

    printf("📋 Daftar Semua Pesanan:\n");

    for (int i = 0; i < shared_data->total_orders; i++) {
        Order *order = &shared_data->orders[i];
        const char *status = order->delivered ? "Delivered" : "Pending";

        printf("- %s (%s) [%s] : %s\n",
               order->nama_penerima,
               order->alamat,
               order->tipe,
               status);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 3 && strcmp(argv[1], "-deliver") == 0) {
        deliver_reguler(argv[2]);
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "-status") == 0) {
        check_status(argv[2]);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "-list") == 0) {
        list_orders();
        return 0;
    }

    printf("📥 Mengunduh file delivery_order.csv...\n");
    int result = system("curl -L -o delivery_order.csv 'https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9'");
    if (result != 0) {
        fprintf(stderr, "❌ Gagal mengunduh file.\n");
        return EXIT_FAILURE;
    }

    FILE *file = fopen("delivery_order.csv", "r");
    if (!file) {
        perror("❌ Gagal membuka file CSV");
        return EXIT_FAILURE;
    }

    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("❌ Gagal membuat shared memory");
        return EXIT_FAILURE;
    }

    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("❌ Gagal meng-attach shared memory");
        return EXIT_FAILURE;
    }

    shared_data->total_orders = 0;

    char line[MAX_LINE_LENGTH];
    fgets(line, sizeof(line), file); 

    while (fgets(line, sizeof(line), file) && shared_data->total_orders < MAX_ORDERS) {
        Order *order = &shared_data->orders[shared_data->total_orders];
        sscanf(line, "%[^,],%[^,],%[^\n]", order->nama_penerima, order->alamat, order->tipe);
        order->delivered = 0;
        shared_data->total_orders++;
    }

    fclose(file);
    printf("✅ Berhasil menyimpan %d order ke shared memory.\n", shared_data->total_orders);
    return 0;
}
