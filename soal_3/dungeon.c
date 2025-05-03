#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include "shop.h"

#define PORT 8080
#define MAX_INVENTORY 10

typedef struct {
    int gold;
    int kills;
    int inventory[MAX_INVENTORY];
    int inventory_count;
    int equipped;
} Player;

Player player = {500, 0, {0}, 1, 0}; 

int enemy_hp = 0, enemy_max_hp = 0;
bool in_battle = false;

void print_health_bar(int hp, int max_hp, char *bar_out) {
    int bar_len = 20;
    int filled = (hp * bar_len) / max_hp;
    strcpy(bar_out, "[");
    for (int i = 0; i < bar_len; i++) {
        strcat(bar_out, (i < filled) ? "\033[0;32m█\033[0m" : " ");
    }
    strcat(bar_out, "]");
}

int generate_enemy_hp() {
    return (rand() % 41 + 40);
}

void start_new_enemy(char *response) {
    enemy_max_hp = generate_enemy_hp();
    enemy_hp = enemy_max_hp;
    in_battle = true;

    char bar[128];
    print_health_bar(enemy_hp, enemy_max_hp, bar);
    snprintf(response, 4096,
        "\n=== \033[1;31mBATTLE STARTED\033[0m ===\n"
        "Enemy appeared with:\n%s %d/%d HP\n"
        "Type '\033[1;32mattack\033[0m' to attack or '\033[1;33mexit\033[0m' to leave battle.\n",
        bar, enemy_hp, enemy_max_hp);
}

void handle_attack(char *response) {
    if (player.inventory_count == 0 || player.equipped >= player.inventory_count) {
        strcpy(response, "No weapon equipped!\n");
        return;
    }

    int weapon_index = player.inventory[player.equipped];
    Weapon w = shop_weapons[weapon_index];
    int damage = w.damage + (rand() % 10); 

    if (strstr(w.passive, "Crit") && rand() % 100 < 30) damage *= 2;
    else if (strstr(w.passive, "Insta") && rand() % 100 < 10) damage = enemy_hp;

    enemy_hp -= damage;

    if (enemy_hp <= 0) {
        int reward = rand() % 151 + 50; 
        player.gold += reward;
        player.kills++;

        snprintf(response, 4096,
            "You dealt \033[1;31m%d damage\033[0m and \033[1;32mdefeated\033[0m the enemy!\n\n"
            "=== \033[1;35mREWARD\033[0m ===\nYou earned \033[1;33m%d gold\033[0m!\n\n",
            damage, reward);

        char next_enemy[512];
        start_new_enemy(next_enemy);
        strcat(response, next_enemy);
    } else {
        char bar[128];
        print_health_bar(enemy_hp, enemy_max_hp, bar);
        snprintf(response, 4096,
            "You dealt \033[1;31m%d damage!\033[0m\n\n"
            "=== \033[1;36mENEMY STATUS\033[0m ===\nEnemy health: %s %d/%d HP\n",
            damage, bar, enemy_hp, enemy_max_hp);
    }
}

void get_player_stats(char *response) {
    int idx = player.inventory[player.equipped];
    Weapon w = shop_weapons[idx];
    snprintf(response, 1024,
        "\n=== \033[1;36mPLAYER STATS\033[0m ===\n"
        "\033[1;33mGold\033[0m: %d | \033[1;32mEquipped Weapon\033[0m: %s | \033[1;31mBase Damage\033[0m: %d | \033[1;34mKills\033[0m: %d",
        player.gold, w.name, w.damage, player.kills);
    if (strlen(w.passive) > 0) {
        strcat(response, " | \033[1;35mPassive\033[0m: ");
        strcat(response, w.passive);
    }
    strcat(response, "\n");
}

void show_inventory(char *response) {
    strcpy(response, "\n=== \033[1;36mYOUR INVENTORY\033[0m ===\n");
    for (int i = 0; i < player.inventory_count; i++) {
        Weapon w = shop_weapons[player.inventory[i]];
        char temp[256];
        snprintf(temp, sizeof(temp), "[%d] %s", i, w.name);
        strcat(response, temp);
        if (strlen(w.passive) > 0) {
            snprintf(temp, sizeof(temp), " (\033[1;35mPassive: %s\033[0m)", w.passive);
            strcat(response, temp);
        }
        if (i == player.equipped) strcat(response, " \033[1;33m(EQUIPPED)\033[0m");
        strcat(response, "\n");
    }
    strcat(response, "\033[1;33mSelect weapon number to equip (0 to cancel): \033[0m");
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[1024], response[4096];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_socket, buffer, sizeof(buffer));
        if (valread <= 0) break;

        buffer[valread] = '\0';
        memset(response, 0, sizeof(response));
        printf("[Client Command] %s\n", buffer);

        if (strcmp(buffer, "SHOW_STATS") == 0) get_player_stats(response);
        else if (strcmp(buffer, "SHOP") == 0) {
            strcpy(response, "\n=== \033[1;35mWEAPON SHOP\033[0m ===\n");
            for (int i = 0; i < MAX_WEAPONS; i++) {
                char temp[256];
                snprintf(temp, sizeof(temp), "[%d] %s - Price: %d gold, Damage: %d", i + 1, shop_weapons[i].name, shop_weapons[i].price, shop_weapons[i].damage);
                strcat(response, temp);
                if (strlen(shop_weapons[i].passive) > 0) {
                    snprintf(temp, sizeof(temp), " (\033[1;34m%s\033[0m)", shop_weapons[i].passive);
                    strcat(response, temp);
                }
                strcat(response, "\n");
            }
            strcat(response, "\033[1;33m\nEnter weapon number to buy (0 to cancel): \033[0m");
        }
        else if (strncmp(buffer, "BUY_", 4) == 0) {
            int id = atoi(buffer + 4) - 1;
            if (id >= 0 && id < MAX_WEAPONS && player.gold >= shop_weapons[id].price) {
                player.gold -= shop_weapons[id].price;
                player.inventory[player.inventory_count++] = id;
                snprintf(response, sizeof(response), "You bought %s!\n", shop_weapons[id].name);
            } else strcpy(response, "Cannot buy weapon.\n");
        }
        else if (strcmp(buffer, "INVENTORY") == 0) show_inventory(response);
        else if (strncmp(buffer, "EQUIP_", 6) == 0) {
            int eq = atoi(buffer + 6);
            if (eq >= 0 && eq < player.inventory_count) {
                player.equipped = eq;
                snprintf(response, sizeof(response), "Now equipped: %s\n", shop_weapons[player.inventory[eq]].name);
            } else strcpy(response, "Invalid selection.\n");
        }
        else if (strcmp(buffer, "BATTLE") == 0) start_new_enemy(response);
        else if (strcmp(buffer, "attack") == 0 && in_battle) handle_attack(response);
        else if (strcmp(buffer, "exit") == 0 && in_battle) {
            in_battle = false;
            strcpy(response, "You fled the battle.\n");
        }
        else if (strcmp(buffer, "EXIT") == 0) {
            strcpy(response, "Goodbye, adventurer!");
            send(client_socket, response, strlen(response), 0);
            break;
        } else strcpy(response, "Unknown command.\n");

        send(client_socket, response, strlen(response), 0);
    }

    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    srand(time(NULL));
    init_weapons();
    player.inventory[0] = 0;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0), new_socket;
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(PORT)};
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);
    printf("Dungeon server is running on port %d...\n", PORT);

    int addrlen = sizeof(address);
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }
    return 0;
}
