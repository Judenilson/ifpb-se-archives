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
#include "nvs.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <esp_http_server.h>
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "rc522.c"
#include "TagsList.h"

#define MAX_TAGS 5
#define TAG_ID_LEN 16
#define TAG_NAME_LEN 50
#define NO_TAG "nenhuma tag"
char lastReadTag[TAG_ID_LEN] = NO_TAG;
#define LEN_BOTOES_CADASTRAR_RESPONSE 126

#define ALUNOS_CADASTRADOS "my_key"
char string[((TAG_ID_LEN + TAG_NAME_LEN + 1 + 1) * MAX_TAGS) + 1] = "";

#define WIFI_SSID		"TP-LINK_FE84"
#define WIFI_PASSWORD	"71656137"
#define MAXIMUM_RETRY   5

#define LIGHT_PIN   GPIO_NUM_32
#define RELE_PIN    GPIO_NUM_33

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

TagsList* AlunosCadastrados = NULL;
TagsList* AlunosPresentes = NULL;

static EventGroupHandle_t s_wifi_event_group;

static const char* TAGM = "MainAPP";

static uint8_t light_state = 0; // off
static int s_retry_num = 0;

#define MODE_DONT_READ_TAGS 0
#define MODE_READ_TAGS_FOR_PRESENCE 1
#define MODE_READ_TAGS_FOR_REGISTER 2
int modo = MODE_DONT_READ_TAGS;

void saveFile(const char key[], const char value[])
{
    nvs_handle_t my_handle;
    nvs_open("storage", NVS_READWRITE, &my_handle);

    // Write data, key - "key", value - "value"
    nvs_set_str(my_handle, key, value);
    printf("Write %s: %s\n", key, value);

    nvs_commit(my_handle);
    nvs_close(my_handle);
}

size_t checkKeyExists(nvs_handle_t* handle, const char key[]) {
    size_t required_size = 0;
    nvs_get_str(*handle, key, NULL, &required_size);
    if (required_size > 0) { // se encontrou o cado na memória    
        printf("Found %s: in flash memory\n", key);
    } else {
        printf("MEMORY WARNNING: Could not find %s: in flash memory\n", key);
    }
    return required_size;
}

void deleteFile(const char key[]) {
    nvs_handle_t my_handle;
    nvs_open("storage", NVS_READWRITE, &my_handle);

    size_t required_size = checkKeyExists(&my_handle, key);
    if (required_size > 0) { // se encontrou o cado na memória
        nvs_erase_key(my_handle, key);    
        printf("Erased %s\n", key);
    } else {
        printf("Could not erase %s\n", key);
    }

    nvs_commit(my_handle);
    nvs_close(my_handle);
}

void readFile(const char key[], char* string)
{    
    nvs_handle_t my_handle;
    nvs_open("storage", NVS_READONLY, &my_handle);

    // Read data, key - "key", value - "read_data"
    size_t required_size = checkKeyExists(&my_handle, key);
    if (required_size > 0) { // se encontrou o cado na memória
        char *server_name = malloc(required_size);
        nvs_get_str(my_handle, key, server_name, &required_size);
        strcpy(string, server_name);    
        printf("Read %s: %s\n", key, server_name);
    } else {
        printf("Could not read %s\n", key);
    }

    nvs_close(my_handle);
}

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

const char menu_resp[] = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body><h3>Controle de Presença</h3><a href=\"/light\"><button>Luzes</button></a><br><a href=\"/lista\"><button>Lista de Presença</button></a><a href=\"/cadastros\"><button>Lista de Cadastros</button></a><br><br><br><a href=\"/telegram\"><button>Iniciar Aula</button></a><a href=\"/aulafim\"><button>Encerrar Aula</button></a><br><a href=\"/cadastro\"><button>Cadastrar Aluno</button></a></body></html>";
const char on_resp[] = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body><h3>LUZES da Sala: ACESAS</h3><a href=\"/off\"><button>DESLIGAR</button></a><a href=\"/\"><button>VOLTAR</button></a></body></html>";
const char off_resp[] = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body><h3>LUZES da Sala: APAGADAS</h3><a href=\"/on\"><button>LIGAR</button></a><a href=\"/\"><button>VOLTAR</button></a></body></html>";
const char telegram_resp[] = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body><object width='0' height='0' type='text/html' data='https://api.telegram.org/bot5775630816:AAEuxojQRdMLpiVQINcnt0_iMWv87YQjsaM/sendMessage?chat_id=-708112312&text=AULA_INICIADA!!!'></object>Mensagem de início de aula enviada para o Telegram!<br><br><a href=\"/\"><button>VOLTAR</button></a></body></html>";
const char aulafim_telegram_resp[] = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body><object width='0' height='0' type='text/html' data='https://api.telegram.org/bot5775630816:AAEuxojQRdMLpiVQINcnt0_iMWv87YQjsaM/sendMessage?chat_id=-708112312&text=AULA_ENCERRADA!!!'></object>Mensagem de aula encerrada enviada para o Telegram!<br><br><a href=\"/\"><button>VOLTAR</button></a></body></html>";

