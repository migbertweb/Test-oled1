#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
#include "wifi_diagnostic.h"

static const char *TAG = "WIFI_DIAG";

static EventGroupHandle_t s_wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT = BIT1;
static const int SCAN_DONE_BIT = BIT2;

static bool s_wifi_initialized = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi STA iniciado");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        const char* reason;
        switch (event->reason) {
            case WIFI_REASON_AUTH_EXPIRE:
                reason = "Autenticación expirada";
                break;
            case WIFI_REASON_AUTH_FAIL:
                reason = "Fallo de autenticación";
                break;
            case WIFI_REASON_NO_AP_FOUND:
                reason = "No se encontró el AP";
                break;
            case WIFI_REASON_ASSOC_FAIL:
                reason = "Fallo de asociación";
                break;
            case WIFI_REASON_HANDSHAKE_TIMEOUT:
                reason = "Timeout en handshake";
                break;
            default:
                reason = "Error desconocido";
        }
        ESP_LOGW(TAG, "WiFi desconectado - Razón: %s (código: %d)", reason, event->reason);
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Escaneo completado");
        xEventGroupSetBits(s_wifi_event_group, SCAN_DONE_BIT);
    }
}

bool wifi_diagnostic_init(void) {
    esp_err_t ret;
    int retry_count = 0;
    const int MAX_RETRIES = 3;

    if (s_wifi_initialized) {
        ESP_LOGW(TAG, "WiFi ya estaba inicializado - Reiniciando...");
        wifi_diagnostic_cleanup();
        vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar 1 segundo
    }

    ESP_LOGI(TAG, "Inicializando sistema WiFi...");
    
    // Crear grupo de eventos
    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
        if (s_wifi_event_group == NULL) {
            ESP_LOGE(TAG, "No se pudo crear grupo de eventos");
            return false;
        }
    }

    // Inicializar TCP/IP stack
    while (retry_count < MAX_RETRIES) {
        ret = esp_netif_init();
        if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
            break;
        }
        ESP_LOGW(TAG, "Intento %d: Error init netif: %s", retry_count + 1, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
    }
    if (retry_count == MAX_RETRIES) {
        ESP_LOGE(TAG, "Error fatal inicializando netif después de %d intentos", MAX_RETRIES);
        return false;
    }

    // Crear event loop
    retry_count = 0;
    while (retry_count < MAX_RETRIES) {
        ret = esp_event_loop_create_default();
        if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
            break;
        }
        ESP_LOGW(TAG, "Intento %d: Error event loop: %s", retry_count + 1, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
    }
    if (retry_count == MAX_RETRIES) {
        ESP_LOGE(TAG, "Error fatal creando event loop después de %d intentos", MAX_RETRIES);
        return false;
    }

    // Crear y verificar netif STA
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (!sta_netif) {
        ESP_LOGE(TAG, "Error creando netif STA");
        esp_netif_deinit();
        esp_event_loop_delete_default();
        return false;
    }

    // Inicializar WiFi
    retry_count = 0;
    while (retry_count < MAX_RETRIES) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret == ESP_OK) {
            break;
        }
        ESP_LOGW(TAG, "Intento %d: Error init wifi: %s", retry_count + 1, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
    }
    if (retry_count == MAX_RETRIES) {
        ESP_LOGE(TAG, "Error fatal inicializando WiFi después de %d intentos", MAX_RETRIES);
        esp_netif_destroy(sta_netif);
        esp_netif_deinit();
        esp_event_loop_delete_default();
        return false;
    }

    // Registrar event handlers
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_event_handler, NULL, NULL);

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error set mode: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error start wifi: %s", esp_err_to_name(ret));
        return false;
    }

    s_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi inicializado exitosamente");
    return true;
}

