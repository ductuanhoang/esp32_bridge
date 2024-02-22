/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "gatt_svr.h"
#include "device_data.h"
#define TAG "BLUETOOTH_GAT"

bool uart_active_handle = false;
uint8_t gatt_svr_led_static_val = 0;

static int gatt_svr_chr_rmc_read_notify(uint16_t conn_handle, uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt, void *arg);

static int gatt_svr_chr_rmc_write_notify(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    // NMEA_SERVICE
    {/* Service: receive the UART message */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(NMEA_SERVICE),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             .uuid = BLE_UUID16_DECLARE(NMEA_RMC_READ),
             .access_cb = gatt_svr_chr_rmc_read_notify,
             .val_handle = &uart_service_handle,
             .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ,
         },
         {
             .uuid = BLE_UUID16_DECLARE(NMEA_RMC_WRITE),
             .access_cb = gatt_svr_chr_rmc_write_notify,
             //  .val_handle = &uart_service_handle,
             .flags = BLE_GATT_CHR_F_WRITE,
         },
         {
             0, /* No more characteristics in this service */
         },
     }},
    {
        0, /* No more services */
    },
};

static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                              void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len)
    {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

/**
 * @brief characteristics receive message from UART
 *
 * @param conn_handle
 * @param attr_handle
 * @param ctxt
 * @param arg
 * @return int
 */
static int gatt_svr_chr_rmc_read_notify(uint16_t conn_handle, uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    ESP_LOGI(TAG, "gatt_svr_chr_rmc_read_notify called");
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    printf("uuid = %x\n", uuid);
    if (uuid == NMEA_RMC_READ)
    {
        rc = os_mbuf_append(ctxt->om, &uart_message_handle,
                            strlen((const char *)uart_message_handle));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

/**
 * @brief characteristics control active or disable message UART
 *
 * @param conn_handle
 * @param attr_handle
 * @param ctxt
 * @param arg
 * @return int
 */
static int gatt_svr_chr_rmc_write_notify(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    ESP_LOGI(TAG, "gatt_svr_chr_rmc_write_notify called");

    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);
    printf("uuid = %x\n", uuid);
    if (uuid == NMEA_RMC_WRITE)
    {
        rc = gatt_svr_chr_write(ctxt->om, 0,
                                sizeof(uart_message_handle),
                                &uart_message_handle, NULL);
        ESP_LOGI(TAG, "message write = %s\n", uart_message_handle);
        return rc;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op)
    {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGI(TAG, "registered service %s with handle=%d\n",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGI(TAG, "registering characteristic %s with "
                      "def_handle=%d val_handle=%d\n",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle,
                 ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGI(TAG, "registering descriptor %s with handle=%d\n",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}

/**
 * @brief report uart message
 *
 * @param message
 * @param lenght
 */
void gatt_report_notify(const char *message, uint16_t len)
{
    int rc;
    struct os_mbuf *om;
    snprintf((char *)uart_message_handle, len + 1, "%s", message);
    if (!notify_state)
    {
        return;
    }
    ESP_LOGI(TAG, "send report message: %s with len: %d", message, len);

    om = ble_hs_mbuf_from_flat(message, len);
    rc = ble_gattc_notify_custom(conn_handle, uart_service_handle, om);

    assert(rc == 0);
}