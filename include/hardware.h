#ifndef HARDWARE_H
#define HARDWARE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// Configuración de pines
#define LED_GPIO         3
#define BUTTON_GPIO      4

// Estados
typedef enum {
    LED_OFF = 0,
    LED_ON = 1
} led_state_t;

typedef enum {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED = 1
} button_state_t;

// Funciones de inicialización
void hardware_init(void);

// Funciones del LED
void led_set(led_state_t state);
void led_toggle(void);
led_state_t led_get_state(void);

// Funciones del botón
button_state_t button_read(void);
bool button_is_pressed(void);
uint32_t button_get_press_count(void);

// Función de actualización (para debounce)
void hardware_update(void);

#endif // HARDWARE_H