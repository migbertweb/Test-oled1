#include "web_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "hardware.h"
#include "wifi_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
<<<<<<< HEAD
#include "dht11_sensor.h"
=======
>>>>>>> dfecfd08907d8c1119f8ae96d2f40eb00149c613

static const char *TAG = "WEB_SERVER";
static httpd_handle_t server = NULL;

// HTML con codificación UTF-8
static const char* HTML_PAGE = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"    <meta charset='UTF-8'>"
"    <title>ESP32-C3 Control</title>"
"    <meta name='viewport' content='width=device-width, initial-scale=1'>"
"    <style>"
"        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }"
"        .container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
"        .status { padding: 10px; margin: 10px 0; border-radius: 5px; text-align: center; font-weight: bold; }"
"        .led-on { background: #4CAF50; color: white; }"
"        .led-off { background: #f44336; color: white; }"
"        .wifi-good { background: #4CAF50; color: white; }"
"        .wifi-weak { background: #FF9800; color: white; }"
"        .wifi-poor { background: #f44336; color: white; }"
<<<<<<< HEAD
"        .sensor-ok { background: #4CAF50; color: white; padding: 5px; border-radius: 3px; }"
"        .sensor-error { background: #f44336; color: white; padding: 5px; border-radius: 3px; }"
=======
>>>>>>> dfecfd08907d8c1119f8ae96d2f40eb00149c613
"        .btn { background: #008CBA; color: white; padding: 12px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; width: 100%; font-size: 16px; }"
"        .btn:hover { background: #005f7a; }"
"        .info { background: #e7f3ff; padding: 10px; border-radius: 5px; margin: 10px 0; }"
"        .section { margin: 20px 0; }"
"    </style>"
"</head>"
"<body>"
"    <div class='container'>"
<<<<<<< HEAD
"        <h1>ESP32-C3 Control Panel</h1>"
"        "
"        <div class='info'>"
"            <strong>IP Address:</strong> %s<br>"
"            <strong>WiFi Signal:</strong> <span class='%s'>%d dBm</span>"
"        </div>"
"        "
"        <div class='section'>"
"            <h2>LED Status</h2>"
=======
"        <h1>ESP32-C3 Control</h1>"
"        "
"        <div class='info'>"
"            <strong>IP:</strong> %s<br>"
"            <strong>Senal WiFi:</strong> <span class='%s'>%d dBm</span>"
"        </div>"
"        "
"        <div class='section'>"
"            <h2>Estado del LED</h2>"
>>>>>>> dfecfd08907d8c1119f8ae96d2f40eb00149c613
"            <div class='status %s' id='ledStatus'>"
"                LED: %s"
"            </div>"
"        </div>"
"        "
"        <div class='section'>"
<<<<<<< HEAD
"            <h2>LED Control</h2>"
"            <button class='btn' onclick='controlLED(1)'>TURN ON LED</button>"
"            <button class='btn' onclick='controlLED(0)'>TURN OFF LED</button>"
"            <button class='btn' onclick='controlLED(2)'>TOGGLE LED</button>"
"        </div>"
"        "
"        <div class='section'>"
"            <h2>System Information</h2>"
"            <div class='info'>"
"                <strong>Button Press Count:</strong> <span id='pressCount'>%lu</span><br>"
"                <strong>Button State:</strong> <span id='buttonState'>%s</span>"
"            </div>"
"        </div>"
"        "
"        <div class='section'>"
"            <h2>DHT11 Sensor</h2>"
"            <div class='info'>"
"                <strong>Temperature:</strong> <span id='temperature'>%.1f</span> °C<br>"
"                <strong>Humidity:</strong> <span id='humidity'>%.1f</span> %%<br>"
"                <strong>Status:</strong> <span id='sensorStatus' class='%s'>%s</span>"
"            </div>"
"        </div>"
"        "
"        <button class='btn' onclick='updateStatus()'>REFRESH ALL</button>"
=======
"            <h2>Control LED</h2>"
"            <button class='btn' onclick='controlLED(1)'>ENCENDER LED</button>"
"            <button class='btn' onclick='controlLED(0)'>APAGAR LED</button>"
"            <button class='btn' onclick='controlLED(2)'>ALTERNAR LED</button>"
        "        </div>"
        "        "
        "        <div class='section'>"
        "            <h2>Informacion del Sistema</h2>"
        "            <div class='info'>"
        "                <strong>Pulsaciones del boton:</strong> <span id='pressCount'>%lu</span><br>"
        "                <strong>Estado del boton:</strong> <span id='buttonState'>%s</span>"
        "            </div>"
        "        </div>"
        "        "
        "        <div class='section'>"
        "            <h2>Sensor DHT11</h2>"
        "            <div class='info'>"
        "                <strong>Temperatura:</strong> <span id='temperature'>%.1f °C</span><br>"
        "                <strong>Humedad:</strong> <span id='humidity'>%.1f %%</span><br>"
        "                <strong>Estado:</strong> <span id='sensorStatus'>%s</span>"
        "            </div>"
        "        </div>"
