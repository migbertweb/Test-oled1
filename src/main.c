#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "oled.h"
#include "hardware.h"

static const char *TAG = "MAIN";

// Variables globales
// static uint32_t display_mode = 0; // 0=Estado, 1=Debug, 2=Welcome
// static uint32_t last_mode_change = 0;
// static const uint32_t MODE_CHANGE_DELAY = 5000; // 5 segundos

void app_main(void) {
 ESP_LOGI(TAG, "Iniciando sistema LED + Boton + OLED");
    
    // Inicializar todos los componentes
    hardware_init();
    i2c_master_init();
    oled_init();
    
    // Mostrar pantalla de bienvenida
    oled_show_welcome_screen();
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    
    ESP_LOGI(TAG, "Sistema listo - LED:GPIO%d, Boton:GPIO%d", LED_GPIO, BUTTON_GPIO);
    
    // uint32_t last_display_update = 0;
    // const uint32_t DISPLAY_UPDATE_INTERVAL = 200; // ms
    
    while(1) {
        // Actualizar hardware (leer botón con debounce)
        hardware_update();
        
        /* uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Cambio automático de modo cada 5 segundos
        if ((current_time - last_mode_change) > MODE_CHANGE_DELAY) {
            display_mode = (display_mode + 1) % 3; // Ciclar entre 0,1,2
            last_mode_change = current_time;
            ESP_LOGI(TAG, "Cambiando a modo display: %lu", display_mode);
        }
        
        // Actualizar display periódicamente
        if ((current_time - last_display_update) > DISPLAY_UPDATE_INTERVAL) {
            switch(display_mode) {
                case 0: // Modo estado principal
                    oled_show_status_screen(led_get_state(), button_get_press_count());
                    break;
                    
                case 1: // Modo debug
                    oled_show_button_debug(led_get_state(), button_read());
                    break;
                    
                case 2: // Modo información
                    oled_show_welcome_screen();
                    break;
            }
            last_display_update = current_time;
        } */
        
                // Mostrar siempre la pantalla de debug
        oled_show_button_debug(led_get_state(), button_read());

        // Pequeña pausa para no saturar la CPU
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

}