/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/direction.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

/* Length of CTE in unit of 8[us] */
#define CTE_LEN (0x14U)
/* Number of CTE send in single periodic advertising train */
#define PER_ADV_EVENT_CTE_COUNT 5

static void adv_sent_cb(struct bt_le_ext_adv *adv,
            struct bt_le_ext_adv_sent_info *info);

static struct bt_le_ext_adv_cb adv_callbacks = {
    .sent = adv_sent_cb,
};

static struct bt_le_ext_adv *adv_set;

static struct bt_le_adv_param param =
        BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV |
                     BT_LE_ADV_OPT_USE_IDENTITY,
                     BT_GAP_ADV_FAST_INT_MIN_2,
                     BT_GAP_ADV_FAST_INT_MAX_2,
                     NULL);

static struct bt_le_ext_adv_start_param ext_adv_start_param = {
    .timeout = 0,
    .num_events = 0,
};

static struct bt_le_per_adv_param per_adv_param = {
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    .options = BT_LE_ADV_OPT_USE_TX_POWER,
};

#if defined(CONFIG_BT_DF_CTE_TX_AOD)
/* Example sequence of antenna switch patterns for antenna matrix designed by
 * Nordic. For more information about antenna switch patterns see README.rst.
 */
static uint8_t ant_patterns[] = {0x2, 0x0, 0x5, 0x6, 0x1, 0x4, 0xC, 0x9, 0xE,
                 0xD, 0x8, 0xA};
#endif /* CONFIG_BT_DF_CTE_TX_AOD */

struct bt_df_adv_cte_tx_param cte_params = { .cte_len = CTE_LEN,
                         .cte_count = PER_ADV_EVENT_CTE_COUNT,
#if defined(CONFIG_BT_DF_CTE_TX_AOD)
                         .cte_type = BT_DF_CTE_TYPE_AOD_2US,
                         .num_ant_ids = ARRAY_SIZE(ant_patterns),
                         .ant_ids = ant_patterns
#else
                         .cte_type = BT_DF_CTE_TYPE_AOA,
                         .num_ant_ids = 0,
                         .ant_ids = NULL
#endif /* CONFIG_BT_DF_CTE_TX_AOD */
};

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
    BT_DATA_BYTES(BT_DATA_SVC_DATA16,
                  0xaa, 0xfe, /* Eddystone UUID */
                  0x00, /* Eddystone-UID frame type */
                  0x00, /* TX Power */
                  'N', 'I', 'N', 'A', '-', 'B', '4', 'T', 'A', 'G', /* Namespace placeholder, will be replaced in init */
                  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, /* Instance Id placeholder, will be replaced in init */
                  0x00, /* reserved */
                  0x00 /* reserved */
                 )
};

static void adv_sent_cb(struct bt_le_ext_adv *adv,
            struct bt_le_ext_adv_sent_info *info)
{
    printk("Advertiser[%d] %p sent %d\n", bt_le_ext_adv_get_index(adv),
           adv, info->num_sent);
}

int main(void)
{
    char addr_s[BT_ADDR_LE_STR_LEN];
    struct bt_le_oob oob_local;
    int err;

    printk("Starting Connectionless Beacon Demo\n");

    /* Initialize the Bluetooth Subsystem */
    printk("Bluetooth initialization...");
    err = bt_enable(NULL);
    if (err) {
        printk("failed (err %d)\n", err);
        return 0;
    }
    printk("success\n");
    
    printk("Advertising set create...");
    err = bt_le_ext_adv_create(&param, &adv_callbacks, &adv_set);
    if (err) {
        printk("failed (err %d)\n", err);
        return 0;
    }
    printk("success\n");

    printk("Update extended advertising data (Eddystone frame)...");
    err = bt_le_ext_adv_set_data(adv_set, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Failed to set extended advertising data (err %d)\n", err);
        return 0;
    }
    printk("Success\n");
    
    printk("Update CTE params...");
    err = bt_df_set_adv_cte_tx_param(adv_set, &cte_params);
    if (err) {
        printk("failed (err %d)\n", err);
        return 0;
    }
    printk("success\n");

    printk("Periodic advertising params set...");
    err = bt_le_per_adv_set_param(adv_set, &per_adv_param);
    if (err) {
        printk("failed (err %d)\n", err);
        return 0;
    }
    printk("success\n");

    printk("Enable CTE...");
    err = bt_df_adv_cte_tx_enable(adv_set);
    if (err) {
        printk("failed (err %d)\n", err);
        return 0;
    }
    printk("success\n");

    printk("Periodic advertising enable...");
    err = bt_le_per_adv_start(adv_set);
    if (err) {
        printk("failed (err %d)\n", err);
        return 0;
    }
    printk("success\n");

    printk("Extended advertising enable...");
    err = bt_le_ext_adv_start(adv_set, &ext_adv_start_param);
    if (err) {
        printk("failed (err %d)\n", err);
        return 0;
    }
    printk("success\n");

    bt_le_ext_adv_oob_get_local(adv_set, &oob_local);
    bt_addr_le_to_str(&oob_local.addr, addr_s, sizeof(addr_s));

    printk("Started extended advertising as %s\n", addr_s);

    return 0;
}
