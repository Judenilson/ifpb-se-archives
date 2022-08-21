#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <esp_http_server.h>
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "rc522.c"

#define WIFI_SSID		"TP-LINK_FE84"
#define WIFI_PASSWORD	"71656137"
#define MAXIMUM_RETRY   5

#define LIGHT_PIN   GPIO_NUM_32
#define RELE_PIN    GPIO_NUM_33

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;

static const char* TAGM = "MainAPP";

static uint8_t light_state = 0; // off
static int s_retry_num = 0;

char lista[2000] = "";
char lista_nomes[2000] = "";
int indexLista = 0;
int modo = 0;
const char separador[2] = {'!'};

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAGM, "Tente reconectar ao AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAGM,"Falha de conexao com o AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAGM, "IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAGM, "wifi_init_sta finalizado.");

    /* Aguardando até que a conexão seja estabelecida (WIFI_CONNECTED_BIT) ou a conexão falhe pelo máximo
    número de tentativas (WIFI_FAIL_BIT). */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() retorna os bits antes da chamada ser retornada, portanto, podemos testar 
    qual evento realmente aconteceu. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAGM, "Conectado ao AP SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASSWORD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAGM, "Falha em conectar ao AP SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASSWORD);
    } else {
        ESP_LOGE(TAGM, "EVENTO INESPERADO");
    }
}

const char menu_resp[] = "<h3>Controle de Presen&ccedila</h3><button><a href=\"/light\">Luzes</a></button><br><button><a href=\"/lista\">Lista de Presen&ccedila</a></button><br><br><br><button><a href=\"/telegram\">Iniciar Aula</button></a><a href=\"/aulafim\"><button>Encerrar Aula</button></a>";
const char on_resp[] = "<h3>LUZES da Sala: ACESAS</h3><a href=\"/off\"><button>DESLIGAR</button></a><a href=\"/\"><button>VOLTAR</button></a>";
const char off_resp[] = "<h3>LUZES da Sala: APAGADAS</h3><a href=\"/on\"><button>LIGAR</button></a><a href=\"/\"><button>VOLTAR</button></a>";
const char telegram_resp[] = "<object width='0' height='0' type='text/html' data='https://api.telegram.org/bot5775630816:AAEuxojQRdMLpiVQINcnt0_iMWv87YQjsaM/sendMessage?chat_id=-708112312&text=AULA_INICIADA!!!'></object>Mensagem de in&iacutecio de aula enviada para o Telegram!<br><br><a href=\"/\"><button>VOLTAR</button></a>";
const char aulafim_telegram_resp[] = "<object width='0' height='0' type='text/html' data='https://api.telegram.org/bot5775630816:AAEuxojQRdMLpiVQINcnt0_iMWv87YQjsaM/sendMessage?chat_id=-708112312&text=AULA_ENCERRADA!!!'></object>Mensagem de aula encerrada enviada para o Telegram!<br><br><a href=\"/\"><button>VOLTAR</button></a>";

