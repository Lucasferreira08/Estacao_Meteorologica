// Ficheiro: server.c (Versão Final com Todos os Endpoints)

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdlib.h>
#include "global_manage.h"

// String HTML_BODY completa. Note que a chamada para atualizarDados() foi reabilitada.
// const char HTML_BODY[] =
//     "<!DOCTYPE html><html><head><title>Estação Meteorológica</title><meta charset='UTF-8'>"
//     "<style>"
//     "body{font-family:sans-serif;background:#f0f2f5;display:flex;justify-content:center;padding:20px}"
//     ".container{width:100%;max-width:800px;background:white;padding:20px;border-radius:10px;box-shadow:0 4px 12px rgba(0,0,0,0.1)}"
//     "h1{text-align:center;color:#333}hr{border:1px solid #eee;margin:20px 0}"
//     ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:20px;text-align:center}"
//     ".card{background:#f8f9fa;padding:15px;border-radius:8px;border:1px solid #ddd}"
//     ".card p{margin:0;font-size:1.5rem;font-weight:bold;color:#007bff}.card span{font-size:0.9rem;color:#6c757d}"
//     ".chart-container{margin-top:20px}.form-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin-top:20px}"
//     ".form-group{display:flex;flex-direction:column}label{margin-bottom:5px;font-weight:bold;color:#555}"
//     "input{padding:8px;border:1px solid #ccc;border-radius:5px;font-size:1rem}"
//     "button{padding:10px;border:none;background:#28a745;color:white;border-radius:5px;cursor:pointer;margin-top:10px}"
//     "</style>"
//     "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
//     "<script>"
//     "let myChart;"
//     "function initChart(){const ctx=document.getElementById('tempChart').getContext('2d');"
//     "myChart=new Chart(ctx,{type:'line',data:{labels:[],datasets:[{label:'Temperatura (C)',data:[],borderColor:'rgb(255, 99, 132)',backgroundColor:'rgba(255, 99, 132, 0.5)',tension:0.1}]},"
//     "options:{scales:{y:{beginAtZero:false}},animation:false}})}"
//     "function setConfig(tipo){const valor=document.getElementById('input_'+tipo).value;"
//     "fetch('/config?'+tipo+'='+valor).then(()=>document.getElementById('input_'+tipo).blur())}"
//     "function atualizarDados(){fetch('/dados_sensores').then(r=>r.json()).then(d=>{"
//     "document.getElementById('temp').innerText=d.temp.toFixed(2);document.getElementById('umid').innerText=d.umid.toFixed(2);"
//     "document.getElementById('press').innerText=d.press.toFixed(2);document.getElementById('alt').innerText=d.alt.toFixed(2);"
//     "if(!document.activeElement.id.includes('input')){document.getElementById('input_offset_temp').value=d.offset_temp;"
//     "document.getElementById('input_offset_press').value=d.offset_press;}"
//     "myChart.data.labels=d.hist_labels;myChart.data.datasets[0].data=d.hist_temp;myChart.update();"
//     "}).catch(e=>console.error('Erro:',e))}"
//     "window.onload=()=>{initChart();atualizarDados();setInterval(atualizarDados,2000)};"
//     "</script></head><body>"
//     "<div class=container><h1>Estação Meteorológica Pico W</h1><div class=grid>"
//     "<div class=card><p id=temp>--</p><span>Temperatura (°C)</span></div>"
//     "<div class=card><p id=umid>--</p><span>Umidade (%)</span></div>"
//     "<div class=card><p id=press>--</p><span>Pressão (hPa)</span></div>"
//     "<div class=card><p id=alt>--</p><span>Altitude (m)</span></div></div>"
//     "<div class=chart-container><canvas id=tempChart></canvas></div><hr>"
//     "<h2>Configurações</h2><div class=form-grid>"
//     "<div class=form-group><label for=input_offset_temp>Offset Temperatura:</label><input type=number step=0.1 id=input_offset_temp>"
//     "<button onclick=\"setConfig('offset_temp')\">Definir</button></div>"
//     "<div class=form-group><label for=input_offset_press>Offset Pressão:</label><input type=number step=0.1 id=input_offset_press>"
//     "<button onclick=\"setConfig('offset_press')\">Definir</button></div>"
//     "</div></div></body></html>";

