#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "../include/simcom/sim7070_core.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define SIM_UART_PORT UART_NUM_2
#define BUF_SIZE 1024

#define AT_BIT_OK BIT0
#define AT_BIT_ERROR BIT1
#define AT_BIT_PROMPT BIT2

#define MAX_CONSECUTIVE_FAILURES 3

static SemaphoreHandle_t at_mutex = NULL;
static QueueHandle_t uart_queue;
static EventGroupHandle_t at_event_group;
static char fsm_rx_buffer[BUF_SIZE];
static volatile int fsm_rx_idx = 0;
static const char* TAG = "SIM7070_GPRS";
static gpio_num_t stored_rx_pin, stored_tx_pin, stored_pwr_pin;
static int consecutive_failures = 0;

static void sim7070_uart_event_task(void* pvParameters);

int send_at_cmd(const char* cmd, char* rx_buf, int rx_buf_len,
                uint32_t timeout_ms) {
    if (at_mutex != NULL) xSemaphoreTake(at_mutex, portMAX_DELAY);

    xEventGroupClearBits(at_event_group,
                         AT_BIT_OK | AT_BIT_ERROR | AT_BIT_PROMPT);
    fsm_rx_idx = 0;
    memset(fsm_rx_buffer, 0, BUF_SIZE);
    uart_flush_input(SIM_UART_PORT);
    uart_write_bytes(SIM_UART_PORT, cmd, strlen(cmd));

    EventBits_t bits =
        xEventGroupWaitBits(at_event_group, AT_BIT_OK | AT_BIT_ERROR, pdFALSE,
                            pdFALSE, pdMS_TO_TICKS(timeout_ms));

    int len = strlen(fsm_rx_buffer);
    if (rx_buf != NULL && len > 0) {
        strncpy(rx_buf, fsm_rx_buffer, rx_buf_len - 1);
        rx_buf[rx_buf_len - 1] = '\0';
    }

    if (bits & AT_BIT_OK) {
        if (at_mutex != NULL) xSemaphoreGive(at_mutex);
        return len;
    } else if (bits & AT_BIT_ERROR) {
        ESP_LOGE(TAG, "Błąd z modemu (ERROR) dla: %s", cmd);
        if (at_mutex != NULL) xSemaphoreGive(at_mutex);
        return -1;
    } else {
        ESP_LOGE(TAG, "Timeout komendy: %s", cmd);
        if (at_mutex != NULL) xSemaphoreGive(at_mutex);
        return 0;
    }
}

