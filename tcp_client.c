#include "string.h"

#include "pico/cyw43_arch.h"
#include "pico/lwip_nosys.h"
#include "pico/malloc.h"
#include "pico/stdlib.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "tcp_client.h"

const char TCP_SERVER_IP[] = "192.168.2.19";
const u16_t TCP_PORT = 9988;

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {

    printf("sent %d", len);
    TCP_CLIENT_T *client = (TCP_CLIENT_T *)arg;

    return ERR_OK;
}

static err_t tcp_client_recv(
    void *arg, struct tcp_pcb *tpcb, struct pbuf *packet_buffer, err_t err
) {
    /*
    API implementation details are in the section 'Receiving TCP data':
    https://www.nongnu.org/lwip/2_0_x/raw_api.html
    */
    TCP_CLIENT_T *client = (TCP_CLIENT_T *)arg;

    cyw43_arch_lwip_check();
    if (!packet_buffer) {
        // The callback function will be passed a NULL pbuf
        // to indicate that the remote host has closed the connection.
        printf("CONNECTION CLOSED");
    }

    if (packet_buffer->tot_len > 0) {
        printf("recv %d err %d\n", packet_buffer->tot_len, err);

        for (struct pbuf *packet = packet_buffer; packet != NULL;
             packet = packet->next) {

            const char *payload = (char *)packet->payload;
            char message[packet->len];
            strncpy(message, payload, packet->len);
            printf("%s\n", message);

            if (strcmp(message, "HELLO FROM GO") == 0) {
                printf("MESSAGE ACKNOWLEDGED, COMPLETING");
                client->complete = true;
            }
        }

        // API requires tcp_recved be called within the recv callback
        // The len argument indicates the length of the received data.
        tcp_recved(tpcb, packet_buffer->tot_len);
    }

    // If there are no errors and the callback function is to return
    // ERR_OK, then it must free the pbuf. Otherwise, it must not
    // free the pbuf so that lwIP core code can store it.
    pbuf_free(packet_buffer);

    return ERR_OK;
}

err_t tcp_client_on_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_CLIENT_T *client = (TCP_CLIENT_T *)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        return err;
    }

    printf("CONNECTED TO SERVER\n\n");

    client->connected = true;
    return ERR_OK;
}

static err_t tcp_client_close(void *arg) {

    printf("Closing client\n");

    TCP_CLIENT_T *client = (TCP_CLIENT_T *)arg;
    err_t err = ERR_OK;
    if (client->tcp_pcb != NULL) {

        tcp_arg(client->tcp_pcb, NULL);
        tcp_poll(client->tcp_pcb, NULL, 0);
        tcp_sent(client->tcp_pcb, NULL);
        tcp_recv(client->tcp_pcb, NULL);
        tcp_err(client->tcp_pcb, NULL);

        err = tcp_close(client->tcp_pcb);
        if (err != ERR_OK) {
            printf("close failed %d, calling abort\n", err);
            tcp_abort(client->tcp_pcb);
            err = ERR_ABRT;
        }
        // client->tcp_pcb = NULL;
    }
    return err;
}

TCP_CLIENT_T *tcp_client_init(char *data, int len) {

    TCP_CLIENT_T *client = calloc(1, sizeof(TCP_CLIENT_T));

    if (!client) {
        printf("failed to allocate state\n");
        return NULL;
    }

    client->tcp_pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!client->tcp_pcb) {
        printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(client->tcp_pcb, client);
    tcp_sent(client->tcp_pcb, tcp_client_sent);
    tcp_recv(client->tcp_pcb, tcp_client_recv);

    ip4addr_aton(TCP_SERVER_IP, &client->remote_addr);

    strncpy(client->buffer, data, len);

    client->buffer_len = len;

    return client;
}

bool tcp_client_connect(TCP_CLIENT_T *client) {

    printf(
        "Connecting to %s port %d\n",
        ip4addr_ntoa(&client->remote_addr),
        TCP_PORT
    );

    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(
        client->tcp_pcb, &client->remote_addr, TCP_PORT, tcp_client_on_connected
    );
    if (err != ERR_OK) {
        printf("CONNECT ERROR, error number %d\n", err);
        return false;
    }

    err = tcp_write(
        client->tcp_pcb, client->buffer, client->buffer_len, TCP_WRITE_FLAG_COPY
    );

    if (err != ERR_OK) {
        printf("WRITE ERROR CODE %d\n", err);
        return false;
    }

    err = tcp_output(client->tcp_pcb);
    if (err != ERR_OK) {
        printf("TCP OUTPUT ERR CODE %d\n", err);
        return false;
    }
    cyw43_arch_lwip_end();

    while (!client->complete) {
        printf("waiting\n");
        sleep_ms(200);
    }

    cyw43_arch_lwip_begin();
    err = tcp_client_close(client);

    if (err != ERR_OK) {
        printf("error closing tcp client, error number %d\n", err);
    } else {
        printf("ALL DONE\n");
    }
    cyw43_arch_lwip_end();

    async_context_t *context = cyw43_arch_async_context();
    lwip_nosys_deinit(context);

    return true;
}