const char HTML_BODY[] =
    "<!DOCTYPE html><html><head><title>Estação Meteorológica BitDogLab</title><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>" // Responsividade
    "<style>"
    "body{font-family:sans-serif;background:#f0f2f5;display:flex;justify-content:center;padding:10px;margin:0}"
    ".container{width:100%;max-width:800px;background:white;padding:20px;border-radius:10px;box-shadow:0 4px 12px rgba(0,0,0,0.1)}"
    "h1{text-align:center;color:#333}hr{border:1px solid #eee;margin:20px 0}"
    ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:20px;text-align:center}"
    ".card{background:#f8f9fa;padding:15px;border-radius:8px;border:1px solid #ddd}"
    ".card p{margin:0;font-size:1.5rem;font-weight:bold;color:#007bff}.card span{font-size:0.9rem;color:#6c757d}"
    ".chart-container{margin-top:20px}.form-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin-top:20px}"
    ".form-group{display:flex;flex-direction:column}label{margin-bottom:5px;font-weight:bold;color:#555;text-align:left}"
    "input{padding:8px;border:1px solid #ccc;border-radius:5px;font-size:1rem;width:calc(100% - 18px)}"
    "button{padding:10px;border:none;background:#28a745;color:white;border-radius:5px;cursor:pointer;margin-top:10px}"
    "</style>"
    "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
    "<script>"
    "let myChart;"
    "function initChart(){const ctx=document.getElementById('tempChart').getContext('2d');"
    "myChart=new Chart(ctx,{type:'line',data:{labels:[],datasets:[{label:'Temperatura (C)',data:[],borderColor:'rgb(255, 99, 132)',backgroundColor:'rgba(255, 99, 132, 0.5)',tension:0.1}]},"
    "options:{scales:{y:{beginAtZero:false}},animation:false}})}"
    
    // Função genérica para enviar configurações
    "function setConfig(param){const valor=document.getElementById('input_'+param).value;"
    "fetch('/config?'+param+'='+valor).then(()=>document.getElementById('input_'+param).blur())}"

    "function atualizarDados(){fetch('/dados_sensores').then(r=>r.json()).then(d=>{"
    "document.getElementById('temp').innerText=d.temp.toFixed(2);document.getElementById('umid').innerText=d.umid.toFixed(2);"
    "document.getElementById('press').innerText=d.press.toFixed(2);document.getElementById('alt').innerText=d.alt.toFixed(2);"
    "if(!document.activeElement.id.includes('input')){document.getElementById('input_offset_temp').value=d.offset_temp;"
    "document.getElementById('input_offset_press').value=d.offset_press;"
    "document.getElementById('input_limite_min_temp').value=d.limite_min_temp;"
    "document.getElementById('input_limite_max_temp').value=d.limite_max_temp;}"
    "myChart.data.labels=d.hist_labels;myChart.data.datasets[0].data=d.hist_temp;myChart.update();"
    "}).catch(e=>console.error('Erro:',e))}"
    "window.onload=()=>{initChart();atualizarDados();setInterval(atualizarDados,2000)};"
    "</script></head><body>"
    "<div class=container><h1>Estação Meteorológica BitDogLab</h1><div class=grid>"
    "<div class=card><p id=temp>--</p><span>Temperatura (°C)</span></div>"
    "<div class=card><p id=umid>--</p><span>Umidade (%)</span></div>"
    "<div class=card><p id=press>--</p><span>Pressão (hPa)</span></div>"
    "<div class=card><p id=alt>--</p><span>Altitude (m)</span></div></div>"
    "<div class=chart-container><canvas id=tempChart></canvas></div><hr>"
    "<h2>Configurações</h2><div class=form-grid>"
    // Formulário de Calibração
    "<div class=form-group><label for=input_offset_temp>Offset Temperatura:</label><input type=number step=0.1 id=input_offset_temp>"
    "<button onclick=\"setConfig('offset_temp')\">Definir</button></div>"
    "<div class=form-group><label for=input_offset_press>Offset Pressão:</label><input type=number step=0.1 id=input_offset_press>"
    "<button onclick=\"setConfig('offset_press')\">Definir</button></div>"
    // Formulário de Limites de Alerta
    "<div class=form-group><label for=input_limite_min_temp>Alerta Temp. Mínima (°C):</label><input type=number id=input_limite_min_temp>"
    "<button onclick=\"setConfig('limite_min_temp')\">Definir</button></div>"
    "<div class=form-group><label for=input_limite_max_temp>Alerta Temp. Máxima (°C):</label><input type=number id=input_limite_max_temp>"
    "<button onclick=\"setConfig('limite_max_temp')\">Definir</button></div>"
    "</div></div></body></html>";


typedef enum { SENDING_HEADERS, SENDING_BODY } SENDING_PHASE;
typedef struct HTTP_STATE_T {
    char response_buffer[1024];
    const char *response_ptr;  
    size_t response_len;       
    SENDING_PHASE phase;       
} HTTP_STATE;

