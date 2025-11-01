#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "WIFI_SCAN";

static EventGroupHandle_t s_scan_event_group;
static const int SCAN_DONE_BIT = BIT0;

static void scan_event_handler(void* arg, esp_event_base_t event_base, 
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Escaneo completado");
        xEventGroupSetBits(s_scan_event_group, SCAN_DONE_BIT);
    }
}

void wifi_scan_networks(void) {
    ESP_LOGI(TAG, "=== ESCANEO DE REDES WIFI ===");
    
    s_scan_event_group = xEventGroupCreate();

    // Inicializar sistema de eventos y WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Registrar handler para evento de escaneo
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, 
                                                        WIFI_EVENT_SCAN_DONE,
                                                        &scan_event_handler,
                                                        NULL,
                                                        NULL));

    // Configurar modo STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Esperar a que WiFi esté listo
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    // Configurar y iniciar escaneo
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 300
            }
        }
    };

    ESP_LOGI(TAG, "Iniciando escaneo...");
    esp_err_t ret = esp_wifi_scan_start(&scan_config, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando escaneo: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    // Esperar a que el escaneo termine (timeout 10 segundos)
    EventBits_t bits = xEventGroupWaitBits(s_scan_event_group,
                                          SCAN_DONE_BIT,
                                          pdTRUE,
                                          pdFALSE,
                                          pdMS_TO_TICKS(10000));

    if (bits & SCAN_DONE_BIT) {
        ESP_LOGI(TAG, "Escaneo terminado, obteniendo resultados...");
        
        uint16_t ap_count = 0;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        ESP_LOGI(TAG, "Redes encontradas: %d", ap_count);

        if (ap_count > 0) {
            wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));

            ESP_LOGI(TAG, "==========================================");
            ESP_LOGI(TAG, " No.  SSID                             RSSI  Canal  Autenticación");
            ESP_LOGI(TAG, "------------------------------------------");
            
            for (int i = 0; i < ap_count; i++) {
                const char* auth_mode;
                switch(ap_list[i].authmode) {
                    case WIFI_AUTH_OPEN: auth_mode = "Abierto"; break;
                    case WIFI_AUTH_WEP: auth_mode = "WEP"; break;
                    case WIFI_AUTH_WPA_PSK: auth_mode = "WPA-PSK"; break;
                    case WIFI_AUTH_WPA2_PSK: auth_mode = "WPA2-PSK"; break;
                    case WIFI_AUTH_WPA_WPA2_PSK: auth_mode = "WPA/WPA2"; break;
                    case WIFI_AUTH_WPA3_PSK: auth_mode = "WPA3"; break;
                    default: auth_mode = "Otro"; break;
                }
                
                ESP_LOGI(TAG, " %2d.  %-32s %4d    %2d    %s", 
                        i+1, ap_list[i].ssid, ap_list[i].rssi, 
                        ap_list[i].primary, auth_mode);
                
                // Resaltar si es nuestra red
                if (strcmp((char*)ap_list[i].ssid, "Sukuna-78-2.4g") == 0) {
                    ESP_LOGI(TAG, "      ⭐⭐ RED DETECTADA - RSSI: %d ⭐⭐", ap_list[i].rssi);
                }
            }
            ESP_LOGI(TAG, "==========================================");
            
            free(ap_list);
        } else {
            ESP_LOGW(TAG, "No se encontraron redes WiFi");
        }
    } else {
        ESP_LOGE(TAG, "Timeout esperando resultados del escaneo");
    }

cleanup:
    // Limpiar
    esp_event_handler_instance_unregister(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, NULL);
    esp_wifi_stop();
    esp_wifi_deinit();
    vEventGroupDelete(s_scan_event_group);
    esp_netif_destroy_default_wifi(sta_netif);
    esp_event_loop_delete_default();
    
    ESP_LOGI(TAG, "=== FIN ESCANEO ===");
}