void wifi_diagnostic_scan(void) {
    if (!s_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi no inicializado para escanear");
        return;
    }

    ESP_LOGI(TAG, "=== INICIANDO ESCANEO WIFI ===");
    
    // Configurar escaneo
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = { .active = { .min = 100, .max = 300 } }
    };

    // Limpiar bits anteriores
    xEventGroupClearBits(s_wifi_event_group, SCAN_DONE_BIT);

    ESP_LOGI(TAG, "Iniciando escaneo...");
    esp_err_t ret = esp_wifi_scan_start(&scan_config, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando escaneo: %s", esp_err_to_name(ret));
        return;
    }

    // Esperar resultados (timeout 15 segundos)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          SCAN_DONE_BIT,
                                          pdTRUE,
                                          pdFALSE,
                                          pdMS_TO_TICKS(15000));

    if (bits & SCAN_DONE_BIT) {
        ESP_LOGI(TAG, "Procesando resultados del escaneo...");
        
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        ESP_LOGI(TAG, "📶 Redes encontradas: %d", ap_count);

        if (ap_count > 0) {
            wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
            esp_wifi_scan_get_ap_records(&ap_count, ap_list);

            ESP_LOGI(TAG, "===============================================");
            for (int i = 0; i < ap_count; i++) {
                const char* auth_str = "Desconocido";
                switch(ap_list[i].authmode) {
                    case WIFI_AUTH_OPEN: auth_str = "Abierto"; break;
                    case WIFI_AUTH_WEP: auth_str = "WEP"; break;
                    case WIFI_AUTH_WPA_PSK: auth_str = "WPA"; break;
                    case WIFI_AUTH_WPA2_PSK: auth_str = "WPA2"; break;
                    case WIFI_AUTH_WPA_WPA2_PSK: auth_str = "WPA/WPA2"; break;
                    case WIFI_AUTH_WPA3_PSK: auth_str = "WPA3"; break;
                    case WIFI_AUTH_WPA2_WPA3_PSK: auth_str = "WPA2/WPA3"; break;
                    case WIFI_AUTH_WAPI_PSK: auth_str = "WAPI"; break;
                    case WIFI_AUTH_OWE: auth_str = "OWE"; break;
                    case WIFI_AUTH_WPA3_ENT_192: auth_str = "WPA3 ENT-192"; break;
                    case WIFI_AUTH_WPA3_EXT_PSK: auth_str = "WPA3-EXT"; break;
                    case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE: auth_str = "WPA3-EXT-MIX"; break;
                    case WIFI_AUTH_DPP: auth_str = "DPP"; break;
                    case WIFI_AUTH_WPA_ENTERPRISE: auth_str = "WPA Enterprise"; break;
                    case WIFI_AUTH_WPA2_ENTERPRISE: auth_str = "WPA2 Enterprise"; break;
                    case WIFI_AUTH_WPA3_ENTERPRISE: auth_str = "WPA3 Enterprise"; break;
                    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE: auth_str = "WPA2/3 Enterprise"; break;
                    case WIFI_AUTH_MAX: auth_str = "MAX"; break;
                    default: auth_str = "Desconocido"; break;
                }
                
                ESP_LOGI(TAG, "%2d. %-32s | RSSI: %3d | Canal: %2d | %s",
                        i+1, ap_list[i].ssid, ap_list[i].rssi, 
                        ap_list[i].primary, auth_str);
                
                // Destacar nuestra red objetivo
                if (strcmp((char*)ap_list[i].ssid, "Sukuna-78-2.4g") == 0) {
                    ESP_LOGI(TAG, "     🎯 RED OBJETIVO DETECTADA - Señal: %s", 
                            ap_list[i].rssi > -50 ? "Excelente" :
                            ap_list[i].rssi > -65 ? "Buena" :
                            ap_list[i].rssi > -75 ? "Regular" : "Débil");
                }
            }
            ESP_LOGI(TAG, "===============================================");
            
            free(ap_list);
        } else {
            ESP_LOGW(TAG, "❌ No se detectaron redes WiFi");
            ESP_LOGW(TAG, "   Verifica que el WiFi esté activado y haya redes disponibles");
        }
    } else {
        ESP_LOGE(TAG, "⏰ Timeout en escaneo WiFi");
    }
    
    ESP_LOGI(TAG, "=== FIN ESCANEO ===");
}

bool wifi_diagnostic_connect(const char* ssid, const char* password) {
    if (!s_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi no inicializado - Intentando reinicializar...");
        if (!wifi_diagnostic_init()) {
            ESP_LOGE(TAG, "Fallo al reinicializar WiFi");
            return false;
        }
    }

    ESP_LOGI(TAG, "=== INICIANDO CONEXION ===");
    ESP_LOGI(TAG, "SSID: %s", ssid);
    
    // Verificar parámetros
    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "SSID inválido");
        return false;
    }
    
    if (!password || strlen(password) < 8) {
        ESP_LOGE(TAG, "Password inválido (mínimo 8 caracteres)");
        return false;
    }
    
    // Configurar credenciales con más opciones de seguridad
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WEP,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    // Configurar WiFi con reintentos
    esp_err_t ret;
    int retry_count = 0;
    const int MAX_RETRIES = 3;

    while (retry_count < MAX_RETRIES) {
        ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        if (ret == ESP_OK) {
            break;
        }
        ESP_LOGW(TAG, "Intento %d: Error config WiFi: %s", retry_count + 1, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
    }
    
    if (retry_count == MAX_RETRIES) {
        ESP_LOGE(TAG, "Error fatal configurando WiFi");
        return false;
    }

    // Limpiar bits anteriores
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    ESP_LOGI(TAG, "Conectando a WiFi...");
    esp_wifi_connect();

    // Esperar conexión (timeout 30 segundos)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdTRUE,
                                          pdFALSE,
                                          pdMS_TO_TICKS(30000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "✅ CONEXION EXITOSA");
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "❌ FALLO LA CONEXION");
        return false;
    } else {
        ESP_LOGE(TAG, "⏰ TIMEOUT EN CONEXION");
        return false;
    }
}

void wifi_diagnostic_cleanup(void) {
    ESP_LOGI(TAG, "Limpiando recursos WiFi...");
    
    // Detener WiFi
    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Error deteniendo WiFi: %s", esp_err_to_name(ret));
    }
    
    // Esperar un momento
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Desinicializar WiFi
    ret = esp_wifi_deinit();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Error desinicializando WiFi: %s", esp_err_to_name(ret));
    }
    
    // Limpiar netif
    esp_netif_destroy_default_wifi(NULL);
    
    // Intentar limpiar event loop
    ret = esp_event_loop_delete_default();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Error eliminando event loop: %s", esp_err_to_name(ret));
    }
    
    // Limpiar grupo de eventos si existe
    if (s_wifi_event_group != NULL) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    
    s_wifi_initialized = false;
    ESP_LOGI(TAG, "Recursos WiFi liberados");
    
    // Esperar un momento antes de permitir reinicialización
    vTaskDelay(pdMS_TO_TICKS(1000));
}

char* wifi_diagnostic_get_ip(void) {
    static char ip_str[16] = "0.0.0.0";
    
    if (s_wifi_initialized) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) == ESP_OK) {
            snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
        }
    }
    
    return ip_str;
}