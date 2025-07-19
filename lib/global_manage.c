// Ficheiro: global_manage.c (Implementação Completa)

#include "global_manage.h" // Inclui a nossa própria definição de estrutura e funções
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "aht20.h"      // Biblioteca do seu sensor de umidade/temperatura
#include "bmp280.h"     // Biblioteca do seu sensor de pressão/temperatura
#include <math.h>

// --- Definições do Hardware ---
#define I2C_PORT i2c0
#define I2C_SDA_PIN 0
#define I2C_SCL_PIN 1
#define SEA_LEVEL_PRESSURE 101325.0 // Pressão ao nível do mar em Pa

// TODO: VERIFIQUE E AJUSTE ESTES PINOS DE ACORDO COM A SUA PLACA BITDOGLAB
// #define BUZZER_PIN 15
// #define LED_R_PIN  12 // Exemplo para um LED RGB
// #define LED_G_PIN  13
// #define LED_B_PIN  14

// --- Variáveis de Estado Globais (visíveis apenas neste ficheiro) ---

// Estrutura que armazena todos os dados e configurações.
// 'static' garante que esta variável só pode ser acessada pelas funções deste ficheiro.
static SENSOR_DATA g_sensor_data;

// Parâmetros de calibração do BMP280, lidos uma vez na inicialização.
static struct bmp280_calib_param bmp_params;

/**
 * @brief Callback chamado periodicamente pelo timer.
 * * Esta função é o núcleo da coleta de dados. Ela lê os valores brutos dos sensores,
 * aplica as conversões e os offsets de calibração, e atualiza a estrutura de dados global.
 * Também preenche o histórico de dados para o gráfico.
 * * @param t Ponteiro para a estrutura do timer (não utilizado aqui).
 * @return true para manter o timer agendado.
 */
void ler_sensores() { // struct repeating_timer *t
    // --- Leitura do Sensor BMP280 ---
    int32_t raw_temp_bmp, raw_pressure;
    bmp280_read_raw(I2C_PORT, &raw_temp_bmp, &raw_pressure); 
    
    // Converte os valores brutos para unidades legíveis
    int32_t temp_converted = bmp280_convert_temp(raw_temp_bmp, &bmp_params); 
    int32_t press_converted = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &bmp_params); 

    // Aplica os offsets de calibração e armazena na estrutura global
    g_sensor_data.temperatura_bmp = (temp_converted / 100.0f) + g_sensor_data.offset_temp;
    g_sensor_data.pressao_hpa = (press_converted / 100.0f) + g_sensor_data.offset_press;

    // Calcula a altitude
    g_sensor_data.altitude = 44330.0 * (1.0 - pow(press_converted / SEA_LEVEL_PRESSURE, 0.1903)); 

    // --- Leitura do Sensor AHT20 ---
    AHT20_Data aht_data;
    if (aht20_read(I2C_PORT, &aht_data)) { 
        g_sensor_data.umidade_aht = aht_data.humidity;
    } else {
        printf("Erro na leitura do AHT20!\n");
    }

    // --- Atualiza o Buffer Circular para o Gráfico ---
    // Adiciona a leitura de temperatura mais recente ao histórico
    g_sensor_data.hist_temp[g_sensor_data.hist_idx] = g_sensor_data.temperatura_bmp;
    // Avança o índice do buffer, voltando a 0 se chegar ao fim (comportamento circular)
    g_sensor_data.hist_idx = (g_sensor_data.hist_idx + 1) % 20;

    // return true; // Retorna true para que o timer continue a ser chamado
}

/**
 * @brief Inicializa todo o sistema de gerenciamento de sensores.
 * * Esta função deve ser chamada uma única vez a partir do seu 'main'.
 * Ela configura o hardware I2C, inicializa os sensores e agenda o timer
 * para a coleta de dados periódica.
 */
void init_sensor_manager(void) {
    // Inicializa o hardware I2C
    i2c_init(I2C_PORT, 400 * 1000); 
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C); 
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C); 
    gpio_pull_up(I2C_SDA_PIN); 
    gpio_pull_up(I2C_SCL_PIN); 

    // Inicializa os sensores
    bmp280_init(I2C_PORT); 
    bmp280_get_calib_params(I2C_PORT, &bmp_params); 
    aht20_reset(I2C_PORT); 
    aht20_init(I2C_PORT); 
    
    // Zera os valores iniciais na estrutura de dados
    memset(&g_sensor_data, 0, sizeof(SENSOR_DATA));
    g_sensor_data.limite_min_temp = 20; // Define um limite mínimo padrão
    g_sensor_data.limite_max_temp = 30; // Define um limite máximo padrão

    // Agenda o timer para chamar a função 'ler_sensores_callback' a cada 2000 ms (2 segundos)
    // static struct repeating_timer timer;
    // add_repeating_timer_ms(2000, ler_sensores_callback, NULL, &timer);
}

/**
 * @brief Retorna um ponteiro para a estrutura de dados dos sensores.
 * * O servidor web usará esta função para obter os dados mais recentes a serem enviados
 * para a interface web no formato JSON.
 * @return Ponteiro para a estrutura SENSOR_DATA global.
 */
SENSOR_DATA* get_sensor_data(void) {
    return &g_sensor_data;
}

/**
 * @brief Define o valor do offset de calibração para a temperatura.
 * * Esta função será chamada pelo servidor web quando o utilizador submeter
 * um novo valor de offset no formulário da página.
 * @param offset O novo valor do offset.
 */
void set_temp_offset(float offset) {
    g_sensor_data.offset_temp = offset;
}

/**
 * @brief Define o valor do offset de calibração para a pressão.
 * @param offset O novo valor do offset.
 */
void set_press_offset(float offset) {
    g_sensor_data.offset_press = offset;
}

/**
 * @brief Define os limites de temperatura para alertas.
 * @param min Temperatura mínima.
 * @param max Temperatura máxima.
 */
void set_limites_temp(int min, int max) {
    g_sensor_data.limite_min_temp = min;
    g_sensor_data.limite_max_temp = max;
}