static void http_close_and_free(struct tcp_pcb *tpcb, HTTP_STATE *state) {
    tcp_arg(tpcb, NULL);
    tcp_sent(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);
    if (state) {
        free(state);
    }
    tcp_close(tpcb);
}

static void http_err(void *arg, err_t err) {
    if (arg) {
        free(arg);
    }
}

static err_t http_send_data(struct tcp_pcb *tpcb, HTTP_STATE *state) {
    u16_t available_len = tcp_sndbuf(tpcb);
    if (available_len > TCP_MSS) {
        available_len = TCP_MSS;
    }
    u16_t send_len = state->response_len < available_len ? state->response_len : available_len;
    if (send_len == 0) return ERR_OK;

    err_t err = tcp_write(tpcb, state->response_ptr, send_len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        http_close_and_free(tpcb, state);
        return err;
    }
    state->response_ptr += send_len;
    state->response_len -= send_len;
    return ERR_OK;
}

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    HTTP_STATE *state = (HTTP_STATE *)arg;
    if (state->response_len > 0) {
        http_send_data(tpcb, state);
    } else {
        if (state->phase == SENDING_HEADERS) {
            state->phase = SENDING_BODY;
            state->response_ptr = HTML_BODY;
            state->response_len = strlen(HTML_BODY);
            http_send_data(tpcb, state);
        } else {
            http_close_and_free(tpcb, state);
        }
    }
    return ERR_OK;
}

// static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
//     if (!p) {
//         http_close_and_free(tpcb, (HTTP_STATE *)arg);
//         return ERR_OK;
//     }
//     tcp_recved(tpcb, p->tot_len);
    
//     char req_buffer[128];
//     pbuf_copy_partial(p, req_buffer, p->len < sizeof(req_buffer) ? p->len : sizeof(req_buffer) - 1, 0);
//     req_buffer[p->len < sizeof(req_buffer) ? p->len : sizeof(req_buffer) - 1] = '\0';
//     pbuf_free(p);

//     if (arg) { 
//         return ERR_OK;
//     }
    
//     HTTP_STATE *state = malloc(sizeof(HTTP_STATE));
//     tcp_arg(tpcb, state);

//     if (strncmp(req_buffer, "GET / ", 6) == 0) {
//         state->phase = SENDING_HEADERS;
//         state->response_len = snprintf(state->response_buffer, sizeof(state->response_buffer),
//             "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", strlen(HTML_BODY));
//         state->response_ptr = state->response_buffer;

//     } else if (strncmp(req_buffer, "GET /dados_sensores", 19) == 0) {
//         state->phase = SENDING_BODY;
        
//         SENSOR_DATA* data = get_sensor_data();
//         char json_body[768];
//         char hist_str[300];
        
//         int offset = 0;
//         offset += sprintf(hist_str + offset, "[");
//         for(int i=0; i<20; ++i) offset += sprintf(hist_str + offset, "%.2f%s", data->hist_temp[i], (i<19) ? "," : "");
//         sprintf(hist_str + offset, "]");
        
//         snprintf(json_body, sizeof(json_body),
//             "{\"temp\":%.2f,\"umid\":%.2f,\"press\":%.2f,\"alt\":%.2f,"
//             "\"offset_temp\":%.2f,\"offset_press\":%.2f,"
//             "\"hist_labels\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20],"
//             "\"hist_temp\":%s}",
//             data->temperatura_bmp, data->umidade_aht, data->pressao_hpa, data->altitude,
//             data->offset_temp, data->offset_press, hist_str);
        
//         state->response_len = snprintf(state->response_buffer, sizeof(state->response_buffer),
//             "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s",
//             strlen(json_body), json_body);
//         state->response_ptr = state->response_buffer;

//     } else if (strncmp(req_buffer, "GET /config?", 12) == 0) {
//         state->phase = SENDING_BODY;
        
//         if (strstr(req_buffer, "offset_temp=")) {
//             set_temp_offset(atof(strstr(req_buffer, "offset_temp=") + 12));
//         } else if (strstr(req_buffer, "offset_press=")) {
//             set_press_offset(atof(strstr(req_buffer, "offset_press=") + 13));
//         }

//         const char* msg = "OK";
//         state->response_len = snprintf(state->response_buffer, sizeof(state->response_buffer),
//             "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", strlen(msg), msg);
//         state->response_ptr = state->response_buffer;
        
//     } else {
//         state->phase = SENDING_BODY;
//         const char* msg = "<h1>404 Not Found</h1>";
//         state->response_len = snprintf(state->response_buffer, sizeof(state->response_buffer),
//             "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", strlen(msg), msg);
//         state->response_ptr = state->response_buffer;
//     }
    
