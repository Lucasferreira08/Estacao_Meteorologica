#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"      // API de PWM para controle de sinais sonoros
#include "hardware/clocks.h"   // API de clocks do RP2040 para gerenciar frequências

// Inclusão dos módulos personalizados do projeto
#include "server.h"
#include "global_manage.h"
#include "connect_wifi.h"
#include "alerta_manager.h"
#include "ssd1306.h"
#include "matriz.h"

// Inclusão do header gerado pelo pioasm para a matriz de LEDs
#include "pio_matrix.pio.h"

// Inclusão dos cabeçalhos do FreeRTOS
#include "FreeRTOS.h"          // Kernel FreeRTOS
#include "task.h"              // API de criação e controle de tarefas FreeRTOS

// Definição do pino do botão de entrada
#define BUTTON_PIN 5

// --- Variáveis Globais ---
// Flags para comunicação entre as tarefas
bool alerta = false;       // Flag que indica se um alerta está ativo
bool connected = false;    // Flag que indica se o sistema foi ativado pelo botão
// Instância da estrutura do display OLED
ssd1306_t ssd;

// ==========================================================
// LÓGICA DO BOTÃO COM INTERRUPÇÃO
// ==========================================================
/**
 * @brief Callback executado pela interrupção do GPIO quando o botão é pressionado.
 * * @param gpio O número do pino que causou a interrupção.
 * @param events O tipo de evento (ex: borda de subida/descida).
 */
void btn_callback(uint gpio, uint32_t events) {
    // Lógica simples de debounce para evitar múltiplas detecções por um único pressionamento
    static uint32_t last_press_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_press_time < 250) {
        return; // Ignora o pressionamento se for muito rápido
    }
    last_press_time = current_time;

    // Ação principal da interrupção: ativa o sistema
    if (gpio == BUTTON_PIN) {
        connected = true; // Ativa a flag 'connected' para iniciar o monitoramento de alertas
        printf("Botão pressionado! Flag 'connected' ativada.\n");
    }
}

/**
 * @brief Configura o pino do botão como entrada e habilita a interrupção.
 */
void setup_button() {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Habilita resistor de pull-up interno

    // Configura a interrupção para ser acionada na borda de descida (quando o botão vai de HIGH para LOW)
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
}

// ==========================================================
// DEFINIÇÃO DAS TAREFAS (TASKS) DO FREERTOS
// ==========================================================

/**
 * @brief Tarefa que gerencia a conexão Wi-Fi e o servidor web.
 */
void vServerTask()
{
    // Exibe mensagem de conexão no display
    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "CONECTANDO...", 0, 0);
    ssd1306_send_data(&ssd);
    vTaskDelay(pdMS_TO_TICKS(2000));     

    // Chama a função para conectar ao Wi-Fi e obtém o endereço IP
    char* result = connect_wifi();
    // Inicia o servidor HTTP para a interface web
    start_http_server();

    // Exibe o status de conectado e o IP no display
    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "CONECTADO", 0, 0);
    ssd1306_draw_string(&ssd, "IP:", 0, 15);
    if (result) {
        ssd1306_draw_string(&ssd, result, 25, 15);
    }
    ssd1306_draw_string(&ssd, "Aperte botao A", 0, 30);
    ssd1306_draw_string(&ssd, "para iniciar...", 0, 40);
    ssd1306_send_data(&ssd);

    // Loop principal da tarefa
    while (true)
    {
        // Chama a função de polling da rede, essencial para manter a conexão Wi-Fi ativa
        cyw43_arch_poll();
        // Libera o processador para outras tarefas por 50ms
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Código de desinicialização (geralmente não é alcançado)
    cyw43_arch_deinit();
}

/**
 * @brief Tarefa dedicada à leitura dos sensores.
 */
