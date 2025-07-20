#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"     // API de PWM para controle de sinais sonoros
#include "hardware/clocks.h"  // API de clocks do RP2040 para gerenciar frequências

#include "server.h"
#include "global_manage.h"
#include "connect_wifi.h"
#include "alerta_manager.h"
#include "ssd1306.h"
#include "matriz.h"

#include "pio_matrix.pio.h"

#include "FreeRTOS.h"        // Kernel FreeRTOS
#include "task.h"            // API de criação e controle de tarefas FreeRTOS

bool alerta;
bool connected;

// Tarefa que controla o web_server
void vServerTask()
{
    int result = connect_wifi();
    if (result) return;

    start_http_server();

    connected=true;

    while (true)
    {
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        vTaskDelay(pdMS_TO_TICKS(1));    // Esperar 100ms
    }

    cyw43_arch_deinit();
}

void vSensorTask()
{
    init_sensor_manager();

    while (true)
    {
        ler_sensores();
        vTaskDelay(pdMS_TO_TICKS(2000));    // Esperar 100ms
    }
}

void vAlerta1Task()
{
    ssd1306_t ssd;
    PIO pio = pio0;
    uint sm = pio_init(pio); // Inicializa State Machine para PIO

    display_init(&ssd);

    SENSOR_DATA *data = get_sensor_data();

    while (true)
    {
        if (connected) 
        {
            if (data->temperatura_bmp>data->limite_max_temp || data->pressao_hpa>data->limite_max_press || data->umidade_aht>data->limite_max_umid || data->altitude>data->limite_max_alt) 
            {
                desenhar_alerta_lim_superior(pio, sm);
                desenha_display_alerta(&ssd);
                alerta=true;
            }
            else if (data->temperatura_bmp<data->limite_min_temp || data->pressao_hpa<data->limite_min_press || data->umidade_aht<data->limite_min_umid || data->altitude<data->limite_min_alt) 
            {
                desenhar_alerta_lim_inferior(pio, sm);
                desenha_display_alerta(&ssd);
                alerta=true;
            }
            else 
            {
                apagar_matriz(pio, sm);
                desenha_display_normal(&ssd);
                alerta=false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));    // Esperar 100ms
    }
}

void vAlerta2Task(void *pvParameters) {
    buzzer_pwm_config();
    leds_init();
    
    while (1) {
        if (alerta && connected) {
            // Em modo alerta, alterna LED e gera tom no buzzer
            gpio_put(LED_RED, 0);
            gpio_put(LED_BLUE, 1);
            pwm_set_gpio_level(BUZZER_PIN, 2048);
            vTaskDelay(pdMS_TO_TICKS(200));

            gpio_put(LED_BLUE, 0);
            gpio_put(LED_RED, 1);
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_put(LED_RED, 0);
        } else {
            // Desliga ambos se não alerta
            gpio_put(LED_BLUE, 0);
            gpio_put(LED_RED, 0);
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

int main()
{
    stdio_init_all();

    xTaskCreate(vServerTask, "Server Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vSensorTask, "Sensor Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vAlerta1Task, "Alerta1 Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vAlerta2Task, "Alerta2 Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    // Inicia o scheduler do FreeRTOS
    vTaskStartScheduler();

    // Se voltar aqui, o bootrom exibe mensagem de erro
    panic_unsupported();
    
    return 0;
}
