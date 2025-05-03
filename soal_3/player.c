// player.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080

void show_menu() {
    printf("\n===== \033[1;32mMAIN MENU\033[0m =====\n");
    printf("1. Show Player Stats\n");
    printf("2. Shop (Buy Weapons)\n");
    printf("3. View Inventory & Equip Weapons\n");
    printf("4. Battle Mode\n");
    printf("5. Exit Game\n");
    printf("\033[1;33mChoose an option:\033[0m ");
}

void send_command(int sock, const char *command) {
    char buffer[4096] = {0};
    send(sock, command, strlen(command), 0);
    int valread = read(sock, buffer, sizeof(buffer));
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("%s", buffer);
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {.sin_family = AF_INET, .sin_port = htons(PORT)};
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    printf("\033[1;34mConnected to the dungeon server!\033[0m\n");

    while (1) {
        show_menu();
        int choice;
        scanf("%d", &choice); getchar();
        char command[32];

        switch (choice) {
            case 1: send_command(sock, "SHOW_STATS"); break;
            case 2:
                send_command(sock, "SHOP");
                int w;
                scanf("%d", &w); getchar();
                if (w > 0) {
                    sprintf(command, "BUY_%d", w);
                    send_command(sock, command);
                }
                break;
            case 3:
                send_command(sock, "INVENTORY");
                int sel;
                scanf("%d", &sel); getchar();
                if (sel > 0) {
                    sprintf(command, "EQUIP_%d", sel);
                    send_command(sock, command);
                }
                break;
            case 4:
                send_command(sock, "BATTLE");
                while (1) {
                    printf("\n> ");
                    char input[32];
                    fgets(input, sizeof(input), stdin);
                    input[strcspn(input, "\n")] = 0;
                    send(sock, input, strlen(input), 0);
                    char buffer[4096] = {0};
                    int valread = read(sock, buffer, sizeof(buffer));
                    if (valread > 0) {
                        buffer[valread] = '\0';
                        printf("%s", buffer);
                        if (strstr(buffer, "fled") || strstr(buffer, "Goodbye")) break;
                    }
                }
                break;
            case 5:
                send_command(sock, "EXIT");
                close(sock);
                return 0;
        }
    }
}