void vSensorTask()
{
    // Inicializa o gerenciador de sensores (I2C, etc.)
    init_sensor_manager();

    // Loop principal da tarefa
    while (true)
    {
        // Chama a função que lê os dados de todos os sensores
        ler_sensores();
        // Libera o processador para outras tarefas por 2000ms (2 segundos)
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief Tarefa que gerencia a lógica de alertas e a exibição em displays.
 */
void vAlerta1Task()
{
    // Configura o PIO para controlar a matriz de LEDs
    PIO pio = pio0;
    uint sm = pio_init(pio); // Inicializa a máquina de estados do PIO

    // Obtém o ponteiro para a estrutura de dados dos sensores
    SENSOR_DATA *data = get_sensor_data();

    // Loop principal da tarefa
    while (true)
    {
        // Só executa a lógica se o sistema foi ativado pelo botão
        if (connected) 
        {
            // Verifica se algum valor ultrapassou o limite MÁXIMO
            if (data->temperatura_bmp > data->limite_max_temp || data->pressao_hpa > data->limite_max_press || data->umidade_aht > data->limite_max_umid || data->altitude > data->limite_max_alt) 
            {
                desenhar_alerta_lim_superior(pio, sm); // Desenha padrão de alerta na matriz
                desenha_display_alerta_sup(&ssd, data);  // Mostra alerta no OLED
                alerta = true; // Ativa a flag global de alerta
            }
            // Verifica se algum valor ficou abaixo do limite MÍNIMO
            else if (data->temperatura_bmp < data->limite_min_temp || data->pressao_hpa < data->limite_min_press || data->umidade_aht < data->limite_min_umid || data->altitude < data->limite_min_alt) 
            {
                desenhar_alerta_lim_inferior(pio, sm); // Desenha padrão de alerta na matriz
                desenha_display_alerta_inf(&ssd, data);   // Mostra alerta no OLED
                alerta = true; // Ativa a flag global de alerta
            }
            // Se não há alertas
            else 
            {
                apagar_matriz(pio, sm);                 // Apaga a matriz de LEDs
                desenha_display_normal(&ssd, data);     // Mostra dados normais no OLED
                alerta = false; // Desativa a flag global de alerta
            }
        }
        // Libera o processador para outras tarefas por 200ms
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/**
 * @brief Tarefa que controla os atuadores de alerta (Buzzer e LED RGB).
 */
void vAlerta2Task(void *pvParameters) {
    // Inicializa o PWM para o buzzer e os GPIOs para os LEDs
    buzzer_pwm_config();
    leds_init();
    
    // Loop principal da tarefa
    while (1) {
        // Só executa se a flag de alerta e a de conexão estiverem ativas
        if (alerta && connected) {  
            // Em modo alerta, alterna LEDs e gera tom no buzzer
            gpio_put(LED_RED, 0);
            gpio_put(LED_BLUE, 1);
            pwm_set_gpio_level(BUZZER_PIN, 2048); // Liga o som
            vTaskDelay(pdMS_TO_TICKS(200));

            gpio_put(LED_BLUE, 0);
            gpio_put(LED_RED, 1);
            pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o som
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_put(LED_RED, 0);
        } else {
            // Se não há alerta, garante que os atuadores estejam desligados
            gpio_put(LED_BLUE, 0);
            gpio_put(LED_RED, 0);
            pwm_set_gpio_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

// ==========================================================
// FUNÇÃO PRINCIPAL
// ==========================================================
int main()
{
    // Inicializa a comunicação serial e o display
    stdio_init_all();
    display_init(&ssd);

    // Configura o botão com interrupção
    setup_button();

    // Cria as quatro tarefas do sistema
    xTaskCreate(vServerTask, "Server Task", 2048, NULL, 1, NULL); // Aumentado stack para a rede
    xTaskCreate(vSensorTask, "Sensor Task", 1024, NULL, 1, NULL);
    xTaskCreate(vAlerta1Task, "Alerta1 Task", 1024, NULL, 1, NULL);
    xTaskCreate(vAlerta2Task, "Alerta2 Task", 1024, NULL, 1, NULL);

    // Inicia o escalonador do FreeRTOS, que começa a executar as tarefas
    vTaskStartScheduler();

    // O código nunca deve chegar aqui. Se chegar, algo deu muito errado.
    while(1) {};
}