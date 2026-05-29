#include "sim7000_core.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include "freertos/event_groups.h"


#define SIM_UART_PORT UART_NUM_2
#define BUF_SIZE 1024

#define AT_BIT_OK BIT0
#define AT_BIT_ERROR BIT1
#define AT_BIT_PROMPT BIT2

static QueueHandle_t uart_queue;
static EventGroupHandle_t at_event_group;
static char fsm_rx_buffer[BUF_SIZE];
static volatile int fsm_rx_idx = 0;
static const char *TAG = "SIM7000";

int send_at_cmd(const char* cmd, char* rx_buf, int rx_buf_len, uint32_t timeout_ms) {
    xEventGroupClearBits(at_event_group, AT_BIT_OK | AT_BIT_ERROR | AT_BIT_PROMPT);
    fsm_rx_idx = 0;
    memset(fsm_rx_buffer, 0, BUF_SIZE);
    uart_flush_input(SIM_UART_PORT);
    uart_write_bytes(SIM_UART_PORT, cmd, strlen(cmd));
    
    EventBits_t bits = xEventGroupWaitBits(at_event_group, 
                                           AT_BIT_OK | AT_BIT_ERROR, 
                                           pdFALSE, pdFALSE, 
                                           pdMS_TO_TICKS(timeout_ms));

    int len = strlen(fsm_rx_buffer);
    if (rx_buf != NULL && len > 0) {
        strncpy(rx_buf, fsm_rx_buffer, rx_buf_len - 1);
        rx_buf[rx_buf_len - 1] = '\0';
    }

    if (bits & AT_BIT_OK) {
        return len;
    } else if (bits & AT_BIT_ERROR) {
        ESP_LOGE(TAG, "Błąd z modemu (ERROR) dla: %s", cmd);
        return -1;
    } else {
        ESP_LOGE(TAG, "Timeout komendy: %s", cmd);
        return 0;
    }
}

