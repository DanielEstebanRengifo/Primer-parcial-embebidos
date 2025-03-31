#include "DHT.h"
#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#define DHTTYPE DHT11
//--------------------------------------
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Declaracion de pines
#define DHTPIN 15
#define pinAlarma 2
#define pinLedAlarma 4
#define pinLed 5
#define PinLuz 34
#define PinBoton 33 
#define PinInfra  25
//Pines del reloj en el set up
//-------------------------------------
DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//VARIABLES
typedef struct {
  float ValueTemperatura;
  float ValueHumedad;
  int ValueLuz;
  int year, month, day, hour, minute, second;
  //Cadenas para formateo--------------
  char date[11]; // "DD/MM/AAAA"
  char time[9];  // "HH:MM:SS" 
  char temp[6];  // "HH.HH" 
  char hum[6];   // "HH.HH"
  char luz[5];   // "LLLL" 
  char contadorBotonStr[5]; // "0000" 
  char contadorInfraStr[5]; // "0000" 
} SensoresData; SensoresData Cadena;
//------------------------------------------------------------
//COSAS PARA LAS INTERRPCIONES
//Contador 
volatile int contadorBoton = 0;
//Sensor inflarojo 
volatile int contadorInfra = 0;
//Mux 
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Definir core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Retardos----------------------------
static const int rate_LED = 500; 
static const int rate_LUZ = 1700;
static const int rate_Temperatura = 2300;
static const int rate_Humedad = 2600; 
static const int rate_DISplay = 10000 ;
//Task--------------------------------
TaskHandle_t TaskTemperatura = NULL;
TaskHandle_t TaskHumedad = NULL;
TaskHandle_t TaskLED = NULL;
TaskHandle_t TaskLUZ = NULL;
TaskHandle_t TaskDisplay = NULL;
TaskHandle_t TaskSleep;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(pinAlarma, OUTPUT);
  pinMode(pinLedAlarma, OUTPUT);
  pinMode(pinLed, OUTPUT);
  pinMode(PinLuz, INPUT); 
  pinMode(PinBoton, INPUT_PULLUP); 
  pinMode(PinInfra, INPUT_PULLUP);
  //-------------------------------------------
  //Reloj
  Wire.begin(21, 22); //Pines reloj 
  if (!rtc.begin()) {
    Serial.println("Error al iniciar RTC");
    while (1);
  }
  //-------------------------------------------
  //%%%%%%%%%%%%%%%
  //CONFIGURAR HORA
  rtc.adjust(DateTime(2025, 3, 25, 10, 53, 0));  //YYYY, MM, DD, HH, MM, SS
  //%%%%%%%%%%%%%%%
  //-------------------------------------------
  // Inicializar contadores
  contadorBoton = 0;
  contadorInfra = 0;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  //TAREAS
