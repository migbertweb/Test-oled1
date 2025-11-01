#include "hardware.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *TAG = "HARDWARE";

// Variables de estado
static led_state_t current_led_state = LED_OFF;
static button_state_t last_button_state = BUTTON_RELEASED;
static uint32_t press_count = 0;
static uint32_t last_debounce_time = 0;
static const uint32_t DEBOUNCE_DELAY = 50; // ms

void hardware_init(void) {
    // Configurar LED como salida
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_config);
    
    // Configurar botón como entrada con pull-up
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&button_config);
    
    // Estado inicial del LED
    led_set(LED_OFF);
    
    ESP_LOGI(TAG, "Hardware inicializado - LED: GPIO%d, Botón: GPIO%d", LED_GPIO, BUTTON_GPIO);
}

void led_set(led_state_t state) {
    current_led_state = state;
    gpio_set_level(LED_GPIO, state);
}

void led_toggle(void) {
    current_led_state = !current_led_state;
    gpio_set_level(LED_GPIO, current_led_state);
}

led_state_t led_get_state(void) {
    return current_led_state;
}

button_state_t button_read(void) {
int level = gpio_get_level(BUTTON_GPIO);
    // Si el GPIO lee 0 (LOW), el botón está PRESIONADO
    // Si el GPIO lee 1 (HIGH), el botón está LIBERADO
    return (level == 0) ? BUTTON_PRESSED : BUTTON_RELEASED;
}

bool button_is_pressed(void) {
    return button_read() == BUTTON_PRESSED;
}

uint32_t button_get_press_count(void) {
    return press_count;
}

void hardware_update(void) {
    // Leer estado actual del botón
    button_state_t current_button_state = button_read();
    
    // Detectar flanco de bajada (botón presionado)
    if (current_button_state == BUTTON_PRESSED && last_button_state == BUTTON_RELEASED) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Debounce
        if ((current_time - last_debounce_time) > DEBOUNCE_DELAY) {
            press_count++;
            led_toggle(); // Cambiar estado del LED
            ESP_LOGI(TAG, "Botón presionado - LED: %s", led_get_state() ? "ON" : "OFF");
            last_debounce_time = current_time;
        }
    }
    
    last_button_state = current_button_state;
}