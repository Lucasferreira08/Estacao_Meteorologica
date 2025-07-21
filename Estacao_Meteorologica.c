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

#define BUTTON_PIN 5

bool alerta=false;
bool connected=false;
ssd1306_t ssd;

// ==========================================================
// LÓGICA DO BOTÃO COM INTERRUPÇÃO
// ==========================================================
// Callback: esta função é executada quando a interrupção do botão ocorre
void btn_callback(uint gpio, uint32_t events) {
    // Lógica simples de debounce: ignora interrupções por 250ms
    static uint32_t last_press_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_press_time < 250) {
        return;
    }
    last_press_time = current_time;

    // Ação principal da interrupção
    if (gpio == BUTTON_PIN) {
        connected = true; // Ativa a flag 'connected'
        printf("Botão pressionado! Flag 'connected' ativada.\n");
    }
}

// Função para configurar o botão
void setup_button() {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Habilita resistor de pull-up interno

    // Configura a interrupção para ser acionada na borda de descida (quando o botão é pressionado)
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
}

// Tarefa que controla o web_server
void vServerTask()
{
    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "CONECTANDO...", 0, 0);
    ssd1306_send_data(&ssd);
    vTaskDelay(pdMS_TO_TICKS(2000));     

    char* result = connect_wifi();
    start_http_server();

    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "CONECTADO", 0, 0);
    ssd1306_draw_string(&ssd, "IP:", 0, 15); // Ajuste de posição Y para melhor visualização
    if (result) {
        ssd1306_draw_string(&ssd, result, 25, 15);
    }
    ssd1306_draw_string(&ssd, "Aperte botao A", 0, 30);
    ssd1306_draw_string(&ssd, "para iniciar...", 0, 40);

    ssd1306_send_data(&ssd);

    while (true)
    {
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        vTaskDelay(pdMS_TO_TICKS(50));    // Esperar 100ms
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
    PIO pio = pio0;
    uint sm = pio_init(pio); // Inicializa State Machine para PIO

    SENSOR_DATA *data = get_sensor_data();

    while (true)
    {
        if (connected) 
        {
            if (data->temperatura_bmp>data->limite_max_temp || data->pressao_hpa>data->limite_max_press || data->umidade_aht>data->limite_max_umid || data->altitude>data->limite_max_alt) 
            {
                desenhar_alerta_lim_superior(pio, sm);
                desenha_display_alerta_sup(&ssd, data);
                alerta=true;
            }
            else if (data->temperatura_bmp<data->limite_min_temp || data->pressao_hpa<data->limite_min_press || data->umidade_aht<data->limite_min_umid || data->altitude<data->limite_min_alt) 
            {
                desenhar_alerta_lim_inferior(pio, sm);
                desenha_display_alerta_inf(&ssd, data);
                alerta=true;
            }
            else 
            {
                apagar_matriz(pio, sm);
                desenha_display_normal(&ssd, data);
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
    display_init(&ssd);

    setup_button();

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
