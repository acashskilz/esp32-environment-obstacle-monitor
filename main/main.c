#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// Hardware Pin Mapping
#define DHT_DATA_PIN      4
#define OBSTACLE_SENSOR_PIN 27

// Using volatile because the ISR modifies this outside the main loop flow
volatile bool object_nearby = false;

// ISR - Stored in RAM for speed. Triggers when the sensor state changes.
static void IRAM_ATTR obstacle_handler(void* arg) {
    object_nearby = true; 
}

// Low-level helper to track signal timing for the 1-wire protocol
static int wait_for_signal(int pin, int level, int timeout_us) {
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(pin) != level) {
        if (esp_timer_get_time() - start > timeout_us) {
            return -1; 
        }
    }
    return (int)(esp_timer_get_time() - start);
}

// Manual DHT11 bit-banging driver
int fetch_dht11_data(int *temp_out, int *hum_out) {
    uint8_t bits[5] = {0};

    // Phase 1: Host Start Signal
    gpio_set_direction(DHT_DATA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_DATA_PIN, 0); 
    vTaskDelay(pdMS_TO_TICKS(20)); 
    gpio_set_level(DHT_DATA_PIN, 1); 
    esp_rom_delay_us(30); 
    
    // Phase 2: Sensor Response
    gpio_set_direction(DHT_DATA_PIN, GPIO_MODE_INPUT);
    if (wait_for_signal(DHT_DATA_PIN, 0, 100) == -1) return -1; 
    if (wait_for_signal(DHT_DATA_PIN, 1, 100) == -1) return -1; 
    if (wait_for_signal(DHT_DATA_PIN, 0, 100) == -1) return -1; 

    // Phase 3: Capturing the 40-bit stream
    for (int i = 0; i < 40; i++) {
        if (wait_for_signal(DHT_DATA_PIN, 1, 100) == -1) return -1;
        
        int high_duration = wait_for_signal(DHT_DATA_PIN, 0, 100);
        if (high_duration == -1) return -1;

        // Threshold of 40us determines if bit is 0 or 1
        if (high_duration > 40) {
            bits[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Phase 4: Validation
    if (bits[4] == ((bits[0] + bits[1] + bits[2] + bits[3]) & 0xFF)) {
        *hum_out = bits[0];
        *temp_out = bits[2];
        return 0; 
    }
    return -2; 
}

void setup_hardware(void) {
    // Configure Obstacle Sensor (Pull-up is usually needed for IR modules)
    gpio_config_t sensor_cfg = {
        .pin_bit_mask = (1ULL << OBSTACLE_SENSOR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE // Trigger when signal drops (Object detected)
    };
    gpio_config(&sensor_cfg);

    // Initial DHT idle state
    gpio_set_direction(DHT_DATA_PIN, GPIO_MODE_OUTPUT_OD); 
    gpio_set_level(DHT_DATA_PIN, 1);
}

void app_main(void) {
    setup_hardware();

    // Start the interrupt engine
    gpio_install_isr_service(0);
    gpio_isr_handler_add(OBSTACLE_SENSOR_PIN, obstacle_handler, (void*) OBSTACLE_SENSOR_PIN);

    printf("System initialized. Monitoring for obstacles and environment...\n");

    while (1) {
        // Handle Obstacle Event
        if (object_nearby) {
            printf("\n>>> [EVENT] OBSTACLE DETECTED! <<<\n");
            object_nearby = false; 
        }

        // Handle Periodic Env Reading
        int t = 0, h = 0;
        int result = fetch_dht11_data(&t, &h);

        if (result == 0) {
            printf("Room Data -> Temp: %d C | Humidity: %d %%\n", t, h);
        } else if (result == -1) {
            printf("DHT Error: Connection timeout.\n");
        } else if (result == -2) {
            printf("DHT Error: Checksum mismatch (Noise).\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1500)); 
    }
}