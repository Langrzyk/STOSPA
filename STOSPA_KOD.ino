//#include "Arduino.h"
#include <PCF8574.h>                        // PCF8574 library - Remote 8-bit I/O expander for I2C-bus
#include <OneWire.h>                        // OneWire library
#include <DallasTemperature.h>              // DS18B20 library - Tepmerature sensor
#include <Adafruit_NeoPixel.h>              // WS2812b library - Intelligent control LED 
#include <ESP8266HTTPClient.h>              // Client HTTP library
#include <ESP8266WiFi.h>                    // WiFi library

//D8-15X D7-13X D6-12X D5-14X D4-2 D3-0X D2- 4 D1-5 D0-16

#define ONE_WIRE_BUS 14                     // GPIO D5 where the DS18B20 is connected to
#define HEATER_PIN P0                       // GPIO P0 where the heater's relay is connected to
#define VALVE_PIN P2                        // GPIO P1 where the electrovalve's relay is connected to
#define PUMP_PIN P1                         // GPIO P2 where the pumps's relay is connected to
#define BUBBLE_PIN P3                       // GPIO P3 where the bubble's relay is connected to
#define LED_PIN 2                           // GPIO D4 where the WS2812b LED strip is connected to
#define TRIG_PIN 13                         // GPIO D7 where the HC-SR04 trigger pin is connected to
#define ECHO_PIN 12                         // GPIO D6 where the HC-SR04 echo pin is connected to
#define NUM_PIXELS 27                       // A number of pixels in the strip

//ACCESS WIFI
const char* ssid = "Beti";                  // Variable storing the name of the network
const char* pwd = "Betinkaa";               // Variable storing the password of the network
WiFiServer server(80);                      // open port 80 for server connection

// INITIALIZE DS18B20 (temperature sensor)
OneWire oneWire(ONE_WIRE_BUS);              // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);        // Pass our oneWire reference to Dallas Temperature sensor 

// INITIALIZE PCF8574 Remote 8-bit I/O expander for I2C-bus (Set i2c address D1 D2) 
PCF8574 expander(0x20);

// INITIALIZE WS2812b (led strip)
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

//LED
unsigned long LED_TIME = 100;                // The time we need to wait between led
uint32_t LED_color1;                         // current Color in case we need it
uint32_t LED_color2;                         // *
uint32_t LED_color_off;                      // *
uint16_t LED_nr = 0;                         // Current pixel are we operating on
bool LED_loop=0;                             // variable to control led loop

//TEMP
unsigned long TEMP_TIME = 750;               // The time we need to wait between temperature reading

//WATER
unsigned long WATER_TIME = 10;               // The time we need to wait between water level reading

unsigned long LED_Millis = 0;                // Variable for operations using the millis function  
unsigned long TEMP_Millis = 0;               // *
unsigned long WATER_Millis = 0;              // *
unsigned long SET_TEMP_Millis = 0;           // *

//variables from website
int LOGIN=0;                                 // Variable from get_status_DB() function which return status of the login status
int POWER=0;                                 // Variable from get_status_DB() function which return status of the power button
int BUBBLES=0;                               // Variable from get_status_DB() function which return status of the bubbles button
int LIGHT=0;                                 // Variable from get_status_DB() function which return status of the light button
float HEATING_TEMP;                          // Variable from get_status_DB() function which return the temperature of the user

//variables from sensor
float water_level;                           // Variable from water_distance() function which use HC-SR04 sensor and return level of water
float actual_temp;                           // Variable from temperature() function which use DS18B20 sensor and return actual temperature

//another variables
const float MAX_LEVEL = 9.00f;               // Variable determining the maximum level of liquid in the tank (the distance between the sensor and the water surface)
const float MIN_LEVEL = 13.00f;              // Variable determining the minimum level of liquid in the tank (the distance between the sensor and the water surface)
int stop_valve = 0;                          // Elctrovalve protection variable


