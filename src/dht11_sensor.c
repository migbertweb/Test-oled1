#include "dht11_sensor.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "DHT11";

// Función de delay personalizada con mejor precisión
static void dht11_delay_us(uint32_t us) {
    uint32_t m = esp_timer_get_time();
    if(us) {
        uint32_t e = (m + us);
        if(m > e) { // overflow
            while(esp_timer_get_time() > e) {
                asm volatile ("nop");
            }
        }
        while(esp_timer_get_time() < e) {
            asm volatile ("nop");
        }
    }
}

static bool dht11_read_bit(void) {
    // Esperar a que el pin baje (inicio de bit)
    int timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 0) {
        if (timeout++ > 100) return false;
        dht11_delay_us(1);
    }
    
    // Medir el tiempo en alto
    timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 1) {
        if (timeout++ > 100) return false;
        dht11_delay_us(1);
    }
    
    // Si el tiempo en alto > 30us, es un 1, sino 0
    return (timeout > 30);
}

static bool dht11_read_byte(uint8_t *byte) {
    *byte = 0;
    for (int i = 0; i < 8; i++) {
        *byte <<= 1;
        if (dht11_read_bit()) {
            *byte |= 1;
        }
    }
    return true;
}

bool dht11_init(void) {
    ESP_LOGI(TAG, "Inicializando sensor DHT11 en GPIO %d", DHT11_GPIO);
    
    // Configuración del pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DHT11_GPIO),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,  // Open drain para mejor comunicación
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando GPIO: %s", esp_err_to_name(ret));
        return false;
    }

    // Establecer pin en alto inicialmente
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo para estabilización

    ESP_LOGI(TAG, "DHT11 inicializado correctamente");
    return true;
}

dht11_data_t dht11_read_data(void) {
    dht11_data_t data = {0};
    data.valid = false;
    
    static uint32_t last_read_time = 0;
    uint32_t current_time = esp_timer_get_time() / 1000000; // Convertir a segundos
    
    // Asegurar al menos 2 segundos entre lecturas
    if (current_time - last_read_time < 2) {
        ESP_LOGW(TAG, "Lectura muy frecuente, esperar %d segundos", 2 - (current_time - last_read_time));
        vTaskDelay(pdMS_TO_TICKS((2 - (current_time - last_read_time)) * 1000));
    }
    
    uint8_t bytes[5] = {0};
    int retry_count = 0;
    const int MAX_RETRIES = 3;
    
    while (retry_count < MAX_RETRIES) {
        // Iniciar comunicación
        gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(DHT11_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(50));  // Estabilizar nivel alto
        
        gpio_set_level(DHT11_GPIO, 0);
        dht11_delay_us(20000); // 20ms low (más largo que los 18ms mínimos)
        
        // Fin de señal de inicio
        gpio_set_level(DHT11_GPIO, 1);
        dht11_delay_us(40);    // 40us high
    
        // Cambiar a entrada con pull-up
        gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
        gpio_set_pull_mode(DHT11_GPIO, GPIO_PULLUP_ENABLE);
        
        // Esperar respuesta del sensor (pulso bajo inicial)
        int timeout = 0;
        while (gpio_get_level(DHT11_GPIO) == 1) {
            if (timeout++ > 200) { // Aumentado el timeout
                if (retry_count < MAX_RETRIES - 1) {
                    ESP_LOGW(TAG, "Timeout esperando respuesta DHT11, intento %d", retry_count + 1);
                    vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar antes de reintentar
                    retry_count++;
                    continue;
                }
                ESP_LOGE(TAG, "Timeout final esperando respuesta DHT11");
                return data;
            }
            dht11_delay_us(1);
        }
        
        // Verificar pulso bajo (80us)
        timeout = 0;
        while (gpio_get_level(DHT11_GPIO) == 0) {
            if (timeout++ > 100) {
                ESP_LOGW(TAG, "Pulso bajo muy largo");
                break;
            }
            dht11_delay_us(1);
        }
        
        // Verificar pulso alto (80us)
        timeout = 0;
        while (gpio_get_level(DHT11_GPIO) == 1) {
            if (timeout++ > 100) {
                ESP_LOGW(TAG, "Pulso alto muy largo");
                break;
            }
            dht11_delay_us(1);
        }
    
    // Leer los 5 bytes de datos
    for (int i = 0; i < 5; i++) {
        if (!dht11_read_byte(&bytes[i])) {
            ESP_LOGE(TAG, "Error leyendo byte %d", i);
            return data;
        }
    }
    
        // Verificar checksum
        uint8_t checksum = bytes[0] + bytes[1] + bytes[2] + bytes[3];
        if (bytes[4] != checksum) {
            ESP_LOGW(TAG, "Error de checksum: %d != %d, intento %d", bytes[4], checksum, retry_count + 1);
            if (retry_count < MAX_RETRIES - 1) {
                retry_count++;
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            ESP_LOGE(TAG, "Error final de checksum después de %d intentos", MAX_RETRIES);
            return data;
        }
        
        // Validar datos
        if (bytes[0] > 100 || bytes[2] > 125) {
            ESP_LOGW(TAG, "Datos fuera de rango: Hum=%d%%, Temp=%d°C", bytes[0], bytes[2]);
            if (retry_count < MAX_RETRIES - 1) {
                retry_count++;
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            return data;
        }
        
        // Convertir datos
        data.humidity = bytes[0] + (bytes[1] * 0.1);
        data.temperature = bytes[2] + (bytes[3] * 0.1);
        data.valid = true;
        
        // Actualizar tiempo de última lectura
        last_read_time = current_time;
        
        ESP_LOGI(TAG, "DHT11 OK - Temp: %.1f°C, Hum: %.1f%% (intento %d)", 
                data.temperature, data.humidity, retry_count + 1);
        return data;
    }
    
    ESP_LOGE(TAG, "Todos los intentos fallaron");
    return data;
}

const char* dht11_get_status(void) {
    dht11_data_t data = dht11_read_data();
    if (data.valid) {
        return "OK";
    }
    return "ERROR";
}