esp_err_t cadastro_form_handler(httpd_req_t *req)
{	
    char cadastro_form[500 + TAG_ID_LEN + 1] = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body><h1>Cadastro de Aluno</h1><h5>&uacute;ltima tag lida: ";
    strcat(cadastro_form, lastReadTag);
    strcat(cadastro_form, " </h5><form action='/cadastrar' method='post' accept-charset=\"utf-8\"><input type='text' id='name' name='name' placeholder='nome do aluno' maxlength='49'><input type='submit' value='Cadastrar'></form><br><br><a href=\"/\"><button>VOLTAR</button></a></body></html>");
	httpd_resp_send(req, cadastro_form, HTTPD_RESP_USE_STRLEN);
    modo = MODE_READ_TAGS_FOR_REGISTER;
    return ESP_OK;
}

esp_err_t cadastrar_aluno_handler(httpd_req_t *req)
{
    ESP_LOGI(TAGM, "/echo handler read content length %d", req->content_len);

    char*  buf = malloc(req->content_len + 1);
    size_t off = 0;
    int    ret;

    if (!buf) {
        ESP_LOGE(TAGM, "Failed to allocate memory of %d bytes!", req->content_len + 1);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    while (off < req->content_len) {
        /* Read data received in the request */
        ret = httpd_req_recv(req, buf + off, req->content_len - off);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            free(buf);
            return ESP_FAIL;
        }
        off += ret;
        ESP_LOGI(TAGM, "/echo handler recv length %d", ret);
    }
    buf[off] = '\0';

    if (req->content_len < 128) {
        ESP_LOGI(TAGM, "/echo handler read %s", buf);
    }

    /* Search for Custom header field */
    char*  req_hdr = 0;
    size_t hdr_len = httpd_req_get_hdr_value_len(req, "Custom");
    if (hdr_len) {
        /* Read Custom header value */
        req_hdr = malloc(hdr_len + 1);
        if (!req_hdr) {
            ESP_LOGE(TAG, "Failed to allocate memory of %d bytes!", hdr_len + 1);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        httpd_req_get_hdr_value_str(req, "Custom", req_hdr, hdr_len + 1);

        /* Set as additional header for response packet */
        httpd_resp_set_hdr(req, "Custom", req_hdr);
    }
    char aluno_name[TAG_NAME_LEN];
    strncpy(aluno_name, &buf[5], TAG_NAME_LEN);    
    for (int i = 0; i< strlen(aluno_name); i++){
        if (aluno_name[i] == '+'){
            aluno_name[i] = ' ';
        }
    }
    printf("\n Cadastrando aluno \n");
    printf("aluno_name = %s \n", aluno_name);
    printf("lastReadTag = %s \n", lastReadTag);

    const char botoes[] = "<br><br><a href=\"/cadastro\"><button>TELA DE CADASTRO</button></a><a href=\"/cadastros\"><button>ALUNOS CADASTRADOS</button></a></body></html>";
    // cadastro vai funcionar
    if (strcmp(aluno_name, "") != 0 && strcmp(lastReadTag, NO_TAG) != 0 && idExits(AlunosCadastrados, lastReadTag) == 0) {
        tagsListAppend(AlunosCadastrados, lastReadTag, aluno_name);

        getString(AlunosCadastrados, string);
        saveFile(ALUNOS_CADASTRADOS, string);

        char response[TAG_ID_LEN + TAG_NAME_LEN  + LEN_BOTOES_CADASTRAR_RESPONSE + 100 + 1] = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body> Aluno ";
        strcat(response, aluno_name); 
        strcat(response, " cadastrado com a tag ");
        strcat(response, lastReadTag);
        strcat(response, botoes);
        httpd_resp_send(req, response, strlen(response));
    }
    // cadastro não vai funcionar
    else {
        if (strcmp(aluno_name, "") == 0) { // não colocou nome
            char response[LEN_BOTOES_CADASTRAR_RESPONSE + 135 + 1] = "N&atilde;o foi poss&iacute;vel realizar o cadastro, o nome do aluno n&atilde;o foi preenchido, clique no bot&atilde;o TELA DE CADASTRO abaixo para tentar novamente";
            strcat(response, botoes);
            httpd_resp_send(req, response, strlen(response));
        }
        else if (strcmp(lastReadTag, NO_TAG) == 0) { // não colocou tag
            char response[LEN_BOTOES_CADASTRAR_RESPONSE + 145 + 1] = "N&atilde;o foi poss&iacute;vel realizar o cadastro, a tag n&atilde;o foi lida, passe uma tag no leitor e clique no bot&atilde;o TELA DE CADASTRO abaixo para tentar novamente";
            strcat(response, botoes);
            httpd_resp_send(req, response, strlen(response));
        }
        else if (idExits(AlunosCadastrados, lastReadTag) == 1) { // a tag já estava cadastrada
            char aluno_da_tag_ja_cadastrada[TAG_NAME_LEN];
            getNameById(AlunosCadastrados, lastReadTag, aluno_da_tag_ja_cadastrada);
            char response[TAG_ID_LEN + TAG_NAME_LEN + LEN_BOTOES_CADASTRAR_RESPONSE + 168 + 1] = "N&atilde;o foi poss&iacute;vel realizar o cadastro, ";
            strcat(response, " a tag ");
            strcat(response, lastReadTag);
            strcat(response, " j&aacute; est&aacute; cadastrada com o aluno ");
            strcat(response, aluno_da_tag_ja_cadastrada);
            strcat(response, ", passe outra tag no leitor e clique no bot&atilde;o TELA DE CADASTRO abaixo para tentar novamente");
            strcat(response, botoes);
            httpd_resp_send(req, response, strlen(response));
        }
        else { // motivo não identificado
            char response[LEN_BOTOES_CADASTRAR_RESPONSE + 124 + 1] = "N&atilde;o foi poss&iacute;vel realizar o cadastro, motivo n&atilde;o identificado, clique no bot&atilde;o TELA DE CADASTRO abaixo para tentar novamente";
            strcat(response, botoes);
            httpd_resp_send(req, response, strlen(response));
        }
    }

    free(req_hdr);
    free(buf);
    return ESP_OK;
}

esp_err_t get_handler(httpd_req_t *req)
{	
	httpd_resp_send(req, menu_resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t lista_handler(httpd_req_t *req)
{	
    char resposta[25 + (9 + ((9 + TAG_NAME_LEN) * MAX_TAGS) + 1) + 43 + 1] = "<h3>Alunos Presentes</h3>";
    char botao[43 + 1] = "<br><a href=\"/\"><button>VOLTAR</button></a>";
    getNamesHtml(AlunosPresentes, resposta);
    strcat(resposta, botao);
    
	httpd_resp_send(req, resposta, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t cadastros(httpd_req_t *req)
{	
    char resposta[9 + ((18 + TAG_NAME_LEN + TAG_ID_LEN) * MAX_TAGS) + 1 + 43 + 1] = "<h3>Alunos Cadastrados</h3>";
    char botao[43 + 1] = "<br><a href=\"/\"><button>VOLTAR</button></a>";
    getTagsHtml(AlunosCadastrados, resposta);
    strcat(resposta, botao);
    
	httpd_resp_send(req, resposta, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t telegram_handler(httpd_req_t *req)
{	
    modo = MODE_READ_TAGS_FOR_PRESENCE; //modo que habilita salvar as tags.
	httpd_resp_send(req, telegram_resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t aulafim_handler(httpd_req_t *req)
{	
    modo = MODE_DONT_READ_TAGS;  // modo que desabilita salvar as tags.
    
    freeTagsList(AlunosPresentes);
    printTagsList(AlunosPresentes);

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

httpd_uri_t uri_cadastro = {
    .uri      = "/cadastro",
    .method   = HTTP_GET,
    .handler  = cadastro_form_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_cadastros = {
    .uri      = "/cadastros",
    .method   = HTTP_GET,
    .handler  = cadastros,
    .user_ctx = NULL
};

httpd_uri_t uri_cadastrar = {
    .uri      = "/cadastrar",
    .method   = HTTP_POST,
    .handler  = cadastrar_aluno_handler,
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
    config.max_uri_handlers = 10;
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
        httpd_register_uri_handler(server, &uri_cadastro);
        httpd_register_uri_handler(server, &uri_cadastrar);
        httpd_register_uri_handler(server, &uri_cadastros);
    }

    return server;
}

void tag_handler(uint8_t* sn) { // o número de série tem sempre 5 bytes

    char tag[TAG_ID_LEN] = "";

    for (int i = 0; i < 5; i++) {
        int num = sn[i];
        char snum[4];
        snprintf(snum, 4, "%d", num);        
        strcat(tag, snum);
    }

    strcpy(lastReadTag, tag);
    printf("\nTag Atual: %s\n", lastReadTag);

    if (modo == MODE_READ_TAGS_FOR_PRESENCE && idExits(AlunosCadastrados, tag) == 1) {
        char name[TAG_NAME_LEN];
        getNameById(AlunosCadastrados, lastReadTag, name);
        tagsListAppend(AlunosPresentes, lastReadTag, name);

        vTaskDelay(10);
        gpio_set_level(RELE_PIN, 1);
        vTaskDelay(100);
        gpio_set_level(RELE_PIN, 0);
    }
    
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

    // tem que deixa isso no final do main se não não roda
    printf("\n\n lendo da memória \n\n");
    readFile(ALUNOS_CADASTRADOS, string);
    AlunosCadastrados = CreateTagsListFromString(string);
    AlunosPresentes = CreateTagsList();
}
