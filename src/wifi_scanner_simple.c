#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "WIFI_SCAN";

void wifi_scan_networks_simple(void) {
    ESP_LOGI(TAG, "=== ESCANEO SIMPLE DE REDES WIFI ===");
    
    // Inicialización mínima
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // Esperar inicialización
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "WiFi iniciado, escaneando...");

    // Escaneo básico
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Escaneo exitoso");
        
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        ESP_LOGI(TAG, "Redes encontradas: %d", ap_count);

        if (ap_count > 0) {
            wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
            esp_wifi_scan_get_ap_records(&ap_count, ap_list);

            for (int i = 0; i < ap_count; i++) {
                ESP_LOGI(TAG, "%2d: %-32s (RSSI: %3d)", 
                        i+1, ap_list[i].ssid, ap_list[i].rssi);
                
                if (strcmp((char*)ap_list[i].ssid, "Sukuna-78-2.4g") == 0) {
                    ESP_LOGI(TAG, "     ⭐ RED ENCONTRADA! ⭐");
                }
            }
            free(ap_list);
        }
    } else {
        ESP_LOGE(TAG, "Error en escaneo: %s", esp_err_to_name(ret));
    }

    // Limpiar
    esp_wifi_stop();
    esp_wifi_deinit();
    ESP_LOGI(TAG, "=== FIN ESCANEO ===");
}