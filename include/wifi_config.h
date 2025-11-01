#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <stdbool.h>

// OPCIÓN 1: Tu red actual
#define WIFI_SSID      "Sukuna-78-2.4g"
#define WIFI_PASSWORD  "gMigbert.78"

// OPCIÓN 2: Red de prueba (si tienes otra)
// #define WIFI_SSID      "RedAlternativa"
// #define WIFI_PASSWORD  "password123"

#define WIFI_MAX_RETRY 15  // Más intentos

// Funciones WiFi
bool wifi_init_sta(void);
bool wifi_is_connected(void);
char* wifi_get_ip(void);
void wifi_stop(void);

#endif // WIFI_CONFIG_H