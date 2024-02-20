/*
 * This file is part of the ESP32-XBee distribution (https://github.com/nebkat/esp32-xbee).
 * Copyright (c) 2019 Nebojsa Cvetkovic.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <string.h>
#include <sys/param.h>
#include <tasks.h>

#include "config.h"
#include "interface/socket_server.h"
#include "status_led.h"
#include "stream_stats.h"
#include "uart.h"
#include "util.h"

static const char *TAG = "SOCKET_SERVER";

#define ERROR_ACTION(tag, cond, action, ...) if(cond) { ESP_LOGE(tag, __VA_ARGS__); action; }
#define BUFFER_SIZE 1024
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static int sock_tcp, sock_udp;
static char *buffer;

static status_led_handle_t status_led = NULL;
static stream_stats_handle_t stream_stats = NULL;

typedef struct socket_client_t {
    int socket;
    struct sockaddr_in6 addr;
    int type;
    SLIST_ENTRY(socket_client_t) next;
} socket_client_t;

static SLIST_HEAD(socket_client_list_t, socket_client_t) socket_client_list;

static bool socket_address_equal(struct sockaddr_in6 *a, struct sockaddr_in6 *b) {
    // Check family
    if (a->sin6_family != b->sin6_family) return false;

    // Handle IPv4 addresses
    if (a->sin6_family == PF_INET) {
        struct sockaddr_in *a4 = (struct sockaddr_in *)a;
        struct sockaddr_in *b4 = (struct sockaddr_in *)b;

        return a4->sin_addr.s_addr == b4->sin_addr.s_addr && a4->sin_port == b4->sin_port;
    }

    // Handle IPv6 addresses
    if (a->sin6_family == PF_INET6) {
        return memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(a->sin6_addr)) == 0 && a->sin6_port == b->sin6_port;
    }

    // If not IPv4 or IPv6, return false
    return false;
}


static socket_client_t * socket_client_add(int sock, struct sockaddr_in6 addr, int socktype) {
    socket_client_t *client = malloc(sizeof(socket_client_t));
    if (!client) {
        ESP_LOGE(TAG, "Failed to allocate memory for new client.");
        return NULL;
    }

    *client = (socket_client_t) {
        .socket = sock,
        .addr = addr,
        .type = socktype
    };

    SLIST_INSERT_HEAD(&socket_client_list, client, next);

    char *addr_str = sockaddrtostr((struct sockaddr *) &addr);
    //ESP_LOGI(TAG, "Accepted %s client %s", SOCKTYPE_NAME(socktype), addr_str);
    uart_nmea("$PESP,SOCK,SRV,%s,CONNECTED,%s", SOCKTYPE_NAME(socktype), addr_str);

    if (status_led != NULL) status_led->flashing_mode = STATUS_LED_FADE;
    
    free(addr_str); // If sockaddrtostr allocates memory, ensure it's freed.
    return client;
}

static void socket_client_remove(socket_client_t *socket_client) {
    char *addr_str = sockaddrtostr((struct sockaddr *) &socket_client->addr);
    //ESP_LOGI(TAG, "Disconnected %s client %s", SOCKTYPE_NAME(socket_client->type), addr_str);
    uart_nmea("$PESP,SOCK,SRV,%s,DISCONNECTED,%s", SOCKTYPE_NAME(socket_client->type), addr_str);

    destroy_socket(&socket_client->socket);

    SLIST_REMOVE(&socket_client_list, socket_client, socket_client_t, next);
    free(socket_client);

    if (status_led != NULL && SLIST_EMPTY(&socket_client_list)) status_led->flashing_mode = STATUS_LED_STATIC;

    free(addr_str); // If sockaddrtostr allocates memory, ensure it's freed.
}

static void socket_server_uart_handler(void* handler_args, esp_event_base_t base, int32_t length, void* buf) {
    socket_client_t *client, *client_tmp;
    SLIST_FOREACH_SAFE(client, &socket_client_list, next, client_tmp) {
        int sent = write(client->socket, buf, length);
        if (sent < 0) {
            ESP_LOGE(TAG, "Could not write to %s socket: %d %s", SOCKTYPE_NAME(client->type), errno, strerror(errno));
            socket_client_remove(client);
        } else {
            stream_stats_increment(stream_stats, 0, sent);
        }
    }
}


static int create_bound_socket(int socktype, int port) {
    struct sockaddr_in6 address = {
        .sin6_family = PF_INET6,
        .sin6_addr = IN6ADDR_ANY_INIT,
        .sin6_port = htons(port)
    };

    int sock = socket(PF_INET6, socktype, 0);
    ERROR_ACTION(TAG, sock < 0, return -1, "Could not create %s socket: %d %s", SOCKTYPE_NAME(socktype), errno, strerror(errno));

    int reuse = 1;
    int err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ERROR_ACTION(TAG, err != 0, close(sock); return -1, "Could not set %s socket options: %d %s", SOCKTYPE_NAME(socktype), errno, strerror(errno));

    err = bind(sock, (struct sockaddr *)&address, sizeof(address));
    ERROR_ACTION(TAG, err != 0, close(sock); return -1, "Could not bind %s socket: %d %s", SOCKTYPE_NAME(socktype), errno, strerror(errno));

    return sock;
}

static esp_err_t socket_tcp_init() {
    int port = config_get_u16(CONF_ITEM(KEY_CONFIG_SOCKET_SERVER_TCP_PORT));
    sock_tcp = create_bound_socket(SOCK_STREAM, port);
    if (sock_tcp < 0) return ESP_FAIL;

    int err = listen(sock_tcp, 1);
    ERROR_ACTION(TAG, err != 0, close(sock_tcp); return ESP_FAIL, "Could not listen on TCP socket: %d %s", errno, strerror(errno));

    return ESP_OK;
}


static esp_err_t socket_tcp_accept() {
    struct sockaddr_in6 source_addr;
    socklen_t addr_len = sizeof(source_addr);
    int sock = accept(sock_tcp, (struct sockaddr *)&source_addr, &addr_len);

    if (sock < 0) {
        ESP_LOGE(TAG, "Could not accept new TCP connection: %d %s", errno, strerror(errno));
        return ESP_FAIL;
    }

    socket_client_add(sock, source_addr, SOCK_STREAM);
    return ESP_OK;
}

static esp_err_t socket_udp_init() {
    int port = config_get_u16(CONF_ITEM(KEY_CONFIG_SOCKET_SERVER_UDP_PORT));
    sock_udp = create_bound_socket(SOCK_DGRAM, port);
    return sock_udp < 0 ? ESP_FAIL : ESP_OK;
}

static bool socket_udp_has_client(struct sockaddr_in6 *source_addr) {
    socket_client_t *client;
    SLIST_FOREACH(client, &socket_client_list, next) {
        if (client->type != SOCK_DGRAM) continue;

        struct sockaddr_in6 *client_addr = ((struct sockaddr_in6 *) &client->addr);

        if (socket_address_equal(source_addr, client_addr)) return true;
    }

    return false;
}

static esp_err_t socket_udp_client_accept(struct sockaddr_in6 source_addr) {
    if (socket_udp_has_client(&source_addr)) return ESP_OK;

    int sock = socket(PF_INET6, SOCK_DGRAM, 0);
    ERROR_ACTION(TAG, sock < 0, return ESP_FAIL, "Could not create client UDP socket: %d %s", errno, strerror(errno));

    int reuse = 1;
    int err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ERROR_ACTION(TAG, err != 0, close(sock); return ESP_FAIL, "Could not set client UDP socket options: %d %s", errno, strerror(errno));

    struct sockaddr_in6 server_addr;
    socklen_t socklen = sizeof(server_addr);
    getsockname(sock_udp, (struct sockaddr *)&server_addr, &socklen);

    err = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ERROR_ACTION(TAG, err != 0, close(sock); return ESP_FAIL, "Could not bind client UDP socket: %d %s", errno, strerror(errno));

    err = connect(sock, (struct sockaddr *)&source_addr, sizeof(source_addr));
    ERROR_ACTION(TAG, err != 0, close(sock); return ESP_FAIL, "Could not connect client UDP socket: %d %s", errno, strerror(errno));

    socket_client_add(sock, source_addr, SOCK_DGRAM);
    return ESP_OK;
}

// New modular function to handle receiving and processing
static int socket_receive_and_process(int sockfd, struct sockaddr *source_addr, socklen_t *addr_len) {
    int len = recvfrom(sockfd, buffer, BUFFER_SIZE, MSG_DONTWAIT, source_addr, addr_len);
    if (len > 0) {
        stream_stats_increment(stream_stats, len, 0);
        uart_write(buffer, len);
    }
    return len;
}

static esp_err_t socket_udp_accept() {
    struct sockaddr_in6 source_addr;
    socklen_t socklen = sizeof(source_addr);

    int len;
    while ((len = socket_receive_and_process(sock_udp, (struct sockaddr *)&source_addr, &socklen)) > 0) {
        socket_udp_client_accept(source_addr);
    }

    if (len < 0 && errno != EWOULDBLOCK) {
        ESP_LOGE(TAG, "Unable to receive UDP connection: %d %s", errno, strerror(errno));
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void socket_clients_receive(fd_set *socket_set) {
    socket_client_t *client, *client_tmp;
    SLIST_FOREACH_SAFE(client, &socket_client_list, next, client_tmp) {
        if (!FD_ISSET(client->socket, socket_set)) continue;

        int len;
        while ((len = socket_receive_and_process(client->socket, NULL, NULL)) > 0);

        if (len < 0 && errno != EWOULDBLOCK) {
            socket_client_remove(client);
        }
    }
}

static void socket_server_task(void *ctx) {
    uart_register_read_handler(socket_server_uart_handler);

    stream_stats = stream_stats_new("socket_server");

    // Allocate buffer once here
    buffer = malloc(BUFFER_SIZE);

    while (true) {
        SLIST_INIT(&socket_client_list);

        socket_tcp_init();
        socket_udp_init();

        fd_set socket_set;

        while (true) {
            FD_ZERO(&socket_set);
            FD_SET(sock_tcp, &socket_set);
            FD_SET(sock_udp, &socket_set);

            int maxfd = MAX(sock_tcp, sock_udp);

            socket_client_t *client;
            SLIST_FOREACH(client, &socket_client_list, next) {
                FD_SET(client->socket, &socket_set);
                maxfd = MAX(maxfd, client->socket);
            }

            int err = select(maxfd + 1, &socket_set, NULL, NULL, NULL);
            if (err < 0) {
                ESP_LOGE(TAG, "Could not select socket to receive from: %d %s", errno, strerror(errno));
                goto _error;
            }

            if (FD_ISSET(sock_tcp, &socket_set)) socket_tcp_accept();
            if (FD_ISSET(sock_udp, &socket_set)) socket_udp_accept();

            socket_clients_receive(&socket_set);
        }

        _error:
        destroy_socket(&sock_tcp);
        destroy_socket(&sock_udp);

        socket_client_t *client, *client_tmp;
        SLIST_FOREACH_SAFE(client, &socket_client_list, next, client_tmp) {
            destroy_socket(&client->socket);
            SLIST_REMOVE(&socket_client_list, client, socket_client_t, next);
            free(client);
        }
    }
    free(buffer);  // Free the buffer once here
}

void socket_server_init() {
    if (!config_get_bool1(CONF_ITEM(KEY_CONFIG_SOCKET_SERVER_ACTIVE))) return;
    xTaskCreate(socket_server_task, "socket_server_task", 4096, NULL, TASK_PRIORITY_INTERFACE, NULL);
}
