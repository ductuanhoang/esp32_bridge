#include <driver/uart.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <esp_log.h>
#include <string.h>
#include <protocol/nmea.h>
#include <stream_stats.h>

#include "uart.h"
#include "config.h"
#include "interface/socket_server.h"
#include "tasks.h"

#include <freertos/event_groups.h>
#include <esp_netif_ip_addr.h>
#include <lwip/lwip_napt.h>
#include "device_data.h"

static const char *TAG = "UART";

ESP_EVENT_DEFINE_BASE(UART_EVENT_READ);
ESP_EVENT_DEFINE_BASE(UART_EVENT_WRITE);

static void uart_number0_init(void);
static void uart_number1_init(void);
static void uart_number1_init_with_baud(uint32_t baud);

void uart_register_read_handler(esp_event_handler_t event_handler)
{
    ESP_ERROR_CHECK(esp_event_handler_register(UART_EVENT_READ, ESP_EVENT_ANY_ID, event_handler, NULL));
}

void uart_unregister_read_handler(esp_event_handler_t event_handler)
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(UART_EVENT_READ, ESP_EVENT_ANY_ID, event_handler));
}

void uart_register_write_handler(esp_event_handler_t event_handler)
{
    ESP_ERROR_CHECK(esp_event_handler_register(UART_EVENT_WRITE, ESP_EVENT_ANY_ID, event_handler, NULL));
}

void uart_unregister_write_handler(esp_event_handler_t event_handler)
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(UART_EVENT_WRITE, ESP_EVENT_ANY_ID, event_handler));
}

static int uart_port_1 = -1;
static uint32_t uart_port_1_baudrate = 0;
static stream_stats_handle_t stream_stats;

static void uart_task(void *ctx);

void uart_init()
{
    uart_number0_init();
    uart_number1_init();
    stream_stats = stream_stats_new("uart");

    xTaskCreate(uart_task, "uart_task", 8192, NULL, TASK_PRIORITY_UART, NULL);
}

/**
 * @brief init uart interface 0
 *
 */
static void uart_number0_init(void)
{
    const uart_port_t uart_num = UART_NUM_0;
    uart_config_t uart0_config = {
        .baud_rate = 57600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart0_config));
    ESP_ERROR_CHECK(uart_set_pin(
        uart_num,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(uart_num, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, NULL, 0));
}

/**
 * @brief init uart interface 1
 *
 */
static void uart_number1_init(void)
{
    uart_port_1 = UART_NUM_1;
    int uart_tx_pin = -1;
    int uart_rx_pin = -1;

    uart_hw_flowcontrol_t flow_ctrl;
    bool flow_ctrl_rts = config_get_bool1(CONF_ITEM(KEY_CONFIG_UART_FLOW_CTRL_RTS));
    bool flow_ctrl_cts = config_get_bool1(CONF_ITEM(KEY_CONFIG_UART_FLOW_CTRL_CTS));

    if (flow_ctrl_rts && flow_ctrl_cts)
    {
        flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;
    }
    else if (flow_ctrl_rts)
    {
        flow_ctrl = UART_HW_FLOWCTRL_RTS;
    }
    else if (flow_ctrl_cts)
    {
        flow_ctrl = UART_HW_FLOWCTRL_CTS;
    }
    else
    {
        flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    }

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = flow_ctrl,
    };
    uart_config.parity = UART_PARITY_DISABLE;
    uart_port_1_baudrate = uart_config.baud_rate;

    uart_tx_pin = ESP_UART_TX_PIN;
    uart_rx_pin = ESP_UART_RX_PIN;


    ESP_LOGE(TAG, "*********** uart_port = %d, uart_tx = %d, uart_rx = %d, baudrate = %d", uart_port_1, uart_tx_pin, uart_rx_pin, uart_config.baud_rate);
    ESP_ERROR_CHECK(uart_param_config(uart_port_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(
        uart_port_1,
        uart_tx_pin,
        uart_rx_pin,
        config_get_i8(CONF_ITEM(KEY_CONFIG_UART_RTS_PIN)),
        config_get_i8(CONF_ITEM(KEY_CONFIG_UART_CTS_PIN))));
    ESP_ERROR_CHECK(uart_driver_install(uart_port_1, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, NULL, 0));
}

/**
 * @brief init uart interface 1
 *
 */
