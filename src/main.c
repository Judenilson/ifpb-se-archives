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

#define WIFI_SSID		"Judenilson"
#define WIFI_PASSWORD	"Judenilson82"
#define MAXIMUM_RETRY   5

#define LIGHT_PIN   GPIO_NUM_32
#define RELE_PIN    GPIO_NUM_33

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char* TAGM = "MainAPP";

static uint8_t light_state = 0; // off
static int s_retry_num = 0;

char lista_resp[200] = "TAG Atual: ";

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

const char menu_resp[] = "<h3>Controle de Presen&ccedila</h3><button><a href=\"/light\">Luzes</a></button><br><button><a href=\"/lista\">Lista de Presen&ccedila</a></button><br><br><br><button><a href=\"/telegram\">Iniciar Aula</button></a>";
const char on_resp[] = "<h3>LUZES da Sala: ACESAS</h3><a href=\"/off\"><button>DESLIGAR</button</a><a href=\"/\"><button>VOLTAR</button</a>";
const char off_resp[] = "<h3>LUZES da Sala: APAGADAS</h3><a href=\"/on\"><button>LIGAR</button></a><a href=\"/\"><button>VOLTAR</button</a>";
const char telegram_resp[] = "<object width='0' height='0' type='text/html' data='https://api.telegram.org/bot5775630816:AAEuxojQRdMLpiVQINcnt0_iMWv87YQjsaM/sendMessage?chat_id=-708112312&text=AULA_INICIADA!!!'></object>Mensagem Enviada para o Telegram!<br><br><a href=\"/\"><button>VOLTAR</button</a>";


esp_err_t get_handler(httpd_req_t *req)
{	
	httpd_resp_send(req, menu_resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t lista_handler(httpd_req_t *req)
{	
	httpd_resp_sendstr(req, lista_resp);
    return ESP_OK;
}

esp_err_t telegram_handler(httpd_req_t *req)
{	
	httpd_resp_send(req, telegram_resp, HTTPD_RESP_USE_STRLEN);
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

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
		httpd_register_uri_handler(server, &uri_on);
		httpd_register_uri_handler(server, &uri_off);
		httpd_register_uri_handler(server, &uri_light);
		httpd_register_uri_handler(server, &uri_lista);
		httpd_register_uri_handler(server, &uri_telegram);
    }

    return server;
}

void adicionarLista (char tag[], char nome[], int index){
    int pos = index * 50;
    int j = 0;

    for (int i = pos; i < (pos + 20); i++){
        lista_resp[i] = tag[j];
        j++;
    }
    
    j = 0;
    for (int i = (pos + 20); i < (pos + 50); i++){
        lista_resp[i] = nome[j];
        j++;
    }
}

void tag_handler(uint8_t* sn) { // o número de série tem sempre 5 bytes
    char vazia[200] = "TAG Atual: ";
    strcpy(lista_resp, vazia);

    char tag[20] = "";    
    char nome[35] = "Judenilson";
    for(int i = 10; i < 35; i++){
        nome[i] = ' ';
    }nome[35] = '\n';

    for (int i = 0; i < 5; i++){
        int num = sn[i];
        char snum[5], ponto[2] = {'.'};
        snprintf(snum, 5, "%d", num);        
        strcat(tag, snum);
        strcat(tag, ponto);
    }
    
    adicionarLista(tag, nome, 0);
    for (int i = 0; i < 200; i++){
        printf("%c", lista_resp[i]);
    } printf("\n");

    vTaskDelay(10);

    // printf("%s \n", tag);
    strcat(lista_resp, tag);

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

    //Initialize NVS caso necessite guardar alguma config na flash.
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