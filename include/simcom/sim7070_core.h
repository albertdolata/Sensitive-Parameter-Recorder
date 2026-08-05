/**
 * @file sim7070_core.h
 * @brief Driver niskopoziomowy dla modułu SIM7070 (GPRS/MQTT) dla ESP32.
 * @details Zapewnia maszynę stanów do obsługi komend AT przez sprzętowy UART,
 * zarządzanie zasilaniem (Power-On), autoryzację w sieci operatora oraz
 * zestawianie połączenia TCP/IP i tunelu MQTT.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dane identyfikacyjne stacji bazowej (Cell ID) uzyskane z komendy
 * AT+CPSI.
 */
typedef struct {
    uint16_t mcc;  /**< Mobile Country Code (Kod kraju). */
    uint16_t mnc;  /**< Mobile Network Code (Kod operatora). */
    uint32_t tac;  /**< Tracking Area Code (Obszar śledzenia). */
    uint32_t cid;  /**< Cell Identifier (Unikalny numer komórki). */
    bool is_valid; /**< Flaga określająca, czy parametry zostały poprawnie
                      sparsowane. */
} cell_info_t;

/**
 * @brief Inicjalizuje interfejs UART i przeprowadza sekwencję Power-On.
 * * @details Konfiguruje UART2 (115200 8N1), tworzy kolejkę zdarzeń oraz
 * zadanie nasłuchujące w tle. Wykonuje sprzętowy reset modemu za pomocą pinu
 * PWR, po czym konfiguruje podstawowe parametry (ATE0, CFUN, APN).
 * * @param[in] rx_pin Pin RX ESP32 (połączony z TX modemu).
 * @param[in] tx_pin Pin TX ESP32 (połączony z RX modemu).
 * @param[in] pwr_pin Pin sterujący kluczem tranzystorowym zasilania modemu.
 * * @return true Jeśli modem odpowiedział poprawnie i zsynchronizował baudrate.
 * @return false W przypadku całkowitego braku odpowiedzi modemu (FATAL ERROR).
 */
bool sim7070_init(gpio_num_t rx_pin, gpio_num_t tx_pin, gpio_num_t pwr_pin);

/**
 * @brief Pobiera parametry identyfikacyjne stacji bazowej, do której zalogowany
 * jest moduł.
 * * @details Funkcja wysyła komendę AT+CPSI?, która zwraca informacje o trybie
 * pracy (np. GSM). Dane są parsowane ze stringa do struktury @ref cell_info_t.
 * * @return cell_info_t Struktura z danymi komórki. Jeśli modem nie jest
 * zalogowany, pole is_valid zostanie ustawione na false.
 */
cell_info_t sim7070_get_network_params(void);

/**
 * @brief Oczekuje na autoryzację modemu w sieci komórkowej (CS oraz PS).
 * * @details Cyklicznie sprawdza status za pomocą AT+CREG? oraz AT+CGREG?.
 * Maksymalny czas oczekiwania to 60 sekund. W przypadku błędu podbija
 * globalny licznik awarii.
 * * @return true Jeśli sieć przydzieliła dostęp do usług głosowych i
 * pakietowych (GPRS).
 * @return false Jeśli upłynął limit czasu oczekiwania.
 */
bool sim7070_wait_for_network(void);

/**
 * @brief Wysyła wiadomość MQTT na określony temat.
 * * @details Proces dwuetapowy:
 * 1. Wysłanie komendy AT+SMPUB i oczekiwanie na znak zachęty '>'.
 * 2. Przesłanie surowego payloadu i oczekiwanie na potwierdzenie OK.
 * * @param[in] topic Nazwa tematu (np. "dom/czujnik1").
 * @param[in] payload Treść wiadomości (ciąg znaków w formacie JSON).
 * * @return true Jeśli wysyłka została potwierdzona przez broker (ACK).
 * @return false Jeśli wystąpił błąd lub przekroczono czas oczekiwania.
 * * @pre Wymaga aktywnego połączenia ustanowionego przez @ref sim7070_mqtt_connect.
 */
bool sim7070_mqtt_send(const char* topic, const char* payload);

/**
 * @brief Konfiguruje parametry stosu TCP/IP i nawiązuje połączenie z brokerem MQTT.
 * * @details Ustawia adres URL brokera, aktywuje kontekst PDP (pobranie adresu IP) 
 * oraz zestawia sesję kliencką. Każdy krytyczny błąd w tej funkcji inkrementuje 
 * licznik awarii, co może wywołać sprzętowy restart modemu. Sukces zeruje licznik.
 * * @return true Jeśli pomyślnie uzyskano IP i połączono się z brokerem.
 * @return false Jeśli sieć nie przydzieliła IP lub broker odrzucił połączenie.
 */
bool sim7070_mqtt_connect(void);

/** * @brief Wysyła niskopoziomową komendę AT do modemu i czeka na odpowiedź.
 * * @details Funkcja jest bezpieczna wątkowo (korzysta z Mutexa). Blokuje zadanie 
 * aż do otrzymania statusu OK, ERROR, znaku zachęty lub do wystąpienia timeoutu.
 * * @param[in] cmd Komenda AT do wysłania (zakończona \r\n).
 * @param[out] rx_buf Bufor, do którego zostanie skopiowana odpowiedź z modemu (może być NULL).
 * @param[in] rx_buf_len Maksymalna długość bufora na odpowiedź.
 * @param[in] timeout_ms Maksymalny czas oczekiwania na zakończenie operacji.
 * * @return int Liczba odebranych bajtów, 0 w przypadku timeoutu, lub -1 w przypadku błędu (ERROR).
 */
int send_at_cmd(const char* cmd, char* rx_buf, int rx_buf_len,
                uint32_t timeout_ms);
/**
 * @brief Bezpiecznie zamyka sesję MQTT i dezaktywuje kontekst PDP.
 */
void sim7070_mqtt_disconnect(void);

#ifdef __cplusplus
}
#endif
