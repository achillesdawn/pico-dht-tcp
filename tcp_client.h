#define BUF_SIZE 2048

typedef struct TCP_CLIENT_T_ {
    struct tcp_pcb* tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[BUF_SIZE];
    int buffer_len;
    int sent_len;
    bool complete;
    int run_count;
    bool connected;
} TCP_CLIENT_T;


TCP_CLIENT_T * tcp_client_init(void);

bool tcp_client_connect(TCP_CLIENT_T* client);

#pragma once