//-------------------------------------------------------------
  xTaskCreatePinnedToCore(Temperatura,"ReadDHT",4096,NULL,configMAX_PRIORITIES-1,&TaskTemperatura,app_cpu);
  xTaskCreatePinnedToCore(Humedad,"ReadDHT",4096,NULL,configMAX_PRIORITIES-1,&TaskHumedad,app_cpu);
  xTaskCreatePinnedToCore(LED,"HighLED",1024,NULL,configMAX_PRIORITIES-1,&TaskLED,app_cpu);
  xTaskCreatePinnedToCore(LUZ,"ReadLUZ",1024,NULL,configMAX_PRIORITIES-1,&TaskLUZ,app_cpu);
  xTaskCreatePinnedToCore(DisplayData ,"ReadDHT",4096,NULL,configMAX_PRIORITIES-1,&TaskDisplay,app_cpu);
  xTaskCreatePinnedToCore(Sleep ,"SleepEsp32",4096,NULL,configMAX_PRIORITIES-2,&TaskSleep,app_cpu);
}
void loop(){} 
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//TAREAS------------------------------------------------------- 
//Leer temperatura
void Temperatura(void *parameter) {
  while(1) {
   Cadena.ValueTemperatura = dht.readTemperature();
   //Alarma--------------------------------------
   if (isnan(Cadena.ValueTemperatura)) {
        digitalWrite(pinAlarma, HIGH);
        digitalWrite(pinLedAlarma, HIGH); // Encender LED de error
    } else {
        digitalWrite(pinAlarma, LOW);
        digitalWrite(pinLedAlarma, LOW); // Apagar LED si la lectura es correcta
  }
   //Formatear
   sprintf(Cadena.temp, "%02d.%02d", (int)Cadena.ValueTemperatura, (int)(Cadena.ValueTemperatura * 100) % 100);
   vTaskDelay(rate_Temperatura / portTICK_PERIOD_MS);
  }
}
//Leer humedad
void Humedad(void *parameter) {
  while(1) {
   Cadena.ValueHumedad = dht.readHumidity();
   //Alarma--------------------------------------
   if (isnan(Cadena.ValueHumedad)) {
        digitalWrite(pinAlarma, HIGH);
        digitalWrite(pinLedAlarma, HIGH); // Encender LED de error
    } else {
        digitalWrite(pinAlarma, LOW);
        digitalWrite(pinLedAlarma, LOW); // Apagar LED si la lectura es correcta
  }
   //Formatear
   sprintf(Cadena.hum, "%02d.%02d", (int)Cadena.ValueHumedad, (int)(Cadena.ValueHumedad * 100) % 100);
   vTaskDelay(rate_Humedad / portTICK_PERIOD_MS);
  }
}
//Led que se enciende y se apaga 
void LED(void *parameter) {
  while(1) {
   digitalWrite(pinLed, HIGH);
   vTaskDelay(rate_LED / portTICK_PERIOD_MS);
   digitalWrite(pinLed, LOW);  
   vTaskDelay(rate_LED / portTICK_PERIOD_MS);
  }
}
//leer luz
void LUZ(void *parameter) {
  while(1) {
   Cadena.ValueLuz = analogRead(PinLuz);
   //Formatear
   sprintf(Cadena.luz, "%04d", Cadena.ValueLuz);
   vTaskDelay(rate_LUZ / portTICK_PERIOD_MS);
  }
}
void Sleep(void *parameter) {
  while(1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Configurar wakeup por TIMER (10 segundos)
    esp_sleep_enable_timer_wakeup(10 * 1000000);

    // CONGIFIGURAR WAKE UP  
    esp_sleep_enable_ext0_wakeup(gpio_num_t(PinBoton), LOW); //Boton EXT1
    esp_sleep_enable_ext1_wakeup((1ULL << PinInfra), ESP_EXT1_WAKEUP_ALL_LOW); //infra EXT0
    // Entrar en modo light sleep
    esp_light_sleep_start();

    // Verificar qué fuente de wakeup activó el despertar
    esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause();
    //---------------------------------------------------------------
    //CONTADORES 
    //Infra
    if (causa == ESP_SLEEP_WAKEUP_EXT1) {
      portENTER_CRITICAL(&mux);
      contadorInfra++;
      portEXIT_CRITICAL(&mux);
    }
    //Boton
    if (causa == ESP_SLEEP_WAKEUP_EXT0) {
      portENTER_CRITICAL(&mux);
      contadorBoton++;
      portEXIT_CRITICAL(&mux);
    }
  }
}
//Desplegar datos y trama
void DisplayData(void *parameter) {
  while(1) {
   //Reloj------------------------------------------------
   DateTime now = rtc.now();// Leer la hora actual
   Cadena.year = now.year();
   Cadena.month = now.month();
   Cadena.day = now.day();
   Cadena.hour = now.hour();
   Cadena.minute = now.minute();
   Cadena.second = now.second();
   //FORMATEO
   //Hora
   sprintf(Cadena.date, "%02d/%02d/%04d", Cadena.day, Cadena.month, Cadena.year);
   sprintf(Cadena.time, "%02d:%02d:%02d", Cadena.hour, Cadena.minute, Cadena.second);
   //Interrupciones
   sprintf(Cadena.contadorBotonStr, "%04d", contadorBoton);
   sprintf(Cadena.contadorInfraStr, "%04d", contadorInfra);
   //---------------------------------------------------------
   //Imprimir datos (temporarl, borrar antes de la entrega)
   Serial.println("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");

   Serial.print("Fecha: ");
   Serial.print(Cadena.date);
   Serial.print("  Hora: ");
   Serial.println(Cadena.time);
   Serial.print("Humidity: ");
   Serial.print(Cadena.hum);
   Serial.print("||");
   Serial.print("Temperature: ");
   Serial.print(Cadena.temp);
   Serial.print("°C");
   Serial.print("||");
   Serial.print("Luz: ");
   Serial.println(Cadena.luz);
   Serial.println("------------------------------------------------------------");
   Serial.print("Pulsaciones: ");
   Serial.print(Cadena.contadorBotonStr);
   Serial.print("||");
   Serial.print("Interrpciones inflarojo: ");
   Serial.println(Cadena.contadorInfraStr);
   
   Serial.println("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");
   //-----------------------------------------------
   //TRAMA: 45 caracteres tiene la trama:(gracias al formateo siempre tendra 45 caracteres)
    Serial.print("$$");
    Serial.print(Cadena.date);Serial.print(",");
    Serial.print(Cadena.time);Serial.print(",");
    Serial.print(Cadena.hum); Serial.print(",");
    Serial.print(Cadena.temp); Serial.print(",");
    Serial.print(Cadena.luz); Serial.print(",");
    Serial.print(Cadena.contadorBotonStr); Serial.print(",");
    Serial.print(Cadena.contadorInfraStr);
    Serial.println("$$");
    Serial.println("------------------------------------------------------------");
    //----------------------------------------------
    Serial.println(" ");
    Serial.println(" ");
    Serial.println(" ");
    vTaskDelay(rate_DISplay / portTICK_PERIOD_MS);
    xTaskNotifyGive(TaskSleep);  // Llama a Sleep
  }
}