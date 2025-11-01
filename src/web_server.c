#include "web_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "hardware.h"
#include "wifi_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "WEB_SERVER";
static httpd_handle_t server = NULL;

// Página HTML principal simplificada (sin cJSON por ahora)
static const char* HTML_PAGE = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"    <title>ESP32-C3 Control</title>"
"    <meta name='viewport' content='width=device-width, initial-scale=1'>"
"    <style>"
"        body { font-family: Arial; margin: 20px; background: #f0f0f0; }"
"        .container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }"
"        .status { padding: 10px; margin: 10px 0; border-radius: 5px; text-align: center; }"
"        .led-on { background: #4CAF50; color: white; }"
"        .led-off { background: #f44336; color: white; }"
"        .btn { background: #008CBA; color: white; padding: 10px; border: none; border-radius: 5px; margin: 5px; width: 100%; }"
"        .info { background: #e7f3ff; padding: 10px; border-radius: 5px; margin: 10px 0; }"
"    </style>"
"</head>"
"<body>"
"    <div class='container'>"
"        <h1>ESP32-C3 Control</h1>"
"        <div class='info'>"
"            <strong>IP:</strong> %s<br>"
"        </div>"
"        "
"        <h2>Estado del Sistema</h2>"
"        <div class='status %s'>"
"            LED: %s"
"        </div>"
"        <div class='info'>"
"            <strong>Pulsaciones:</strong> %lu"
"        </div>"
"        "
"        <h2>Control LED</h2>"
"        <button class='btn' onclick='controlLED(1)'>ENCENDER</button>"
"        <button class='btn' onclick='controlLED(0)'>APAGAR</button>"
"        <button class='btn' onclick='updateStatus()'>ACTUALIZAR</button>"
"        "
"        <script>"
"        function controlLED(action) {"
"            var xhr = new XMLHttpRequest();"
"            xhr.open('POST', '/led', true);"
"            xhr.setRequestHeader('Content-Type', 'application/json');"
"            xhr.send(JSON.stringify({action: action}));"
"            setTimeout(updateStatus, 500);"
"        }"
"        "
"        function updateStatus() {"
"            var xhr = new XMLHttpRequest();"
"            xhr.open('GET', '/status', true);"
"            xhr.onload = function() {"
"                if (xhr.status === 200) {"
"                    var data = JSON.parse(xhr.responseText);"
"                    document.querySelector('.status').className = 'status ' + (data.led_state ? 'led-on' : 'led-off');"
"                    document.querySelector('.status').innerHTML = 'LED: ' + (data.led_state ? 'ENCENDIDO' : 'APAGADO');"
"                    document.querySelector('.info').innerHTML = '<strong>IP:</strong> ' + data.ip_address + '<br><strong>Pulsaciones:</strong> ' + data.press_count;"
"                }"
"            };"
"            xhr.send();"
"        }"
"        "
"        // Actualizar automáticamente cada 3 segundos"
"        setInterval(updateStatus, 3000);"
"        updateStatus();"
"        </script>"
"    </div>"
"</body>"
"</html>";

// Handler para la página principal
static esp_err_t root_get_handler(httpd_req_t *req) {
    system_status_t status = web_get_system_status();
    
    char html_response[2048];
    snprintf(html_response, sizeof(html_response), HTML_PAGE,
             status.ip_address,
             status.led_state ? "led-on" : "led-off",
             status.led_state ? "ENCENDIDO" : "APAGADO",
             status.press_count);
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

// Handler para el estado del sistema (JSON simple)
static esp_err_t status_get_handler(httpd_req_t *req) {
    system_status_t status = web_get_system_status();
    
    char json_response[256];
    snprintf(json_response, sizeof(json_response),
             "{\"led_state\":%s,\"button_state\":%s,\"press_count\":%lu,\"ip_address\":\"%s\"}",
             status.led_state ? "true" : "false",
             status.button_state ? "true" : "false",
             status.press_count,
             status.ip_address);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

// Handler para controlar el LED
static esp_err_t led_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
    
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    buf[ret] = '\0';
    
    // Parseo simple (sin cJSON)
    int action = -1;
    if (strstr(buf, "\"action\":0")) action = 0;
    else if (strstr(buf, "\"action\":1")) action = 1;
    else if (strstr(buf, "\"action\":2")) action = 2;
    
    bool success = true;
    char message[50] = "OK";
    
    switch (action) {
        case 0: // Apagar
            led_set(LED_OFF);
            strcpy(message, "LED apagado");
            break;
        case 1: // Encender
            led_set(LED_ON);
            strcpy(message, "LED encendido");
            break;
        case 2: // Alternar
            led_toggle();
            strcpy(message, "LED alternado");
            break;
        default:
            success = false;
            strcpy(message, "Acción no válida");
            break;
    }
    
    // Respuesta JSON simple
    char response[128];
    snprintf(response, sizeof(response),
             "{\"success\":%s,\"message\":\"%s\",\"led_state\":%s}",
             success ? "true" : "false",
             message,
             led_get_state() ? "true" : "false");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

// Configuración de rutas HTTP
static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t status = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t led_control = {
    .uri       = "/led",
    .method    = HTTP_POST,
    .handler   = led_post_handler,
    .user_ctx  = NULL
};

void web_server_start(void) {
    if (server != NULL) {
        ESP_LOGE(TAG, "Servidor web ya está ejecutándose");
        return;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = 80;
    
    ESP_LOGI(TAG, "Iniciando servidor web en puerto %d", config.server_port);
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registrar handlers
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &status);
        httpd_register_uri_handler(server, &led_control);
        
        ESP_LOGI(TAG, "Servidor web iniciado correctamente");
    } else {
        ESP_LOGE(TAG, "Error al iniciar servidor web");
    }
}

void web_server_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "Servidor web detenido");
    }
}

system_status_t web_get_system_status(void) {
    system_status_t status;
    status.led_state = led_get_state();
    status.button_state = button_read();
    status.press_count = button_get_press_count();
    status.ip_address = wifi_get_ip();
    
    return status;
}