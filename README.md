# ESP32_RTOS_17_Timer_Interrupt_Demo_03

Interrupt differito con semaforo binario.

* Acquisizione di valori analogici dal canale 34 dell'ADC nella ISR del timer hardware, periodo 100ms
* In un task viene stampato il valore del campione acquisito dalla ISR.
 
La sincronizzazione tra ISR e task viene effettuata tramite semaforo binario.
