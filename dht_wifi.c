#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "tcp_client.h"

#include "ssid.h"

const uint8_t LED = 17;


volatile bool led_state = false;

bool toggle_led_repeating_callback(struct repeating_timer* t) {
    led_state = !led_state;
    gpio_put(LED, led_state);
    return true;
}

bool connect_and_send() {

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_BRAZIL)) {
        printf("failed to initialize");
        return false;
    }

    printf("initialized cyw43\n");

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("failed to connect");
        cyw43_arch_deinit();
        return false;
    }

    printf("connected\n");

    TCP_CLIENT_T* client = tcp_client_init();
    bool result = tcp_client_connect(client);
    if (!result) {
        printf("Connect failed\n");
    }

    free(client);
    cyw43_arch_deinit();
    return true;
}

int main() {
    stdio_init_all();

    printf("setting up");

    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    gpio_put(LED, true);

    struct repeating_timer led_timer;
    // add_repeating_timer_ms(500, toggle_led_repeating_callback, NULL, &led_timer);

    while (true) {
        bool result = connect_and_send();
        if (result) {
            printf("result is true");
        }
        sleep_ms(8000);
    }
}
