#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 8080
#define UKURAN_PENAMPUNG 8192

void tampilkan_menu() {
    printf("\n===== MENU MENU BUKAN MENU MAKAN =====\n");
    printf("1. Kirim file untuk didekripsi\n");
    printf("2. Ambil file jpeg dari server\n");
    printf("3. Keluar\n");
    printf("Pilihanmu: ");
}

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
