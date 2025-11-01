#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "oled.h"
#include "hardware.h"
#include "wifi_diagnostic.h"
#include "wifi_config.h"

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "=== DIAGNOSTICO WIFI UNIFICADO ===");
    
    // 1. Inicializar hardware básico
    ESP_LOGI(TAG, "1. Inicializando hardware...");
    hardware_init();
    i2c_master_init();
    oled_init();
    
    oled_clear();
    oled_draw_text_centered(1, "DIAGNOSTICO WIFI");
    oled_draw_text_centered(2, "Iniciando...");
    oled_update();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // 2. Inicializar WiFi UNA sola vez
    ESP_LOGI(TAG, "2. Inicializando sistema WiFi...");
    oled_clear();
    oled_draw_text_centered(1, "INIC. WIFI");
    oled_draw_text_centered(2, "Espere...");
    oled_update();
    
    if (!wifi_diagnostic_init()) {
        ESP_LOGE(TAG, "❌ FALLO INICIALIZACION WIFI");
        oled_clear();
        oled_draw_text_centered(1, "ERROR WIFI");
        oled_draw_text_centered(2, "Reinicia dispositivo");
        oled_update();
        while(1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
    }
    
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // 3. Escanear redes
    ESP_LOGI(TAG, "3. Escaneando redes...");
    oled_clear();
    oled_draw_text_centered(1, "ESCANEANDO");
    oled_draw_text_centered(2, "Redes WiFi...");
    oled_update();
    
    wifi_diagnostic_scan();
    
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // 4. Intentar conexión
    ESP_LOGI(TAG, "4. Conectando a: %s", WIFI_SSID);
    oled_clear();
    oled_draw_text_centered(0, "CONECTANDO");
    oled_draw_text_centered(1, WIFI_SSID);
    oled_draw_text_centered(2, "Espere...");
    oled_update();
    
    bool connected = wifi_diagnostic_connect(WIFI_SSID, WIFI_PASSWORD);
    
    // 5. Mostrar resultado
    if (connected) {
        char* ip = wifi_diagnostic_get_ip();
        ESP_LOGI(TAG, "✅ CONEXION EXITOSA - IP: %s", ip);
        
        oled_clear();
        oled_draw_text_centered(0, "✅ CONECTADO");
        oled_draw_text_centered(1, ip);
        oled_draw_text_centered(2, "Web Server Listo");
        oled_update();
        
        // Aquí iniciarías el servidor web...
        // web_server_start();
        
    } else {
        ESP_LOGE(TAG, "❌ FALLO CONEXION");
        
        oled_clear();
        oled_draw_text_centered(0, "❌ ERROR WIFI");
        oled_draw_text_centered(1, "Verifica:");
        oled_draw_text_centered(2, "- SSID/Password");
        oled_draw_text_centered(3, "- Señal WiFi");
        oled_update();
    }
    
    ESP_LOGI(TAG, "=== DIAGNOSTICO COMPLETADO ===");
    
    // Mantener resultado
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}