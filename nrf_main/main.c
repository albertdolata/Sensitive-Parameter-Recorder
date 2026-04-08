/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>


int interval_max = 5010;
int interval_min = 5000;


struct sensors_data_t {
	uint16_t company_id;
	uint16_t temp;
	uint16_t humid;
	bool red_switch_stat;
}__packed;

static struct sensors_data_t test_data ={
.company_id = 0x0059,
.temp = 10,
.humid = 2,
.red_switch_stat = true, 
};

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, &test_data, sizeof(test_data)),
};

uint16_t get_temp(void){
	// TODO: implementacja odczytu
	static uint16_t fake_temp = 220;
	fake_temp++;
	if (fake_temp > 350) fake_temp = 200;
	return fake_temp;
}

uint16_t get_humidity(void){
	// TODO: implementacja odczytu
	return 45;
}

bool get_reed_switch_state(void){
	// TODO: gpio_pin_get_dt(reed_spec)
	return true;
}

int main(void)
{

	struct bt_le_adv_param adv_param = {
    .id = BT_ID_DEFAULT,
    .sid = 0,
    .secondary_max_skip = 0,
    .options = BT_LE_ADV_OPT_USE_IDENTITY,
    .interval_min = BT_GAP_MS_TO_ADV_INTERVAL(5000), // 5 sekund
    .interval_max = BT_GAP_MS_TO_ADV_INTERVAL(5010),
    .peer = NULL,
};
    int err;

    printk("Inicjalizacja Bluetooth...\n");

    
    err = bt_enable(NULL);
    if (err) {
        printk("Błąd inicjalizacji: %d\n", err);
        return 0;
    }

    
    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Błąd startu: %d\n", err);
        return 0;
    }


    printk("Advertising działa. Sprawdź telefon!\n");


    while (1) {

        test_data.temp = get_temp();
        test_data.humid = get_humidity();
        test_data.red_switch_stat = get_reed_switch_state();

        int err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
        
        if (err) {
            printk("Błąd aktualizacji (err %d)\n", err);
        } else {
            printk("TX -> T: %u, H: %u, S: %d\n", 
                    test_data.temp, test_data.humid, test_data.red_switch_stat);
        }
        k_sleep(K_MSEC(4000));
	}
    return 0;
}