/**
 * @file data_send_service.h
 * @brief Kolejkowanie i asynchroniczna wysyłka danych z sensorów.
 * @details Wykorzystuje kolejke FreeRTOS do buforowania danych, co pozwala na nieblokujące działanie pętli głównej.
 */

#pragma once

#include "sim7000_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct sensors_data_t
 * @brief Struktura danych pomiarowych wraz z elementami sieciowymi.
 * * @note Aby dodać nowy czujnik:
 * 1. Dodaj pole do struktury
 * 2. Zaktualizuj formatowaie JSON w data_send_service.cpp (data_sender_task)
 * 3. Zwiększ rozmiar bufora json_buffer if needed.
 */
typedef struct {
    float temperature;      /**< Temperatura w stopniach Celsjusza. */
    float humidity;         /**< Wilgotność względna w procentach. */
    cell_info_t cell_info;  /**< Metadane stacji bazowej, z której wysłano pakiet. */
} sensor_data_t;


/**
 * @brief Inicjalizuje usługę wysyłania
 * * @details Tworzy kolejkę o rozmiarze 10 elementów typu @ref sensor_data_t 
 * oraz task @p data_sender_task o priorytecie 5.
 */
void data_service_init(void);

/**
 * @brief Dodaje dane do kolejki wysyłkowej.
 * * @details Funkcja jest nieblokująca (timeout = 0). Jeśli kolejka jest pełna, 
 * dane zostaną odrzucone.
 * * @param[in] data Wskaźnik na strukturę z danymi do wysłania.
 * @return true jeśli dane pomyślnie trafiły do kolejki.
 */
bool data_service_push(sensor_data_t *data);


#ifdef __cplusplus
}
#endif