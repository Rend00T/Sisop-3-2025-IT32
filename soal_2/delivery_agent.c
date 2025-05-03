#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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

SharedData *shared_data;

void download_csv() {
    printf("Mengunduh file delivery_order.csv...\n");
    int result = system("curl -L -o delivery_order.csv 'https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9'");
    if (result != 0) {
        fprintf(stderr, "❌ Gagal mengunduh file CSV.\n");
        exit(EXIT_FAILURE);
    }
    printf("✅ File berhasil diunduh.\n");
}

void log_delivery(const char *agent, const char *nama, const char *alamat) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) {
        perror("❌ Gagal membuka delivery.log untuk ditulis");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] Express package delivered to %s in %s\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec,
            agent, nama, alamat);

    printf("Logged delivery to %s in %s by %s\n", nama, alamat, agent);
    fclose(log);
}

void *agent_thread(void *arg) {
    char *agent = (char *)arg;

    for (int i = 0; i < shared_data->total_orders; i++) {
        Order *order = &shared_data->orders[i];
        if (strcmp(order->tipe, "Express") == 0 && order->delivered == 0) {
            order->delivered = 1;
            log_delivery(agent, order->nama_penerima, order->alamat);
            sleep(1); 
        }
    }

    return NULL;
}

int main() {
    download_csv();

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

    shared_data = (SharedData *)shmat(shmid, NULL, 0);
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

    printf("✅ [dispatcher] Berhasil membaca %d order ke shared memory.\n", shared_data->total_orders);

    int express_count = 0;
    printf("📦 Order bertipe Express:\n");
    for (int i = 0; i < shared_data->total_orders; i++) {
        if (strcmp(shared_data->orders[i].tipe, "Express") == 0) {
            printf("- %s (%s)\n", shared_data->orders[i].nama_penerima, shared_data->orders[i].alamat);
            express_count++;
        }
    }
    if (express_count == 0) {
        printf("⚠ Tidak ditemukan order Express.\n");
    }

    pthread_t agents[3];
    pthread_create(&agents[0], NULL, agent_thread, "AGENT A");
    pthread_create(&agents[1], NULL, agent_thread, "AGENT B");
    pthread_create(&agents[2], NULL, agent_thread, "AGENT C");

    for (int i = 0; i < 3; i++) {
        pthread_join(agents[i], NULL);
    }

    printf("✅ Semua Express order telah dikirim dan dicatat di delivery.log.\n");

    return 0;
}
