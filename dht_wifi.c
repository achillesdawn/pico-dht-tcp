#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "tcp_client.h"
#include "dhtlib.h"

#include "ssid.h"

const uint8_t YELLOW_LED = 15;
const uint8_t DHT_PIN = 18;

volatile bool yellow_led_state = false;

bool toggle_led_repeating_callback(struct repeating_timer* t) {
    yellow_led_state = !yellow_led_state;
    gpio_put(YELLOW_LED, yellow_led_state);
    return true;
}

bool connect_to_wifi() {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_BRAZIL)) {
        printf("Failed to initialize wifi hardware/driver");
        return false;
    }

    printf("Initialized cyw43\n");

    cyw43_arch_enable_sta_mode();
    printf("Pico operating in Wifi-Station(STA) mode\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 15000)) {
        printf("Failed to connect to wifi");
        cyw43_arch_deinit();
        return false;
    }

    printf("connected\n");
    return true;
}

bool connect_and_send(DhtData* dht) {

    bool connected = connect_to_wifi();
    if (!connected) {
        return false;
    }

    char buffer[500];
    
    int written = sprintf(buffer, "H_%f\nT_%f\n", dht->humidity, dht->temperature);
    if (written > 0 ) {
        printf("wrote %d into the buffer", written);
    }

    TCP_CLIENT_T* client = tcp_client_init(buffer, written);
    bool result = tcp_client_connect(client);

    free(client);
    cyw43_arch_deinit();

    if (!result) {
        printf("TCP CONNECT failed\n");
        return false;
    }
    return true;
}

void setup() {
    stdio_init_all();

    printf("setting up");

    gpio_init(YELLOW_LED);
    gpio_set_dir(YELLOW_LED, GPIO_OUT);
    gpio_put(YELLOW_LED, true);


    gpio_init(DHT_PIN);
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, true);
}

int main() {
    
    setup();

    sleep_ms(3000);

    printf("Setting alarms\n");

    struct repeating_timer led_timer;
    add_repeating_timer_ms(500, toggle_led_repeating_callback, NULL, &led_timer);

    while (true) {

        DhtData* dht = dht_init_sequence();
        if (dht != NULL) {
            connect_and_send(dht);
        }
        free(dht);

        sleep_ms(15000);
    }
}
