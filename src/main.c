#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "oled.h"
#include "hardware.h"
#include "wifi_config.h"
#include "web_server.h"
#include "dht11_sensor.h"

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "🚀 Iniciando Sistema ESP32-C3 con DHT11");
    
    // 1. Inicializar hardware
    hardware_init();
    i2c_master_init();
    oled_init();
    dht11_init();  // <- NUEVO: Inicializar sensor DHT11
    
    oled_show_welcome_screen();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // 2. Conectar WiFi
    ESP_LOGI(TAG, "Conectando WiFi...");
    oled_clear();
    oled_draw_text_centered(1, "Conectando WiFi...");
    oled_update();
    
    bool wifi_ok = wifi_init();
    
    if (wifi_ok) {
        char* ip = wifi_get_ip();
        ESP_LOGI(TAG, "WiFi OK - IP: %s", ip);
        
        // 3. Iniciar servidor web
        web_server_start();
        
        oled_clear();
        oled_draw_text_centered(0, "WEB SERVER ACTIVE");
        oled_draw_text_centered(1, ip);
        oled_draw_text_centered(2, "DHT11 Sensor OK");
        oled_update();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        
        ESP_LOGI(TAG, "Sistema listo en http://%s", ip);
    } else {
        ESP_LOGE(TAG, "WiFi fallo - Modo local");
        oled_clear();
        oled_draw_text_centered(1, "MODO LOCAL");
        oled_draw_text_centered(2, "DHT11 Active");
        oled_update();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    // 4. Bucle principal
    ESP_LOGI(TAG, "Iniciando bucle principal...");
    uint32_t sensor_read_counter = 0;
    
    while(1) {
        hardware_update();
        
        // Leer sensor cada 5 segundos (aprox)
        if (sensor_read_counter % 100 == 0) {
            dht11_data_t sensor_data = dht11_read_data();
            if (sensor_data.valid) {
                ESP_LOGI(TAG, "Sensor - Temp: %.1fC, Hum: %.1f%%", 
                         sensor_data.temperature, sensor_data.humidity);
            }
        }
        sensor_read_counter++;
        
        // Mostrar estado en OLED
        oled_show_button_debug(button_read(), led_get_state());
        
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}