/////////////////////////////////////// SETUP ////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  expander.begin();                          // Start Expander PCF8574P - Adress A0, A1, A2 - to GDN
  sensors.begin();                           // Start the DS18B20 sensor to reading temperature
  sensors.setWaitForConversion(0);           // Activation of asynchronous temperature reading 
  sensors.requestTemperatures();             // Send the command to get temperatures
  pixels.begin();                            // Initialize WS2812b Adafruit_NeoPixel     
  pixels.show();                             // Initialize all pixels to 'off'
  
  WiFi.mode(WIFI_STA);                       // Hides the viewing of ESP as wifi hotspot
  WiFi.begin(ssid, pwd);                     // Establishing a connection to a WiFi network

  // WAITING FOR AN INTERNET CONNECTION
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");                                          
  server.begin();                            // Start the server ??????????????????????????????????????????

  //Relay
  expander.pinMode(HEATER_PIN, OUTPUT);      // Position the heater pin as the output
  expander.digitalWrite(HEATER_PIN, HIGH);   // Setting the heater pin to high
  expander.pinMode(VALVE_PIN, OUTPUT);       // Position the electrovalve pin as the output
  expander.digitalWrite(VALVE_PIN, HIGH);    // Setting the electrovalve pin to high
  expander.pinMode(PUMP_PIN, OUTPUT);        // Position the pump pin as the output
  expander.digitalWrite(PUMP_PIN, HIGH);     // Setting the pump pin to high
  expander.pinMode(BUBBLE_PIN, OUTPUT);      // Position the bubble pin as the output
  expander.digitalWrite(BUBBLE_PIN, HIGH);   // Setting the bubble pin to high

  //HC-SR04 (ultrasonic distance sensor)
  pinMode(TRIG_PIN, OUTPUT);                 // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT);                  // Sets the echoPin as an Input

  //LED
  LED_color1 = pixels.Color(255,20,20);      // Set led color
  LED_color2 = pixels.Color(30,30,255);      // *
  //LED_color_off = pixels.Color(0,0,0);       // Set led color to off

}

//************************************** LOOP **************************************************

void loop() {

  // IF THERE IS A CONNECTION TO A WIFI NETWORK
  if(WiFi.status() == WL_CONNECTED){
    HEATING_TEMP = get_status_DB("setTemp");            // Taking the value of the temperature chosen by the user from database using get_status_DB() function
    water_level = water_distance();                     // Taking the water level (distance from the sensor) from the HC-SR04 sensor using water_distance() function 
    actual_temp = temperature();                        // Taking the temperature value from the DS18b20 sensor using temperature() function 
    
    LOGIN = get_status_DB("login");                     // Taking the information from the database on whether the user is logged in using get_status_DB() function
    POWER = get_status_DB("power");                     // Taking the information about turning the device on or off from the database using get_status_DB() function
    BUBBLES = get_status_DB("bubbles");                 // Taking the information about the status of a bubbles button from the database using get_status_DB() function
    LIGHT = get_status_DB("light");                     // Taking the information about the status of a light button from the database using get_status_DB() function

    // IF THE USER IS LOGGED IN
    if(LOGIN == 1){
      set_temp(actual_temp);                            // Update the temperature on the website

      // IF THE DEVICE IS SWITCHED ON (THE POWER BUTTON IS PRESSED) 
      if(POWER == 1){

        filling_water(water_level);                     // Filling the tank with water
        heat_water(HEATING_TEMP,                        // Heating the water in the tank
                   actual_temp,                         // *
                   water_level);                        // *
        
        // IF THE LIGHT BUTTON IS SWITCHED ON
        if(LIGHT == 1){
          if(LED_loop==0){ 
            if((unsigned long)(millis() - LED_Millis) >= LED_TIME) {
              LED_Millis = millis();
              loop_led(LED_color1);                     // Switching on the LED sequence with color LED_color1
              }
            }
          if(LED_loop==1){
            if((unsigned long)(millis() - LED_Millis) >= LED_TIME) {
              LED_Millis = millis();
              loop_led(LED_color2);                     // Switching on the LED sequence with color LED_color2
            }
          }
        }
        else{
          LED_nr = 0;                                   // Resetting the current diode number
          pixels.clear();                               // Turning off the LED bar
          pixels.show();                                // *
        }

        
        // IF THE BUBBLES BUTTON IS SWITCHED ON
        if(BUBBLES == 1){
          expander.digitalWrite(BUBBLE_PIN, LOW);       // Turn on the bubble
        }
        else{
          expander.digitalWrite(BUBBLE_PIN, HIGH);      // Turn off the bubble
        }

        
      }
      // IF THE DEVICE IS DISABLED
      else{                                             
        expander.digitalWrite(BUBBLE_PIN, HIGH);        // Turn off the bubble
        expander.digitalWrite(HEATER_PIN, HIGH);        // Turn off the heaters
        expander.digitalWrite(VALVE_PIN, HIGH);         // Turn off the electovale
        LED_nr = 0;                                     // Resetting the current diode number
        pixels.clear();                                 // Turning off the LED bar
        pixels.show();                                  // *

        if( stop_valve == 1 ) // Jesli było włączone
          pump_water(water_level);                      // Pumping water out of the tank.
      }
    }
       
    //IF THE USER LOGS OUT:
    else{ 
      expander.digitalWrite(BUBBLE_PIN, HIGH);          // Turn off the bubble
      expander.digitalWrite(HEATER_PIN, HIGH);          // Turn off the heaters
      expander.digitalWrite(VALVE_PIN, HIGH);           // Turn off the electovale
      LED_Millis = 0;                                   // Resetting time variables (so as not to overflow the buffer)
      TEMP_Millis = 0;                                  // *
      WATER_Millis = 0;                                 // *
      SET_TEMP_Millis = 0;                              // *
      LED_nr = 0;                                     // Resetting the current diode number
      pixels.clear();                                 // Turning off the LED bar
      pixels.show();                                  // *

      //If the user logs out unexpectedly before switching off the device
      if( stop_valve == 1 && water_level < MIN_LEVEL){
        expander.digitalWrite(PUMP_PIN,LOW);            // Pumping the remaining water in the tank
       }
      else{
        expander.digitalWrite(PUMP_PIN,HIGH);           // Turn off the pump
        stop_valve = 0;                                 // Release the elctrovalve variable
      }
    } 
  }
  else{
      expander.digitalWrite(PUMP_PIN, HIGH);            // Turn off the pump
      expander.digitalWrite(BUBBLE_PIN, HIGH);          // Turn off the bubble
      expander.digitalWrite(HEATER_PIN, HIGH);          // Turn off the heaters
      expander.digitalWrite(VALVE_PIN, HIGH);           // Turn off the electovale
  }
  
}


