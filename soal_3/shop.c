#include "shop.h"
#include <string.h>

Weapon shop_weapons[MAX_WEAPONS];

void init_weapons() {
    strcpy(shop_weapons[0].name, "Iron Sword");
    shop_weapons[0].price = 50;
    shop_weapons[0].damage = 10;
    strcpy(shop_weapons[0].passive, "");

    strcpy(shop_weapons[1].name, "Flame Dagger");
    shop_weapons[1].price = 100;
    shop_weapons[1].damage = 25;
    strcpy(shop_weapons[1].passive, "");

    strcpy(shop_weapons[2].name, "Shadow Katana");
    shop_weapons[2].price = 200;
    shop_weapons[2].damage = 35;
    strcpy(shop_weapons[2].passive, "");

    strcpy(shop_weapons[3].name, "Heavenly Staff");
    shop_weapons[3].price = 120;
    shop_weapons[3].damage = 20;
    strcpy(shop_weapons[3].passive, "10% Insta-Kill Chance");

    strcpy(shop_weapons[4].name, "Dragon Claws");
    shop_weapons[4].price = 300;
    shop_weapons[4].damage = 50;
    strcpy(shop_weapons[4].passive, "50% Crit Chance");
}
