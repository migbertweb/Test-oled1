#ifndef DHT11_SENSOR_H
#define DHT11_SENSOR_H

#include <stdbool.h>

// Configuración
#define DHT11_GPIO 10

// Estructura para datos del sensor
typedef struct {
    float temperature;
    float humidity;
    bool valid;
} dht11_data_t;

// Funciones
bool dht11_init(void);
dht11_data_t dht11_read_data(void);
const char* dht11_get_status(void);

#endif // DHT11_SENSOR_H