bool sim7000_init(gpio_num_t rx_pin, gpio_num_t tx_pin, gpio_num_t pwr_pin) {

    static bool hw_initialized = false;
    if (!hw_initialized) {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1ULL << pwr_pin);
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);
        gpio_set_level(pwr_pin, 0);

        uart_config_t uart_config = {
            .baud_rate = 19200,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .source_clk = UART_SCLK_APB,
            //.flags = {}
        };

        ESP_ERROR_CHECK(uart_param_config(SIM_UART_PORT, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(SIM_UART_PORT, (int)tx_pin, (int)rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(SIM_UART_PORT, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0));
        at_event_group = xEventGroupCreate();
        xTaskCreate(sim7000_uart_event_task, "sim_rx_task", 4096, NULL, 12, NULL);
        
        hw_initialized = true;
    }

       char rx_buf[BUF_SIZE];
    bool is_alive = false;
    const int max_retries = 3;

        for (int attempt = 1; attempt <= max_retries; ++attempt) {
        ESP_LOGI(TAG, "Próba sprzętowego uruchomienia modemu %d/%d...", attempt, max_retries);
            
            gpio_set_level(pwr_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
            gpio_set_level(pwr_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(3000));
            
            for (int i = 0; i < 5; i++) {
                uart_flush_input(SIM_UART_PORT); 
            
                if (send_at_cmd("AT\r\n", rx_buf, sizeof(rx_buf), 500) > 0 && strstr(rx_buf, "OK")) {
                    is_alive = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            if (is_alive) {
            ESP_LOGI(TAG, "Modem odpowiada (AT -> OK). Synchronizacja udana.");
            break; 
            } else {
            ESP_LOGE(TAG, "Brak odpowiedzi na port UART. Ponawiam Power Sequence...");
        }
    }

    if (!is_alive) {
        ESP_LOGE(TAG, "FATAL ERROR: Baseband nie odpowiada po %d cyklach power-on.", max_retries);
        return false; 
    }

    send_at_cmd("ATE0\r\n", rx_buf, sizeof(rx_buf), 500);

    send_at_cmd("AT+CFUN=0\r\n", rx_buf, sizeof(rx_buf), 5000);

    send_at_cmd("AT+CNMP=38\r\n", rx_buf, sizeof(rx_buf), 2000);

    send_at_cmd("AT+CMNB=1\r\n", rx_buf, sizeof(rx_buf), 2000);

    send_at_cmd("AT+CGDCONT=1,\"IP\",\"iot\"\r\n", rx_buf, sizeof(rx_buf), 2000);

    send_at_cmd("AT+CFUN=1\r\n", rx_buf, sizeof(rx_buf), 10000);
    
    ESP_LOGI(TAG, "Oczekiwanie na inicjalizację karty SIM...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    send_at_cmd("AT+CEREG=2\r\n", rx_buf, sizeof(rx_buf), 500);
    send_at_cmd("AT+CREG=2\r\n", rx_buf, sizeof(rx_buf), 500);

    return true;
}

bool sim7000_wait_for_network(void) {
    char rx_buf[BUF_SIZE];
    for (int i = 0; i < 150; i++) { // Zwiększony timeout do 45s (NB-IoT/LTE-M bywa wolne)
        send_at_cmd("AT+CEREG?\r\n", rx_buf, sizeof(rx_buf), 1000);
        if (strstr(rx_buf, ",1") || strstr(rx_buf, ",5")) {
            ESP_LOGI(TAG, "Zarejestrowano w sieci!");
            return true;
        }
        if(i==31){
            send_at_cmd("AT+CPSI?\r\n", rx_buf, sizeof(rx_buf), 1000);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    return false;
}  

bool sim7000_mqtt_connect(void) {
    char rx_buf[BUF_SIZE];
    
    send_at_cmd("AT+SMDISC\r\n", rx_buf, sizeof(rx_buf), 500);
    send_at_cmd("AT+CNACT=0\r\n", rx_buf, sizeof(rx_buf), 500);

    send_at_cmd("AT+SMCONF=\"URL\",\"igel-kamil.ddns.net\",1883\r\n", rx_buf, sizeof(rx_buf), 1000);
    send_at_cmd("AT+SMCONF=\"KEEPTIME\",60\r\n", rx_buf, sizeof(rx_buf), 500);
    send_at_cmd("AT+SMCONF=\"CLEANSS\",1\r\n", rx_buf, sizeof(rx_buf), 500);
    send_at_cmd("AT+SMCONF=\"CLIENTID\",\"esp32_p_komp\"\r\n", rx_buf, sizeof(rx_buf), 1000);

    send_at_cmd("AT+CNACT=1,\"iot\"\r\n", rx_buf, sizeof(rx_buf), 10000);
    
    send_at_cmd("AT+CNACT?\r\n", rx_buf, sizeof(rx_buf), 1000);
    send_at_cmd("AT+CDNSGIP=\"igel-kamil.ddns.net\"\r\n", rx_buf, sizeof(rx_buf), 1000);

    ESP_LOGI(TAG, "Próba połączenia z domowym serwerem...");
    if (send_at_cmd("AT+SMCONN\r\n", rx_buf, sizeof(rx_buf), 20000) > 0 && strstr(rx_buf, "OK")) {
        ESP_LOGI(TAG, "SUKCES! Połączono z Mosquitto.");
        return true;
    }
    return false;
}

bool sim7000_mqtt_send(const char *topic, const char *payload) {
    char cmd[300];
    snprintf(cmd, sizeof(cmd), "AT+SMPUB=\"%s\",%d,1,0\r\n", topic, (int)strlen(payload));
    
    xEventGroupClearBits(at_event_group, AT_BIT_OK | AT_BIT_ERROR | AT_BIT_PROMPT);
    uart_write_bytes(SIM_UART_PORT, cmd, strlen(cmd));
    
    EventBits_t bits = xEventGroupWaitBits(at_event_group, 
                                           AT_BIT_PROMPT | AT_BIT_ERROR, 
                                           pdFALSE, pdFALSE, 
                                           pdMS_TO_TICKS(5000));
                                           
    if (bits & AT_BIT_PROMPT) {
        vTaskDelay(pdMS_TO_TICKS(100)); 
        uart_write_bytes(SIM_UART_PORT, payload, strlen(payload));
        
        bits = xEventGroupWaitBits(at_event_group, 
                                   AT_BIT_OK | AT_BIT_ERROR, 
                                   pdFALSE, pdFALSE, 
                                   pdMS_TO_TICKS(10000));
                                   
        if (bits & AT_BIT_OK) {
            vTaskDelay(pdMS_TO_TICKS(1000)); 
            return true;
        }
    }
    return false;
}

cell_info_t sim7000_get_network_params(void) {
    cell_info_t info = {0, 0, 0, 0, false};
    char rx_buf[BUF_SIZE];
    
    // 1. Sprawdź sygnał dla celów diagnostycznych
    send_at_cmd("AT+CSQ\r\n", rx_buf, sizeof(rx_buf), 1000);
    
    // 2. Odpytanie o parametry komórki
    if (send_at_cmd("AT+CPSI?\r\n", rx_buf, sizeof(rx_buf), 2000) > 0) {
        char *cpsi_ptr = strstr(rx_buf, "+CPSI:");
        if (cpsi_ptr != NULL) {
            char sys_mode[16];
            char mcc_mnc[16];
            
            // Parsowanie zgodnie ze strukturą SIM7000
            if (sscanf(cpsi_ptr, "+CPSI: %15[^,],%*[^,],%15[^,],%lx,%lu", 
                       sys_mode, mcc_mnc, &info.tac, &info.cid) >= 4) {
                
                sscanf(mcc_mnc, "%hu-%hu", &info.mcc, &info.mnc);
                info.is_valid = true;
            }
        }
    }
    return info;
}

static void sim7000_uart_event_task(void *pvParameters) {
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(BUF_SIZE);

    while (1) {
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                
                case UART_DATA: {
                    if (fsm_rx_idx + event.size >= BUF_SIZE) {
                        ESP_LOGE(TAG, "FSM: Ring Buffer Overflow! Czyszczenie agregatora.");
                        fsm_rx_idx = 0; 
                    }

                    int len = uart_read_bytes(SIM_UART_PORT, dtmp, event.size, portMAX_DELAY);
                    if (len > 0) {
                        memcpy(&fsm_rx_buffer[fsm_rx_idx], dtmp, len);
                        fsm_rx_idx += len;
                        fsm_rx_buffer[fsm_rx_idx] = '\0';                   
                        if (strstr(fsm_rx_buffer, "OK\r\n")) {
                            xEventGroupSetBits(at_event_group, AT_BIT_OK);
                            fsm_rx_idx = 0;
                        }
                        else if (strstr(fsm_rx_buffer, "ERROR\r\n")) {
                            xEventGroupSetBits(at_event_group, AT_BIT_ERROR);
                            fsm_rx_idx = 0;
                        }
                        else if (strchr(fsm_rx_buffer, '>')) {
                            xEventGroupSetBits(at_event_group, AT_BIT_PROMPT);
                            fsm_rx_idx = 0;
                        }
                        
                        else if (strstr(fsm_rx_buffer, "+CEREG: 0") || strstr(fsm_rx_buffer, "+CEREG: 4")) {
                            ESP_LOGW(TAG, "URC: Awaria łącza radiowego (Baseband odrzucony)!");
                            fsm_rx_idx = 0; 
                        }
                    }
                    break;
                }

                case UART_FIFO_OVF:
                    ESP_LOGE(TAG, "Hardware FIFO Overflow!");
                    uart_flush_input(SIM_UART_PORT);
                    xQueueReset(uart_queue);
                    fsm_rx_idx = 0;
                    break;

                case UART_BUFFER_FULL:
                    ESP_LOGE(TAG, "Ring Buffer Full!");
                    uart_flush_input(SIM_UART_PORT);
                    xQueueReset(uart_queue);
                    fsm_rx_idx = 0;
                    break;
                    
                default:
                    break;
            }
        }
    }
    free(dtmp);
    vTaskDelete(NULL);
}