esp_err_t get_handler(httpd_req_t *req)
{	
	httpd_resp_send(req, menu_resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t lista_handler(httpd_req_t *req)
{	
    char resposta[3000]="<h3>Alunos Presentes</h3>";
    char botao[45] = "<br><a href=\"/\"><button>VOLTAR</button></a>";
    strcat(resposta, lista_nomes);
    strcat(resposta, botao);
    
	httpd_resp_send(req, resposta, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t telegram_handler(httpd_req_t *req)
{	
    modo = 1; //modo que habilita salvar as tags.
	httpd_resp_send(req, telegram_resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t aulafim_handler(httpd_req_t *req)
{	
    char lista_Empty[2000] = "";
    char lista_nomes_Empty[2000] = "";
    strcpy(lista, lista_Empty);
    strcpy(lista_nomes, lista_nomes_Empty);
    modo = 0;  //modo que desabilita salvar as tags.

	httpd_resp_send(req, aulafim_telegram_resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t light_handler(httpd_req_t *req)
{
	if (light_state == 0)
		httpd_resp_send(req, off_resp, HTTPD_RESP_USE_STRLEN);
	else
		httpd_resp_send(req, on_resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

esp_err_t on_handler(httpd_req_t *req)
{   
    gpio_set_level(LIGHT_PIN, 1);
	light_state = 1;
    httpd_resp_send(req, on_resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t off_handler(httpd_req_t *req)
{
    gpio_set_level(LIGHT_PIN, 0);
	light_state = 0;
    httpd_resp_send(req, off_resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_lista = {
    .uri      = "/lista",
    .method   = HTTP_GET,
    .handler  = lista_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_telegram = {
    .uri      = "/telegram",
    .method   = HTTP_GET,
    .handler  = telegram_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_aulafim = {
    .uri      = "/aulafim",
    .method   = HTTP_GET,
    .handler  = aulafim_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_light = {
    .uri      = "/light",
    .method   = HTTP_GET,
    .handler  = light_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_on = {
    .uri      = "/on",
    .method   = HTTP_GET,
    .handler  = on_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_off = {
    .uri      = "/off",
    .method   = HTTP_GET,
    .handler  = off_handler,
    .user_ctx = NULL
};


httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    config.stack_size = 20480;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
		httpd_register_uri_handler(server, &uri_on);
		httpd_register_uri_handler(server, &uri_off);
		httpd_register_uri_handler(server, &uri_light);
		httpd_register_uri_handler(server, &uri_lista);
		httpd_register_uri_handler(server, &uri_telegram);
		httpd_register_uri_handler(server, &uri_aulafim);
    }

    return server;
}

void adicionarLista (char tag[], char nome[], int idx){
    int pos = idx * 60;
    int j = 0;

    for (int i = pos; i < (pos + 25); i++){
        lista[i] = tag[j];
        j++;
    }
    
    j = 0;
    for (int i = (pos + 25); i < (pos + 60); i++){
        lista[i] = nome[j];
        j++;
    }
    char outraLinha[5] = "<br>";
    strcat(lista_nomes, nome);
    strcat(lista_nomes, outraLinha);
    
    indexLista++;
}

int existeTagLista (char tag[]){
    int tamTAG = 25;
    int tamLIST = (int)sizeof(lista);
    int i = 0;
    int j = 0;
    int idx = 0;
    while (i < tamLIST){
        idx = i/60;
        if (j < tamTAG){
            if (lista[i] == tag[j]){
                j++;
                i++;
                if (tag[j] == '!'){
                    printf("Achou tag com index %d \n", idx);
                    return idx;
                }
            } else {
                i = (idx + 1)*60;
            }        
        } else {
            j = 0;
        }
    }
    printf("NÃO Achou tag \n");
    return -1;
}

void tag_handler(uint8_t* sn) { // o número de série tem sempre 5 bytes
    char tag[25] = "";    
    char nome[35] = "Judenilson";

    for (int i = 0; i < 5; i++){
        int num = sn[i];
        char snum[5], ponto[2] = {'.'};
        snprintf(snum, 5, "%d", num);        
        strcat(tag, snum);
        if (i != 4){
            strcat(tag, ponto);
        }else{
            strcat(tag, separador);
        }
    }
    
    if ((existeTagLista(tag) == -1) && (modo == 1)){
        adicionarLista(tag, nome, indexLista);
    }
    
    for (int i = 0; i < 200; i++){ //usado 200 pra ficar fácil de observar no console. Pra enxergar o array completo tem q ser 2000.
        printf("%c", lista[i]);
        if ((i % 60 == 0) && (i > 0)){
            printf("\n");        
        }
    } 
    printf("\n");

    vTaskDelay(10);

    gpio_set_level(RELE_PIN, 1);
    vTaskDelay(100);
    gpio_set_level(RELE_PIN, 0);
}

void app_main(void)
{        
    const rc522_start_args_t rfid = {
        .miso_io  = 25,
        .mosi_io  = 23,
        .sck_io   = 19,
        .sda_io   = 22,
        .callback = &tag_handler,
    };

    rc522_start(rfid);

    gpio_set_direction(RELE_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(RELE_PIN, 0);

	gpio_set_direction(LIGHT_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LIGHT_PIN, 0);
	light_state = 0;

    // Initialize NVS caso necessite guardar alguma config na flash.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAGM, "ESP_WIFI_MODE_STA");
    wifi_init_sta();        
	setup_server();
}