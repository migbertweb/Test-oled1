#ifndef WIFI_DIAGNOSTIC_H
#define WIFI_DIAGNOSTIC_H

#include <stdbool.h>

bool wifi_diagnostic_init(void);
void wifi_diagnostic_scan(void);
bool wifi_diagnostic_connect(const char* ssid, const char* password);
void wifi_diagnostic_cleanup(void);
char* wifi_diagnostic_get_ip(void);

#endif