bool sim7070_init(gpio_num_t rx_pin, gpio_num_t tx_pin, gpio_num_t pwr_pin) {
    stored_rx_pin = rx_pin;
    stored_tx_pin = tx_pin;
    stored_pwr_pin = pwr_pin;

    static bool hw_initialized = false;
    if (!hw_initialized) {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << pwr_pin);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&io_conf);
        gpio_set_level(pwr_pin, 0);

        uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .source_clk = UART_SCLK_APB,
        };

        ESP_ERROR_CHECK(uart_param_config(SIM_UART_PORT, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(SIM_UART_PORT, (int)tx_pin, (int)rx_pin,
                                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(SIM_UART_PORT, BUF_SIZE * 2,
                                            BUF_SIZE * 2, 20, &uart_queue, 0));
        at_event_group = xEventGroupCreate();
        at_mutex = xSemaphoreCreateMutex();
        xTaskCreate(sim7070_uart_event_task, "sim_rx_task", 4096, NULL, 12,
                    NULL);

        hw_initialized = true;
    }

    char rx_buf[BUF_SIZE];
    bool is_alive = false;
    const int max_retries = 3;

    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        ESP_LOGI(TAG, "Próba sprzętowego uruchomienia modemu %d/%d...", attempt,
                 max_retries);

        gpio_set_level(pwr_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(1500));
        gpio_set_level(pwr_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(4000));

        for (int i = 0; i < 10; i++) {
            uart_flush_input(SIM_UART_PORT);
            if (send_at_cmd("AT\r\n", rx_buf, sizeof(rx_buf), 500) > 0 &&
                strstr(rx_buf, "OK")) {
                is_alive = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (is_alive) {
            ESP_LOGI(TAG, "Synchronizacja baudrate ustatkowana.");
            break;
        }
    }

    if (!is_alive) {
        ESP_LOGE(TAG, "FATAL ERROR: Baseband nie odpowiada.");
        return false;
    }

    send_at_cmd("ATE0\r\n", rx_buf, sizeof(rx_buf), 500);
    send_at_cmd("AT+CFUN=0\r\n", rx_buf, sizeof(rx_buf), 5000);

    send_at_cmd("AT+CNMP=13\r\n", rx_buf, sizeof(rx_buf), 2000);

    send_at_cmd("AT+CNCFG=0,1,\"internet\"\r\n", rx_buf, sizeof(rx_buf), 2000);

    send_at_cmd("AT+CFUN=1\r\n", rx_buf, sizeof(rx_buf), 10000);

    ESP_LOGI(TAG, "Rozruch RF...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    send_at_cmd("AT+CREG=2\r\n", rx_buf, sizeof(rx_buf), 500);
    send_at_cmd("AT+CGREG=2\r\n", rx_buf, sizeof(rx_buf),
                500);  // GPRS network registration status

    return true;
}

bool sim7070_wait_for_network(void) {
    char rx_buf[BUF_SIZE];
    ESP_LOGI(TAG, "Oczekiwanie na rejestrację w sieci (max 60s)...");

    for (int i = 0; i < 60; i++) {
        bool creg_ok = false;
        bool cgreg_ok = false;

        if (send_at_cmd("AT+CREG?\r\n", rx_buf, sizeof(rx_buf), 1000) > 0) {
            char* ptr = rx_buf;
            while (*ptr) {
                if (*ptr == '\r' || *ptr == '\n') *ptr = ' ';
                ptr++;
            }
            ESP_LOGI(TAG, "Status CREG [%d/60]: %s", i + 1, rx_buf);
            if (strstr(rx_buf, ",1") || strstr(rx_buf, ",5")) {
                creg_ok = true;
            }
        }

        if (creg_ok &&
            send_at_cmd("AT+CGREG?\r\n", rx_buf, sizeof(rx_buf), 1000) > 0) {
            char* ptr = rx_buf;
            while (*ptr) {
                if (*ptr == '\r' || *ptr == '\n') *ptr = ' ';
                ptr++;
            }
            ESP_LOGI(TAG, "Status CGREG [%d/60]: %s", i + 1, rx_buf);
            if (strstr(rx_buf, ",1") || strstr(rx_buf, ",5")) {
                cgreg_ok = true;
            }
        }

        if (creg_ok && cgreg_ok) {
            ESP_LOGI(TAG,
                     "SUKCES! Zarejestrowano w sieci GSM i przyłączono GPRS.");
            return true;
        }

        if (i % 10 == 0 && i > 0) {
            send_at_cmd("AT+CPSI?\r\n", rx_buf, sizeof(rx_buf), 1000);
            ESP_LOGI(TAG, "Parametry stacji (CPSI): %s", rx_buf);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGE(TAG, "Przekroczono czas. Sieć odrzuciła kartę.");
    consecutive_failures++;
    if (consecutive_failures >= MAX_CONSECUTIVE_FAILURES) {
        sim7070_recover();
    }
    return false;
}

bool sim7070_mqtt_connect(void) {
    char rx_buf[BUF_SIZE];

    send_at_cmd("AT+SMDISC\r\n", rx_buf, sizeof(rx_buf), 500);
    send_at_cmd("AT+CNACT=0,0\r\n", rx_buf, sizeof(rx_buf), 2000);
    vTaskDelay(pdMS_TO_TICKS(1500));
    send_at_cmd("AT+CNCFG=0,1,\"internet\"\r\n", rx_buf, sizeof(rx_buf), 2000);

    if (send_at_cmd("AT+CNACT=0,1\r\n", rx_buf, sizeof(rx_buf), 10000) <= 0 ||
        !strstr(rx_buf, "OK")) {
        ESP_LOGE(TAG, "Blad aktywacji kontekstu PDP.");
        consecutive_failures++;
        if (consecutive_failures >= MAX_CONSECUTIVE_FAILURES) {
            sim7070_recover();
        }
        return false;
    }

    ESP_LOGI(TAG, "Oczekiwanie na adres IP z sieci...");
    bool pdp_active = false;
    for (int i = 0; i < 10; i++) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        send_at_cmd("AT+CNACT?\r\n", rx_buf, sizeof(rx_buf), 2000);

        if (strstr(rx_buf, "+CNACT: 0,1") || strstr(rx_buf, "ACTIVE")) {
            pdp_active = true;
            ESP_LOGI(TAG, "Adres IP przyznany pomyślnie!");
            break;
        }
    }

    if (!pdp_active) {
        ESP_LOGE(TAG,
                 "Timeout: Siec nie przydzielila adresu IP. Zamykanie sesji.");
        send_at_cmd("AT+CNACT=0,0\r\n", rx_buf, sizeof(rx_buf), 2000);
        consecutive_failures++;
        if (consecutive_failures >= MAX_CONSECUTIVE_FAILURES) {
            sim7070_recover();
        }
        return false;
    }

    send_at_cmd("AT+SMCONF=\"URL\",\"broker.hivemq.com\",1883\r\n", rx_buf,
                sizeof(rx_buf), 1000);
    send_at_cmd("AT+SMCONF=\"KEEPTIME\",60\r\n", rx_buf, sizeof(rx_buf), 500);
    send_at_cmd("AT+SMCONF=\"CLEANSS\",1\r\n", rx_buf, sizeof(rx_buf), 500);
    send_at_cmd("AT+SMCONF=\"CLIENTID\",\"esp32_p_komp\"\r\n", rx_buf,
                sizeof(rx_buf), 1000);

    ESP_LOGI(TAG, "Zestawianie tunelu TCP (MQTT)...");
    if (send_at_cmd("AT+SMCONN\r\n", rx_buf, sizeof(rx_buf), 20000) > 0 &&
        strstr(rx_buf, "OK")) {
        ESP_LOGI(TAG, "SUKCES! Połączono z brokerem.");
        consecutive_failures = 0;
        return true;
    }

    ESP_LOGE(TAG, "Blad polaczenia z brokerem MQTT.");
    consecutive_failures++;
    if (consecutive_failures >= MAX_CONSECUTIVE_FAILURES) {
        sim7070_recover();
    }
    return false;
}

void sim7070_mqtt_disconnect(void) {
    char rx_buf[128];
    send_at_cmd("AT+SMDISC\r\n", rx_buf, sizeof(rx_buf), 2000);
    send_at_cmd("AT+CNACT=0,0\r\n", rx_buf, sizeof(rx_buf), 2000);
}

bool sim7070_mqtt_send(const char* topic, const char* payload) {
    if (at_mutex != NULL) xSemaphoreTake(at_mutex, portMAX_DELAY);

    char cmd[300];
    snprintf(cmd, sizeof(cmd), "AT+SMPUB=\"%s\",%d,1,0\r\n", topic,
             (int)strlen(payload));

    xEventGroupClearBits(at_event_group,
                         AT_BIT_OK | AT_BIT_ERROR | AT_BIT_PROMPT);
    uart_write_bytes(SIM_UART_PORT, cmd, strlen(cmd));

    EventBits_t bits =
        xEventGroupWaitBits(at_event_group, AT_BIT_PROMPT | AT_BIT_ERROR,
                            pdFALSE, pdFALSE, pdMS_TO_TICKS(5000));

    if (bits & AT_BIT_PROMPT) {
        vTaskDelay(pdMS_TO_TICKS(100));
        uart_write_bytes(SIM_UART_PORT, payload, strlen(payload));

        bits = xEventGroupWaitBits(
            at_event_group, AT_BIT_OK | AT_BIT_ERROR, pdFALSE, pdFALSE,
            pdMS_TO_TICKS(15000));  // GPRS może mieć wyższe opóźnienia

        if (bits & AT_BIT_OK) {
            if (at_mutex != NULL) xSemaphoreGive(at_mutex);
            return true;
        }
    }
    if (at_mutex != NULL) xSemaphoreGive(at_mutex);
    return false;
}

cell_info_t sim7070_get_network_params(void) {
    cell_info_t info = {0, 0, 0, 0, false};
    char rx_buf[BUF_SIZE];

    send_at_cmd("AT+CSQ\r\n", rx_buf, sizeof(rx_buf), 1000);

    if (send_at_cmd("AT+CPSI?\r\n", rx_buf, sizeof(rx_buf), 2000) > 0) {
        char* cpsi_ptr = strstr(rx_buf, "+CPSI:");

        if (cpsi_ptr != NULL) {
            char sys_mode[16] = {0};
            char mcc_mnc[16] = {0};
            unsigned long tmp_lac = 0;
            unsigned long tmp_cid = 0;
            unsigned int tmp_mcc = 0;
            unsigned int tmp_mnc = 0;

            if (sscanf(cpsi_ptr, "+CPSI: %15[^,],%*[^,],%15[^,],%lx,%lu",
                       sys_mode, mcc_mnc, &tmp_lac, &tmp_cid) == 4) {
                ESP_LOGI("SIM7070_CELL",
                         "Modem raportuje technologie: [%s], Operator: [%s]",
                         sys_mode, mcc_mnc);

                if (strstr(sys_mode, "NO SERVICE") == NULL) {
                    if (sscanf(mcc_mnc, "%u-%u", &tmp_mcc, &tmp_mnc) == 2) {
                        info.mcc = (uint16_t)tmp_mcc;
                        info.mnc = (uint16_t)tmp_mnc;
                        info.tac = (uint32_t)tmp_lac;
                        info.cid = (uint32_t)tmp_cid;
                        info.is_valid = true;

                        ESP_LOGI("SIM7070_CELL",
                                 "SUKCES! Przechwycono dane: MCC=%d, MNC=%d, "
                                 "TAC=%X, CID=%lu",
                                 info.mcc, info.mnc, info.tac, info.cid);
                    } else {
                        ESP_LOGE("SIM7070_CELL",
                                 "Blad rozdzielenia MCC i MNC: %s", mcc_mnc);
                    }
                } else {
                    ESP_LOGW("SIM7070_CELL",
                             "Modem jest poza zasiegiem sieci (NO SERVICE).");
                }
            } else {
                ESP_LOGE("SIM7070_CELL",
                         "Nie udalo sie sparsowac odpowiedzi CPSI!");
            }
        }
    }
    return info;
}

static void sim7070_uart_event_task(void* pvParameters) {
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*)malloc(BUF_SIZE);

    while (1) {
        if (xQueueReceive(uart_queue, (void*)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA: {
                    if (fsm_rx_idx + event.size >= BUF_SIZE) {
                        ESP_LOGE(TAG, "FSM: Ring Buffer Overflow!");
                        fsm_rx_idx = 0;
                    }

                    int len = uart_read_bytes(SIM_UART_PORT, dtmp, event.size,
                                              portMAX_DELAY);
                    if (len > 0) {
                        memcpy(&fsm_rx_buffer[fsm_rx_idx], dtmp, len);
                        fsm_rx_idx += len;
                        fsm_rx_buffer[fsm_rx_idx] = '\0';
                        if (strstr(fsm_rx_buffer, "OK\r\n")) {
                            xEventGroupSetBits(at_event_group, AT_BIT_OK);
                            fsm_rx_idx = 0;
                        } else if (strstr(fsm_rx_buffer, "ERROR\r\n")) {
                            xEventGroupSetBits(at_event_group, AT_BIT_ERROR);
                            fsm_rx_idx = 0;
                        } else if (strchr(fsm_rx_buffer, '>')) {
                            xEventGroupSetBits(at_event_group, AT_BIT_PROMPT);
                            fsm_rx_idx = 0;
                        } else if (strstr(fsm_rx_buffer, "+CGREG: 0") ||
                                   strstr(fsm_rx_buffer, "+CREG: 0")) {
                            ESP_LOGW(TAG,
                                     "URC: Awaria łącza radiowego (odrzut z "
                                     "MSC/SGSN)!");
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

static void sim7070_recover(void) {
    ESP_LOGW(TAG,
             "%d kolejnych bledow polaczenia - pelny restart modemu (jak "
             "reczne odciecie)!",
             MAX_CONSECUTIVE_FAILURES);
    sim7070_init(stored_rx_pin, stored_tx_pin, stored_pwr_pin);
    consecutive_failures = 0;
}
