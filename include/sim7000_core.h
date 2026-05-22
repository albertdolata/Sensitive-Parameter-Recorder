/**
 * @file sim7000_core.h
 * @brief Driver niskopoziomowy dla modułu SIM7000 (LTE-M/NB-IoT) dla ESP32
 * @details Zapewnia maszynę stanów do obsługi komend AT przez UART,
 * zarządzanie zasilaniem oraz parsowanie parametrów sieciowych.
 * @todo Dodać funkcje przechodzenia w tryb oszczędzania energii
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h" 

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Dane identyfikacyjne stacji bazowej (Cell ID) uzyskane z komendy AT+CPSI.
 */
typedef struct {
    uint16_t mcc;     /**< Mobile Country Code. */
    uint16_t mnc;     /**< Mobile Network Code. */
    uint32_t tac;     /**< Tracking Area Code (16 lub 24 bity). */
    uint32_t cid;     /**< E-UTRAN Cell Identifier (28 bitów). */
    bool is_valid;    /**< True, jeśli parametry zostały poprawnie sparsowane. */
} cell_info_t;



/**
 * @brief Inicjalizuje interfejs UART i przeprowadza sekwencję Power-On.
 * * @details Konfiguruje UART2 (115200 8N1), tworzy kolejkę zdarzeń oraz task @ref sim7000_uart_event_task.
 * Wykonuje twardy reset modułu za pomocą pinu PWRKEY (2s impuls).
 * * @param[in] rx_pin Pin RX ESP32 (połączony z TX modemu).
 * @param[in] tx_pin Pin TX ESP32 (połączony z RX modemu).
 * @param[in] pwr_pin Pin sterujący kluczem tranzystorowym PWRKEY.
 * * @return true jeśli modem odpowiedział poprawnie na AT i przeszedł konfigurację (ATE0, CFUN, APN).
 * @note Funkcja blokująca (blocking) - całkowity timeout inicjalizacji może wynieść do 30s.
 */
bool sim7000_init(gpio_num_t rx_pin, gpio_num_t tx_pin, gpio_num_t pwr_pin);

/**
 * @brief Pobiera parametry identyfikacjne ze stacji bazwoej do której aktualnie zalogowany jest moduł.
 * * @details Funkcja wysyła komendę AT+CPSI?, która zwraca inforamcje o trybie pracy (np. LTE-M, NB-IOT).
 * Dane są parsowane ze stringa do struktury @ref cell_info_t.
 * * @return cell_info_t Struktura z danymi komórki.
 * Jeśli modem nie jest zalgowany, pole is_valid zostanie ustawione na false.
 * * @note Wymaga aktywnego Basebandu (modem nie może być w trybie Flight Mode - CFUN=0/4).
 * @see sim7000_wait_for_network
 */
cell_info_t sim7000_get_network_params(void);

/**
 * @brief Oczekuje na rejestracje w sieci LTE-M/NB-IoT.
 * & @details Cyklicznie (co 1s) sprawdza status za pomocą AT+CEREG?.
 * Maksymalny czas oczekiwania to ok. 150 sekund.
 * * @return true jeśli status to 1 (registered, home network) lub 5 (registered, roaming)
 */
bool sim7000_wait_for_network(void);

/**
 * @brief Wysyła wiadomość MQTT na określony temat.
 * * @details Proces dwuetapowy:
 * 1. Wysyłanie AT+SMPUB i oczekiwanie na znak zachęty '>'.
 * 2. Przesłanie surowgo payloadu i oczekiwanie na OK.
 * * @param[in] topic Nazwa tematu (wykorzystujemy "dom/czujnik1").
 * @param[in] payload Treść wiadomości (string JSON).
 * * @return true jeśli wysyłka została potwierdzona przez broker.
 * @pre Wymaga aktywnego połączenia przez @ref sim7000_mqtt_connect.
 */
bool sim7000_mqtt_send(const char *topic, const char *payload);


/**
 * @brief Konfiguruje parametry stosu MQTT i nawiązuje połączenie z brokerem.
 * * @details Ustawia adres URL (w naszym przypadku igel-kamil.ddns.net:1883) oraz APN ("iot").
 * Aktywuje kontekst PDP komendą AT+CNACT.
 * @return true jeśli uzyskano IP oraz pomyślnie wykonano AT+SMCONN.
 */
bool sim7000_mqtt_connect(void);


/**
 * @brief Wwnętrzny task obsługujący strumień danych UART.
 * * @details Przetwarza odpowiedzi OK/ERROR oraz URC
 * i ustawia bity w Event Group @p at_event_group.
 * * @param[in] pvParameters parametr FreeRTOS (NULL).
 */
static void sim7000_uart_event_task(void *pvParameters);

/** @brief Wysyła komendę AT do modemu.
 * @param[in] cmd Komenda AT do wysłania.
 * @param[out] rx_buf Bufor na odpowiedź z modemu.
 * @param[in] rx_buf_len Długość bufora na odpowiedź.
 * @param[in] timeout_ms Czas oczekiwania na odpowiedź.
 * @return Liczba bajtów odebranych, lub -1 w przypadku błędu.
 */
int send_at_cmd(const char* cmd, char* rx_buf, int rx_buf_len, uint32_t timeout_ms);

#ifdef __cplusplus


}
#endif