"        "
"        <button class='btn' onclick='updateStatus()'>ACTUALIZAR TODO</button>"
>>>>>>> dfecfd08907d8c1119f8ae96d2f40eb00149c613
"        "
"        <script>"
"        function controlLED(action) {"
"            fetch('/led', {"
"                method: 'POST',"
"                headers: { 'Content-Type': 'application/json' },"
"                body: JSON.stringify({action: action})"
"            })"
"            .then(response => response.json())"
"            .then(data => {"
"                if(data.success) {"
"                    updateStatus();"
"                }"
"            });"
"        }"
"        "
"        function updateStatus() {"
"            fetch('/status')"
"            .then(response => response.json())"
"            .then(data => {"
<<<<<<< HEAD
"                // Update LED"
"                const ledStatus = document.getElementById('ledStatus');"
"                ledStatus.className = 'status ' + (data.led_state ? 'led-on' : 'led-off');"
"                ledStatus.textContent = 'LED: ' + (data.led_state ? 'ON' : 'OFF');"
"                "
"                // Update information"
"                document.getElementById('pressCount').textContent = data.press_count;"
"                document.getElementById('buttonState').textContent = data.button_state ? 'PRESSED' : 'RELEASED';"
"                "
"                // Update sensor data"
"                document.getElementById('temperature').textContent = data.temperature.toFixed(1);"
"                document.getElementById('humidity').textContent = data.humidity.toFixed(1);"
"                "
"                const sensorStatus = document.getElementById('sensorStatus');"
"                sensorStatus.textContent = data.sensor_valid ? 'OK' : 'ERROR';"
"                sensorStatus.className = data.sensor_valid ? 'sensor-ok' : 'sensor-error';"
"            });"
"        }"
"        "
"        // Auto-update every 3 seconds"
"        setInterval(updateStatus, 3000);"
"        "
"        // Update on page load"
=======
"                // Actualizar LED"
"                const ledStatus = document.getElementById('ledStatus');"
"                ledStatus.className = 'status ' + (data.led_state ? 'led-on' : 'led-off');"
"                ledStatus.textContent = 'LED: ' + (data.led_state ? 'ENCENDIDO' : 'APAGADO');"
                "                "
                "                // Actualizar informacion"
                "                document.getElementById('pressCount').textContent = data.press_count;"
                "                document.getElementById('buttonState').textContent = data.button_state ? 'PRESIONADO' : 'LIBERADO';"
                "                "
                "                // Actualizar datos del sensor DHT11"
                "                if(data.sensor_valid) {"
                "                    document.getElementById('temperature').textContent = data.temperature.toFixed(1) + ' °C';"
                "                    document.getElementById('humidity').textContent = data.humidity.toFixed(1) + ' %';"
                "                    document.getElementById('sensorStatus').textContent = 'VÁLIDO';"
                "                } else {"
                "                    document.getElementById('temperature').textContent = 'N/A';"
                "                    document.getElementById('humidity').textContent = 'N/A';"
                "                    document.getElementById('sensorStatus').textContent = 'NO DISPONIBLE';"
                "                }"
"            });"
"        }"
"        "
"        // Actualizar automaticamente cada 3 segundos"
"        setInterval(updateStatus, 3000);"
"        "
"        // Actualizar al cargar la pagina"
>>>>>>> dfecfd08907d8c1119f8ae96d2f40eb00149c613
"        updateStatus();"
"        </script>"
"    </div>"
"</body>"
"</html>";