////////////////////////////////////////  FUNCTIONS  ///////////////////////////////////////////////////

/* 
 *  Funkcja odpowiedzialna za sterowanie temperaturą wody
 *  PARAMETRY:
 *    - Wybrana przez użytkownika temperatura docelową (informacja ta jest pobierana ze strony internetowej za pośrednictwem bazy danych przy użyciu funkcji: get_status_DB() )
 *    - Aktualna temperatura wody w zbiorniku odczytana z czujnika temperatury poprzez funkcję: temperature()
 *    - Aktualny poziom wody w zbiorniku uzyskany z czujnika odległośći poprzez funkcję: water_distance()
 *  OPIS:
 *    Funkcja odpowiada za przekazanie na wyjście pinu HEATER_PIN stanów LOW i HIGH co powoduje uruchomienie grzałek oraz ich wyłączenie.
 *    Stan niski odpowiedzialny za uruchomienie grzałek zostanie przekazany tylko wtedy gdy zostaną spełnione następujące warunki:
 *      Aktualna temperatura jest mniejsza lub równa temperaturze docelowej i poziom wody w zbiorniku osiąga wymagany poziom.
 *    Stan wysoki odpowiedzialny za wyłączenie grzałek zostanie przekazany gdy:
 *      Aktualna temperatura jest większa lub równa temperaturze docelowej z uzwględnieniem histerezy lub poziom wody w zbiorniku będzie zbyt niski.
 *    
 */
 
void heat_water(float HEATING_TEMP, float ACTUAL_TEMP, float WATER_LEVEL){ 
  if(ACTUAL_TEMP <= HEATING_TEMP  && WATER_LEVEL <= (MAX_LEVEL+2)){
    expander.digitalWrite(HEATER_PIN, LOW);
  }
  if(ACTUAL_TEMP >= (HEATING_TEMP+0.5) || WATER_LEVEL > MIN_LEVEL){ // histereza
    expander.digitalWrite(HEATER_PIN, HIGH);
  } 
}

/*  Funkcja odpowiedzialna za 
 *  PARAMETRY:
 *    - Aktualny poziom wody w zbiorniku uzyskany z czujnika odległośći poprzez funkcję: water_distance()
 *  OPIS:
 *    Funkcja odpowiada za przekazanie na wyjście pinu VALVE_PIN stanów LOW i HIGH co powoduje uruchomienie elektrozaworu oraz jego wyłączenie.
 *    Stan niski zostanie przekazany tylko wtedy gdy zostaną spełnione następujące warunki:
 *      Nie będzie wody w zbiorniku i zmienna zabezpieczająca włączenie elektrozaworu jest równa 0
 *    Stan wysoki odpowiedzialny za wyłączenie elektrozaworu zostanie przekazany gdy:
 *      Poziom wody w zbiorniku osiągnie wymagany poziom.
 */

void filling_water(float WATER_LEVEL){
  if(stop_valve==0 && WATER_LEVEL > MIN_LEVEL ){
    stop_valve=1;
    expander.digitalWrite(VALVE_PIN,LOW); //wlącz zawor
  }
  else{ 
    if(WATER_LEVEL <= MAX_LEVEL) 
    expander.digitalWrite(VALVE_PIN,HIGH); 
  }    
}

