/*
 * FreeRTOS Esempio 17: Interrupt differito con semaforo binario.
 *
 * Acquisizione di valori analogici dal canale 34 dell'ADC nella ISR del timer hardware, periodo 100ms;
 * 
 * In un task viene stampato il valore del campione acquisito dalla ISR.
 * 
 * Sincronizzazione tra ISR e task mediante semaforo binario.
 *
 * Nota: nel file soc.h sono definiti i riferimenti ai due core della ESP32:
 *   #define PRO_CPU_NUM (0)
 *   #define APP_CPU_NUM (1)
 *
 * Qui viene adoperata la APP_CPU
 *
 */

#include <Arduino.h>

// pin driver del Led
static const int pinLed = GPIO_NUM_23;
// pin del canale analogico
static const int pinAdc = GPIO_NUM_34;

// variabili globali
// divisore del timer hardware (per tick = 1us)
static const uint16_t timer_divider = 80;
// costante di tempo del timer hardware (100ms)
static const uint64_t timer_max_count = 100000;
// tempo di ciclo del task
static const TickType_t task_delay = pdMS_TO_TICKS(2000);

// handler del timer hardware
static hw_timer_t *timer = NULL;
// valore numerico restituito dall'ADC
static volatile uint16_t val;
// semaforo di sincronizzazione (per l'interrupt differito)
static SemaphoreHandle_t bin_sem = NULL;

//*****************************************************************************
// Interrupt Service Routines (ISRs)

// ISR del timer hardware, eseguita quando il timer raggiunge il valore timer_max_count
void IRAM_ATTR onTimer()
{
  BaseType_t task_woken = pdFALSE;

  // Toggle LED
  int pin_state = digitalRead(pinLed);
  digitalWrite(pinLed, !pin_state);

  // acquisizione del valore dall'ADC
  val = analogRead(pinAdc);

  // Libera il semaforo per dire al task che il nuovo valore è pronto
  xSemaphoreGiveFromISR(bin_sem, &task_woken);

  // Uscita dalla ISR (per il FreeRTOS normale)
  //portYIELD_FROM_ISR(task_woken);

  // Uscita dalla ISR (per il porting di FreeRTOS di ESP-IDF)
  // richiesta di context switch se il semaforo ha sbloccato il task
  if (task_woken) {
    portYIELD_FROM_ISR();
  }
}

//*****************************************************************************
// Tasks

// Attende che venga liberato il semaforo dalla ISR poi stampa il valore acquisito dall'ADC
void printValues(void *parameters) {
  // valore in Volt del dato acquisito
  float volt;

  // ciclo infinito, attente che venga liberato il semaforo poi stampa il valore
  while (1) {
    xSemaphoreTake(bin_sem, portMAX_DELAY);
    volt = (float) val * 3.3 / 4096.0;
    Serial.println(volt, 3); 
  }
}

//*****************************************************************************
// Main (sul core 1, con priorità 1)

// configurazione del sistema
void setup()
{
  // Configurazione della seriale
  Serial.begin(115200);

  // breve pausa
  vTaskDelay(pdMS_TO_TICKS(1000));
  Serial.println();
  Serial.println("FreeRTOS Software timer: demo 3 - interrupt differito con semaforo");

  // Crea il semaforo prima che venga usato nel task o nella ISR
  bin_sem = xSemaphoreCreateBinary();

  // Forza il riavvio della ESP32 se non è possibile creare il semaforo
  if (bin_sem == NULL) {
    Serial.println("ERRORE: impossibile creare il semaforo");
    ESP.restart();
  }

  // Crea il task per la stampa della variabile 
  // con priorità più alta di quella del task setup&loop come richiesto per il context switch nella ISR
  xTaskCreatePinnedToCore(
              printValues,    // Funzione da eseguire
              "Print values", // Nome del task
              1024,           // Stack del task
              NULL,           // parametri per il task
              2,              // Livello di priorità aumentato
              NULL,           // Puntatore al task
              APP_CPU_NUM);   // Core su sui eseguire il task

  // Configurazione del pin del led
  pinMode(pinLed, OUTPUT);

  // Crea e avvia il timer hardware num. 0 (num, divider, countUp)
  timer = timerBegin(0, timer_divider, true);

  // Associa la ISR al timer (timer, function, edge)
  timerAttachInterrupt(timer, &onTimer, true);

  // Imposta il valore di conteggio al quale eseguire la ISR (timer, count, autoreload)
  timerAlarmWrite(timer, timer_max_count, true);

  // Abilita la generazione degli interrupt del timer
  timerAlarmEnable(timer);

  // Elimina il task con "Setup e Loop"
  //vTaskDelete(NULL);
}

void loop()
{
  // lasciare vuoto
}