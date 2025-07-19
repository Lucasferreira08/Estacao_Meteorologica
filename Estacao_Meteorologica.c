#include <stdio.h>
#include "pico/stdlib.h"

#include "server.h"
#include "global_manage.h"
#include "connect_wifi.h"

#include "FreeRTOS.h"        // Kernel FreeRTOS
#include "task.h"            // API de criação e controle de tarefas FreeRTOS

// Tarefa que controla o web_server
void vServerTask()
{
    int result = connect_wifi();
    if (result) return;

    start_http_server();

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

int main()
{
    stdio_init_all();

    xTaskCreate(vServerTask, "Server Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vSensorTask, "Sensor Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    // Inicia o scheduler do FreeRTOS
    vTaskStartScheduler();

    // Se voltar aqui, o bootrom exibe mensagem de erro
    panic_unsupported();
    
    return 0;
}
