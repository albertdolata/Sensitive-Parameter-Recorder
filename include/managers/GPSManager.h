/**
 * @file GPSManager.h
 * @brief Plik nagłówkowy zawierający deklaracje klasy GPSManager.
 */

#pragma once
#include <Arduino.h>
#include "../simcom/sim7070_core.h"

/** 
 * @brief Klasa zarządzająca sprzętowym modułem GNSS.
 *
 * @details Zapewnia wysokopoziomowy interfejs (API) do obsługi modułu GNSS na układzie SIM7070.
 * Hermetyzuje proces zarządzania zasilaniem anteny, asynchronicznego odpytywania o pozycję 
 * oraz parsowania surowych ramek tekstowych AT do natywnych typów danych.
 */

class GPSManager {
   private:
    double latitude;    /**< Szerokość geograficzna w formacie dziesiętnym (DD.dddddd) */
    double longitude;   /**< Długość geograficzna w formacie dziesiętnym (DD.dddddd) */
    uint32_t timestamp; /**< Znacznik czasu UTC w formacie UNIX Epoch (sekundy od 1970) */
    bool isON;          /**< Flaga stanu zasilania silnika GNSS (odpowiednik AT+CGNSPWR) */
    bool fixStatus;     /**< Flaga statusu nawigacji (1 = namierzono satelity, 0 = szukanie) */

    /** 
     * @brief Parsuje surowy ciąg czasu GNSS na format UNIX timestamp.
     *
     * @details Konwertuje specyficzny format czasu zwracany przez układ SIM7070 
     * (najczęściej zapisany jako zbitka "yyyyMMddHHmmss.sss") na uniwersalny licznik 
     * sekund od epoki UNIX.
     * 
     * @param[in] rawTime Surowy ciąg znaków z czasem wyciągnięty z ramki (np. "20260805133053.000").
     */
    void parseTime(String rawTime);

    /** 
     * @brief Parsuje odpowiedź sprzętową z modułu GNSS (komenda CGNSINF).
     *
     * @details Przetwarza wejściowy bufor tekstowy, tokenizując go po przecinkach, 
     * aby zaktualizować wewnętrzny stan obiektu.
     * Oczekiwany format zgodny z dokumentacją SIM7070:
     * "+CGNSINF: <GNSS run status>,<Fix status>,<UTC date & Time>,<Latitude>,<Longitude>,..."
     * 
     * @param[in] response Surowy ciąg znaków odebrany z portu UART modemu.
     */
    void parseResponse(String response);

   public:
    GPSManager();

    /** 
     * @brief Inicjalizuje układ GNSS.
     *
     * @details Wysyła sekwencję komend AT włączających zasilanie układu GNSS.
     * Funkcja musi być wywołana przed pierwszą próbą odczytu danych (update).
     */
    void begin();

    /** 
     * @brief Odpytuje modem o aktualną pozycję i odświeża stan obiektu.
     *
     * @details Funkcja wysyła zapytanie sprzętowe o podanie bieżącej lokalizacji,
     * a w przypadku prawidłowej odpowiedzi wywołuje wewnętrzny parser aktualizujący 
     * współrzędne oraz czas.
     */
    void update();

    /** 
     * @brief Usypia układ GNSS.
     *
     * @details Zatrzymuje akwizycję danych satelitarnych i odcina zasilanie 
     * od obwodów RF anteny GPS. Zmienne wewnątrz klasy zachowują ostatni znany stan.
     */
    void pause();

    /** 
     * @brief Wybudza układ GNSS z uśpienia.
     *
     * @details Ponownie uruchamia zasilanie anteny. Wymaga czasu na ponowne 
     * zsynchronizowanie się z satelitami.
     */
    void resume();

    /** 
     * @brief Zwraca zapisaną szerokość geograficzną.
     * @return Aktualna szerokość geograficzna w stopniach.
     */
    double getLatitude();

    /** 
     * @brief Zwraca zapisaną długość geograficzną.
     * @return Aktualna długość geograficzna w stopniach.
     */
    double getLongitude();

    /** 
     * @brief Zwraca ostatni zsynchronizowany czas satelitarny.
     * @return Znacznik czasu w formacie UNIX (UTC). Zwraca 0, jeśli moduł nigdy nie złapał fixa.
     */
    uint32_t getTimestamp();

    /** 
     * @brief Weryfikuje, czy moduł śledzi satelity i podaje wiarygodną pozycję.
     * @return true jeśli wyznaczono pozycję przestrzenną (Fix = 1), false w przeciwnym razie.
     */
    bool hasFix();

    /** 
     * @brief Sprawdza status zasilania układu nawigacyjnego.
     * @return true jeśli antena GNSS jest aktualnie zasilona.
     */
    bool isPowered();
};