//     tcp_sent(tpcb, http_sent);
//     http_send_data(tpcb, state);
//     tcp_output(tpcb);

//     return ERR_OK;
// }

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        http_close_and_free(tpcb, (HTTP_STATE *)arg);
        return ERR_OK;
    }
    tcp_recved(tpcb, p->tot_len);
    
    char req_buffer[128];
    pbuf_copy_partial(p, req_buffer, p->len < sizeof(req_buffer) ? p->len : sizeof(req_buffer) - 1, 0);
    req_buffer[p->len < sizeof(req_buffer) ? p->len : sizeof(req_buffer) - 1] = '\0';
    pbuf_free(p);

    if (arg) { 
        return ERR_OK;
    }
    
    HTTP_STATE *state = malloc(sizeof(HTTP_STATE));
    tcp_arg(tpcb, state);

    if (strncmp(req_buffer, "GET / ", 6) == 0) {
        // --- Rota para a PÁGINA HTML GRANDE ---
        state->phase = SENDING_HEADERS;
        state->response_len = snprintf(state->response_buffer, sizeof(state->response_buffer),
            "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", strlen(HTML_BODY));
        state->response_ptr = state->response_buffer;

    } else if (strncmp(req_buffer, "GET /dados_sensores", 19) == 0) {
        // --- Rota para os DADOS JSON ---
        state->phase = SENDING_BODY;
        
        SENSOR_DATA* data = get_sensor_data();
        char json_body[768];
        char hist_str[300];
        
        int offset = 0;
        offset += sprintf(hist_str + offset, "[");
        for(int i=0; i<20; ++i) offset += sprintf(hist_str + offset, "%.2f%s", data->hist_temp[i], (i<19) ? "," : "");
        sprintf(hist_str + offset, "]");
        
        // Adiciona os novos limites ao JSON
        snprintf(json_body, sizeof(json_body),
            "{\"temp\":%.2f,\"umid\":%.2f,\"press\":%.2f,\"alt\":%.2f,"
            "\"offset_temp\":%.2f,\"offset_press\":%.2f,"
            "\"limite_min_temp\":%d,\"limite_max_temp\":%d,"
            "\"hist_labels\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20],"
            "\"hist_temp\":%s}",
            data->temperatura_bmp, data->umidade_aht, data->pressao_hpa, data->altitude,
            data->offset_temp, data->offset_press,
            data->limite_min_temp, data->limite_max_temp,
            hist_str);
        
        state->response_len = snprintf(state->response_buffer, sizeof(state->response_buffer),
            "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s",
            strlen(json_body), json_body);
        state->response_ptr = state->response_buffer;

    } else if (strncmp(req_buffer, "GET /config?", 12) == 0) {
        // --- Rota para CONFIGURAÇÕES (EXPANDIDA) ---
        state->phase = SENDING_BODY;
        
        // Verifica cada parâmetro possível e chama a função correspondente
        if (strstr(req_buffer, "offset_temp=")) {
            set_temp_offset(atof(strstr(req_buffer, "offset_temp=") + 12));
        } else if (strstr(req_buffer, "offset_press=")) {
            set_press_offset(atof(strstr(req_buffer, "offset_press=") + 13));
        } else if (strstr(req_buffer, "limite_min_temp=")) {
            // Nota: atoi converte para inteiro
            set_limites_temp(atoi(strstr(req_buffer, "limite_min_temp=") + 16), get_sensor_data()->limite_max_temp);
        } else if (strstr(req_buffer, "limite_max_temp=")) {
            set_limites_temp(get_sensor_data()->limite_min_temp, atoi(strstr(req_buffer, "limite_max_temp=") + 16));
        }

        const char* msg = "OK";
        state->response_len = snprintf(state->response_buffer, sizeof(state->response_buffer),
            "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", strlen(msg), msg);
        state->response_ptr = state->response_buffer;
        
    } else {
        // --- Rota para 404 NOT FOUND ---
        state->phase = SENDING_BODY;
        const char* msg = "<h1>404 Not Found</h1>";
        state->response_len = snprintf(state->response_buffer, sizeof(state->response_buffer),
            "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", strlen(msg), msg);
        state->response_ptr = state->response_buffer;
    }
    
    tcp_sent(tpcb, http_sent);
    http_send_data(tpcb, state);
    tcp_output(tpcb);

    return ERR_OK;
}

static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_arg(newpcb, NULL);
    tcp_recv(newpcb, http_recv);
    tcp_err(newpcb, http_err);
    return ERR_OK;
}

void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    tcp_bind(pcb, IP_ADDR_ANY, 80);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, http_accept);
    printf("Servidor final e completo rodando na porta 80.\n");
}