// Handler para página principal
static esp_err_t root_get_handler(httpd_req_t *req) {
    system_status_t status = web_get_system_status();
    
<<<<<<< HEAD
=======
    // Determinar clase CSS para senal WiFi
>>>>>>> dfecfd08907d8c1119f8ae96d2f40eb00149c613
    const char* wifi_class = "wifi-poor";
    int rssi = wifi_get_rssi();
    if (rssi > -60) wifi_class = "wifi-good";
    else if (rssi > -75) wifi_class = "wifi-weak";
    
<<<<<<< HEAD
    const char* sensor_status = status.sensor_valid ? "OK" : "ERROR";
    const char* sensor_class = status.sensor_valid ? "sensor-ok" : "sensor-error";
    
    char html_response[4096];
    snprintf(html_response, sizeof(html_response), HTML_PAGE,
             status.ip_address,
             wifi_class, rssi,
             status.led_state ? "led-on" : "led-off",
             status.led_state ? "ON" : "OFF",
             status.press_count,
             status.button_state ? "PRESSED" : "RELEASED",
             status.temperature,
             status.humidity,
             sensor_class,
             sensor_status);
    
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    
=======
    char html_response[4096];
    int len = snprintf(html_response, sizeof(html_response), HTML_PAGE,
             status.ip_address,
             wifi_class, rssi,
             status.led_state ? "led-on" : "led-off",
             status.led_state ? "ENCENDIDO" : "APAGADO",
             status.press_count,
             status.button_state ? "PRESIONADO" : "LIBERADO",
             status.sensor_valid ? status.temperature : 0.0f,
             status.sensor_valid ? status.humidity : 0.0f,
             status.sensor_valid ? "VÁLIDO" : "NO DISPONIBLE");
    
    // Configurar headers para UTF-8
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Type", "text/html; charset=utf-8");
    
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    
    ESP_LOGI(TAG, "Pagina web enviada (%d bytes)", len);
>>>>>>> dfecfd08907d8c1119f8ae96d2f40eb00149c613
    return ESP_OK;
}

// Handler para estado del sistema (JSON)
static esp_err_t status_get_handler(httpd_req_t *req) {
    system_status_t status = web_get_system_status();
    
<<<<<<< HEAD
    char json_response[256];
    snprintf(json_response, sizeof(json_response),
             "{\"led_state\":%s,\"button_state\":%s,\"press_count\":%lu,"
             "\"ip_address\":\"%s\",\"rssi\":%d,"
             "\"temperature\":%.1f,\"humidity\":%.1f,\"sensor_valid\":%s}",
=======
    char json_response[512];
    snprintf(json_response, sizeof(json_response),
             "{\"led_state\":%s,\"button_state\":%s,\"press_count\":%lu,\"ip_address\":\"%s\",\"rssi\":%d,\"temperature\":%.1f,\"humidity\":%.1f,\"sensor_valid\":%s}",
>>>>>>> dfecfd08907d8c1119f8ae96d2f40eb00149c613
             status.led_state ? "true" : "false",
             status.button_state ? "true" : "false",
             status.press_count,
             status.ip_address,
             wifi_get_rssi(),
             status.temperature,
             status.humidity,
             status.sensor_valid ? "true" : "false");
    
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
    
    // Parseo simple
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
    ESP_LOGI(TAG, "🔧 Iniciando servidor web...");
    
    if (server != NULL) {
        ESP_LOGW(TAG, "⚠️  Servidor web ya estaba ejecutándose");
        return;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = 80;
    config.stack_size = 8192; // Aumentar stack size por si acaso
    
    ESP_LOGI(TAG, "📝 Configurando servidor en puerto %d...", config.server_port);
    
    esp_err_t ret = httpd_start(&server, &config);
    ESP_LOGI(TAG, "📡 Resultado de httpd_start: %s", esp_err_to_name(ret));
    
    if (ret == ESP_OK) {
        // Registrar handlers
        ret = httpd_register_uri_handler(server, &root);
        ESP_LOGI(TAG, "📄 Handler root: %s", esp_err_to_name(ret));
        
        ret = httpd_register_uri_handler(server, &status);
        ESP_LOGI(TAG, "📄 Handler status: %s", esp_err_to_name(ret));
        
        ret = httpd_register_uri_handler(server, &led_control);
        ESP_LOGI(TAG, "📄 Handler led: %s", esp_err_to_name(ret));
        
        ESP_LOGI(TAG, "✅ Servidor web INICIADO correctamente");
        ESP_LOGI(TAG, "🌐 URLs disponibles:");
        ESP_LOGI(TAG, "   http://%s/", wifi_get_ip());
        ESP_LOGI(TAG, "   http://%s/status", wifi_get_ip());
    } else {
        ESP_LOGE(TAG, "❌ ERROR al iniciar servidor web: %s", esp_err_to_name(ret));
        server = NULL;
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
<<<<<<< HEAD
    
    // Leer datos del sensor DHT11
    dht11_data_t sensor_data = dht11_read_data();
    status.temperature = sensor_data.temperature;
    status.humidity = sensor_data.humidity;
    status.sensor_valid = sensor_data.valid;
=======
    status.temperature = hardware_get_temperature();
    status.humidity = hardware_get_humidity();
    status.sensor_valid = hardware_sensor_valid();
>>>>>>> dfecfd08907d8c1119f8ae96d2f40eb00149c613
    
    return status;
}