static void uart_number1_init_with_baud(uint32_t baud)
{
    uart_port_1 = UART_NUM_1;
    int uart_tx_pin = -1;
    int uart_rx_pin = -1;

    uart_hw_flowcontrol_t flow_ctrl;
    bool flow_ctrl_rts = config_get_bool1(CONF_ITEM(KEY_CONFIG_UART_FLOW_CTRL_RTS));
    bool flow_ctrl_cts = config_get_bool1(CONF_ITEM(KEY_CONFIG_UART_FLOW_CTRL_CTS));

    if (flow_ctrl_rts && flow_ctrl_cts)
    {
        flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;
    }
    else if (flow_ctrl_rts)
    {
        flow_ctrl = UART_HW_FLOWCTRL_RTS;
    }
    else if (flow_ctrl_cts)
    {
        flow_ctrl = UART_HW_FLOWCTRL_CTS;
    }
    else
    {
        flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    }

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = flow_ctrl,
    };

    uart_tx_pin = ESP_UART_TX_PIN;
    uart_rx_pin = ESP_UART_RX_PIN;

    uart_config.baud_rate = baud;

    ESP_LOGE(TAG, "*********** uart_port = %d, uart_tx = %d, uart_rx = %d, baudrate = %d", uart_port_1, uart_tx_pin, uart_rx_pin, uart_config.baud_rate);
    ESP_ERROR_CHECK(uart_param_config(uart_port_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(
        uart_port_1,
        uart_tx_pin,
        uart_rx_pin,
        config_get_i8(CONF_ITEM(KEY_CONFIG_UART_RTS_PIN)),
        config_get_i8(CONF_ITEM(KEY_CONFIG_UART_CTS_PIN))));
    ESP_ERROR_CHECK(uart_driver_install(uart_port_1, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, NULL, 0));
}

static void uart_uninstall(uint8_t _uart_port)
{
    if (uart_is_driver_installed(_uart_port))
        uart_driver_delete(_uart_port);
}

static void uart_task(void *ctx)
{
    uint8_t buffer[UART_BUFFER_SIZE];

    while (true)
    {
        int32_t len = uart_read_bytes(uart_port_1, buffer, sizeof(buffer), 20 / portTICK_PERIOD_MS);

        if (len < 0)
        {
            ESP_LOGE(TAG, "Error reading from UART");
        }
        else if (len > 0)
        {
            buffer[len] = '\0';
            // ESP_LOGI(TAG, "len = %ld", len);
            //  //ESP_LOGI(TAG, "cmd len = %d", buffer[3]);
            //  //ESP_LOGI(TAG, "cmd = %02x", buffer[2]);
            stream_stats_increment(stream_stats, len, 0);
            esp_event_post(UART_EVENT_READ, len, &buffer, len, portMAX_DELAY);
        }
    }
}

void uart_inject(void *buf, size_t len)
{
    esp_event_post(UART_EVENT_READ, len, buf, len, portMAX_DELAY);
}

int uart_log(char *buf, size_t len)
{
    if (!config_get_bool1(CONF_ITEM(KEY_CONFIG_UART_LOG_FORWARD)))
        return 0;
    return uart_write(buf, len);
}

int uart_nmea(const char *fmt, ...)
{
    uint8_t l = 0;
    // va_list args;
    // va_start(args, fmt);

    // char *nmea;
    // nmea_vasprintf(&nmea, fmt, args);
    // int l = uart_write(nmea, strlen(nmea));
    // free(nmea);

    // va_end(args);

    return l;
}

int uart_write(char *buf, size_t len)
{
    if (uart_port_1 < 0)
        return 0;
    if (len == 0)
        return 0;

    int written = uart_write_bytes(uart_port_1, buf, len);
    if (written < 0)
        return written;

    stream_stats_increment(stream_stats, 0, len);

    esp_event_post(UART_EVENT_WRITE, len, buf, len, portMAX_DELAY);

    return written;
}

// uart detection baud rate

const int UART_GOT_BAUDRATE = BIT0;
/**
 * @brief uart send char
 *
 * @param data
 */
void user_uart_send(const char *data)
{
    uart_write_bytes(uart_port_1, data, strlen(data));
}

/**
 * @brief uart send bytes with length
 *
 * @param buf
 * @param len
 */
static void user_uart_send_u8(uint8_t *buf, int len)
{
    uart_write_bytes(uart_port_1, buf, len);
}

/*************************************************************************************************/
/****************************************Xbee detect driver***************************************/
/*************************************************************************************************/