/*  Funkcja odpowiedzialna za 
 *  PARAMETRY:
 *    - Aktualny poziom wody w zbiorniku uzyskany z czujnika odległośći poprzez funkcję: water_distance()
 *  OPIS:
 *    Funkcja odpowiada za przekazanie na wyjście pinu PUMP_PIN stanów LOW i HIGH co powoduje uruchomienie pompy oraz jej wyłączenie.
 *    Stan niski odpowiedzialny za włączenie pompy zostanie przekazany tylko wtedy gdy zostaną spełniony następujący warunek:
 *      Woda znajduje się w zbiorniku
 *    Stan wysoki odpowiedzialny za wyłączenie pompy zostanie przekazany gdy:
 *      Poziom wody w zbiorniku spadnie do minimum
 */

void pump_water(float WATER_LEVEL){
   if( WATER_LEVEL < MIN_LEVEL ){ //Jeśli jest woda powyżej kratki
    expander.digitalWrite(PUMP_PIN,LOW);
   }
   else{
    expander.digitalWrite(PUMP_PIN,HIGH);
   }
}

/*  Funkcja odpowiedzialna za zapis wartości temperatury do bazy danych
 *  PARAMETRY:
 *    - Aktualna temperatura wody w zbiorniku odczytana z czujnika temperatury poprzez funkcję: temperature()
 *  OPIS:
 *    Funkcja odpowiada za zapis wartości temperatiry przekazanej jako parametr do bazy danych w celu jej wyświetlenia na stronie internetowej
 *    Funkcja aktualizuje baze danych co 750ms
 */

void set_temp(float ACTUAL_TEMP){
  if(WiFi.status() == WL_CONNECTED && (unsigned long)(millis() - SET_TEMP_Millis) > TEMP_TIME) { //if connected to WiFi
    SET_TEMP_Millis = millis();
    HTTPClient http;
    String url = "http://192.168.43.223/Ardunet/set_temp.php?temp=" + String(ACTUAL_TEMP);
    http.begin(url);
    int httpCode = http.GET(); //GET method 
  }
}

/*  Funkcja odpowiedzialna za pobranie informacji o wciśniętych przyciskach
 *  PARAMETRY:
 *    - Nazwa przycisku o ktrego stan wciśnięcia chcemy odczytać
 *  OPIS:
 *    Funkcja odpowiada za sprawdzenie połączenia i odczytanie informacji o stanie wciśniętego przycisku którego nazwa jest przekazana poprzez parametr. 
 *    Główny odczyt wykonuje skrypt php o nazwie get_status2.php. 
 *  ZWRACA
 *      - wartość 1 gdy został wciśniety przycisk ON 
 *      - wartość 0 gdy został wciśniety przycisk OFF
 *      - wartość -1 gdy połączenie z internetem jest nie aktywne
 */

//Data from database
float get_status_DB(String value)
{
  if(WiFi.status() == WL_CONNECTED) { //if connected
    WiFiClient client = server.available();    
    HTTPClient http;
    String url = "http://192.168.43.223/Ardunet/get_status2.php?value=" + value;
    http.begin(url);
       
    int httpCode = http.GET(); //GET method
    String load_data = http.getString();
    Serial.print(load_data);
    return load_data.toFloat();
  }
  else{
    Serial.print("-1");
    return -1.0;
  }
}  

/*  Funkcja odpowiedzialna za odczyt temperatury z czujnika
 *  PARAMETRY:
 *    - brak
 *  OPIS:
 *    Funkcja co 750ms wysyła zapytanie requestTemperature() w celu odczytania informacji o wartości temperatury z czujnika DS18B20
 *  ZWRACA:
 *    - Aktualna wartość temperatury
 */

float temperature(){
  if ((unsigned long)(millis() - TEMP_Millis) > TEMP_TIME) {
     TEMP_Millis = millis();
     actual_temp = sensors.getTempCByIndex(0);
     sensors.requestTemperatures(); 
  }
  return actual_temp;
}

/*  Funkcja odpowiedzialna za odczyt poziomu wody w zbiorniku
 *  PARAMETRY:
 *    - brak
 *  OPIS:
 *    
 */

float water_distance(){
  if ((unsigned long)(millis() - WATER_Millis) > WATER_TIME) {
    WATER_Millis = millis();
    float time;
    digitalWrite(TRIG_PIN, LOW); // Clears the trigPin
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); // Sets the trigPin on HIGH state for 10 micro seconds
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
  
    time = pulseIn(ECHO_PIN, HIGH);
    return time / 58;                    //documentation
  }
    
}

/*  Funkcja odpowiedzialna za animacje ledów
 *  PARAMETRY:
 *    - kolor ledów 
 *  OPIS:
 *    
 */

//LED function
void loop_led(uint32_t LED_color){
  pixels.setPixelColor(LED_nr,LED_color);
  pixels.show();
  LED_nr++;
  if(LED_nr == NUM_PIXELS){
    LED_nr = 0;
    LED_loop=!LED_loop;
  }
}
