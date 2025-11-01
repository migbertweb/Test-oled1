#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>
#include <stdint.h>

// Funciones del servidor web
void web_server_start(void);
void web_server_stop(void);

// Funciones para obtener datos del sistema (para la web)
typedef struct {
    bool led_state;
    bool button_state;
    uint32_t press_count;
    char* ip_address;
} system_status_t;

system_status_t web_get_system_status(void);

#endif // WEB_SERVER_H