// // Ficheiro: global_manage.h
// #ifndef GLOBAL_MANAGE_H
// #define GLOBAL_MANAGE_H

// // Estrutura para armazenar os dados e configurações atuais
// typedef struct {
//     float temperatura_bmp;
//     float umidade_aht;
//     float pressao_hpa;
//     float altitude;
//     // Offsets de calibração
//     float offset_temp;
//     float offset_press;
//     // Limites para alertas (exemplo)
//     int limite_min_temp;
//     int limite_max_temp;
//     // Dados históricos para o gráfico (últimos 20 valores)
//     float hist_temp[20];
//     int hist_idx; // Índice atual do buffer circular
// } SENSOR_DATA;

// // Funções para inicializar e obter os dados
// void init_sensor_manager(void);
// SENSOR_DATA* get_sensor_data(void);

// void ler_sensores();

// // Funções para definir configurações via web
// void set_temp_offset(float offset);
// void set_press_offset(float offset);
// void set_limites_temp(int min, int max);

// #endif

// Ficheiro: global_manage.h

#ifndef GLOBAL_MANAGE_H
#define GLOBAL_MANAGE_H

#include "pico/stdlib.h"

// Estrutura completa para armazenar todos os dados e configurações do sistema.
typedef struct {
    // Dados lidos dos sensores
    float temperatura_bmp;
    float umidade_aht;
    float pressao_hpa;
    float altitude;

    // Offsets de calibração configurados via web
    float offset_temp;
    float offset_press;

    // Limites de alerta de temperatura configurados via web
    int limite_min_temp;
    int limite_max_temp;
    
    // Dados históricos para o gráfico da interface web (últimos 20 valores)
    float hist_temp[20];
    int hist_idx; // Índice atual para o buffer circular
} SENSOR_DATA;


/**
 * @brief Inicializa o hardware (I2C, sensores, pinos de alerta) e o timer para leitura periódica.
 * Deve ser chamada uma vez no início do programa.
 */
void init_sensor_manager(void);

/**
 * @brief Retorna um ponteiro para a estrutura de dados global.
 * Usado pelo servidor web para obter os dados atuais e enviá-los como JSON.
 * @return SENSOR_DATA* Ponteiro para os dados dos sensores.
 */
SENSOR_DATA* get_sensor_data(void);

void ler_sensores();

/**
 * @brief Define o offset de calibração para o sensor de temperatura.
 * @param offset Valor do offset recebido da interface web.
 */
void set_temp_offset(float offset);

/**
 * @brief Define o offset de calibração para o sensor de pressão.
 * @param offset Valor do offset recebido da interface web.
 */
void set_press_offset(float offset);

/**
 * @brief Define os limites mínimo e máximo de temperatura para os alertas.
 * @param min Limite mínimo recebido da interface web.
 * @param max Limite máximo recebido da interface web.
 */
void set_limites_temp(int min, int max);

#endif