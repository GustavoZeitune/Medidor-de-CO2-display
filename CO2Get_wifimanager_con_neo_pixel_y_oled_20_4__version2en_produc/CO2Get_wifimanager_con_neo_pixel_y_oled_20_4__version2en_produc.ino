//Sensor CO2
//#include <OTA.h>
#include "WebServer.h"
//#include <Wifi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LittleFS.h>//***********************
#define SPIFFS LittleFS//*********************

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "MHZ19.h"
#include <SoftwareSerial.h>

//AGREGADO OLED Y NEOPIXEL
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

#include <Ticker.h>
#include <RTClib.h>

String firmVer = "7.0";

String webpage = "";
String datos_set;
String status_string = "";

Ticker timer_1ms;
RTC_DS3231 rtc;
bool RTC_OK;

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET LED_BUILTIN  //4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C     ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Which pin on the ESP8266 is connected to the NeoPixels?
#define PIN 0

// How many NeoPixels are attached to the ESP8266?
#define NUMPIXELS 10

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int flg_guardar_dato = 0;
int Status = WL_IDLE_STATUS;
int Status_ant = 99;

int cont_amarillo = 0;
int cont_display = 0;
int estado_CO2 = 0;

//FIN AGREGADO OLED Y NEOPIXEL

#define RX_PIN 13      // D7 - Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 15      // D8 - Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600  // Device to MH-Z19 Serial baudrate (should not be changed)

#define LED_D6 12

const String SensorID = String(ESP.getChipId(), HEX);

HTTPClient http_post;
WiFiClient client_post;
HTTPClient http_post2;
WiFiClient client_post2;
HTTPClient http_get;
WiFiClient client_get;
String respuestaPost = "";
String respuestaPostLocal = "";
String respuestaGet = "";
String IP_Puerto_local = "";

const int RSSI_MAX = -50;   // define maximum strength of signal in dBm
const int RSSI_MIN = -100;  // define minimum strength of signal in dBm

uint32_t PAUSA = 6;  // 5 min o 6 seg por ahora;


MHZ19 myMHZ19;                            // Constructor for library
SoftwareSerial mySerial(RX_PIN, TX_PIN);  // (Uno example) create device to MH-Z19 serial


int muestras_CO2[21];
byte index_CO2 = 1;
int ultimoCO2 = 0;
DateTime fechaUltimoDato;

ADC_MODE(ADC_VCC);  //este modo sirve para habilitar el divisor interno y
//y poder medir correctamente el BUS de 3.3v

void connect();

long connect_tic = 120000;  // interval at which to blink (milliseconds)
long OLED_tic = 0;
long LEDS_tic = 0;
long CO2_tic = 0;
long fin_AP_tic = 0;

#define MAX_BYTES 1500000


void Timer_1ms() {
  if (connect_tic > 0) connect_tic--;
  if (OLED_tic > 0) OLED_tic--;
  if (LEDS_tic > 0) LEDS_tic--;
  if (CO2_tic > 0) CO2_tic--;
  if (fin_AP_tic > 0) fin_AP_tic--;
}

//-------------------VARIABLES GLOBALES--------------------------
int contconexion = 0;

char ssid[50];      
char pass[50];

int flag_guardar = 0;
int flag_fin_AP = 0;

const char *passConf = "12345678";

String mensaje = "";

//-----------CODIGO HTML PAGINA DE CONFIGURACION---------------
String pagina = "";
String paginafin = "</body>""</html>";
//-------------------------------------------------------------

//------------------------SETUP WIFI-----------------------------
void setup_wifi() {
// Conexión WIFI
  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(pass, 50);
  WiFi.mode(WIFI_STA); //para que no inicie el SoftAP en el modo normal
//  WiFi.mode(WIFI_AP_STA); /*1*/ //ESP8266 works in both AP mode and station mode
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED and contconexion <50) { //Cuenta hasta 50 si no se puede conectar lo cancela
    ++contconexion;
    delay(100);
    Serial.print(".");
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
  }
  if (contconexion <50) {   
      Serial.println("");
      Serial.println("WiFi conectado");
      Serial.print("IP que asignada en la red: ");
      Serial.println(WiFi.localIP());
      digitalWrite(2, HIGH);  
      contconexion = 0;
  }
  else { 
      Serial.println("");
      Serial.println("Error de conexion");
      digitalWrite(2, LOW);
      contconexion = 0;
  }
}

//-------------------PAGINA DE CONFIGURACION--------------------
void Wificonf() {
pagina = "<!DOCTYPE html>"
"<html>"
"<head>"
"<title>Configuración WiFi</title>"
"<meta charset='UTF-8'>"
"</head>"
"<body>";
pagina += "<p>SSID: " + String(ssid) + "</p>";
pagina += "<p>IP asignada por la red: " + WiFi.localIP().toString() + "</p>";
pagina += "</form>"
"<form action='guardar_conf' method='get'>"
"SSID:<br><br>"
"<input class='input1' name='ssid' type='text'><br>"
"PASSWORD:<br><br>"
"<input class='input1' name='pass' type='password'><br><br>"
"<input class='boton' type='submit' value='GUARDAR'/><br><br>"
"</form>"
"<a href='escanear'><button class='boton'>ESCANEAR</button></a><br>"
"<br><a href='/'>[Back]</a><br>";


  WebServer.send(200, "text/html", pagina + mensaje + paginafin); 
}

//--------------------MODO_CONFIGURACION------------------------
void modoconf() {
   
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);

  WiFi.softAP(("Sensor_" + SensorID), passConf);
  IPAddress myIP = WiFi.softAPIP(); 
  Serial.print("IP del access point: ");
  Serial.println(myIP);
  Serial.println("WebServer iniciado...");

  WebServer.on("/", Index); //Index
  
  WebServer.on("/wifi", Wificonf); //Configuración WiFi

  WebServer.on("/guardar_conf", guardar_conf); //Graba en la eeprom la configuracion

  WebServer.on("/escanear", escanear); //Escanean las redes wifi disponibles
  
  WebServer.on("/ultimodato", UltimoDatoEnviado); //Podemos ver el ultimo dato enviado
  
  WebServer.on("/envio", DatoEnviado); //Enviar un dato

  WebServer.begin();

}

//---------------------GUARDAR CONFIGURACION-------------------------
void guardar_conf() { 
  Serial.println(WebServer.arg("ssid"));//Recibimos los valores que envia por GET el formulario web
  grabar(0,WebServer.arg("ssid"));
  Serial.println(WebServer.arg("pass"));
  grabar(50,WebServer.arg("pass"));

  mensaje = "Configuracion Guardada...";
  Wificonf();
  mensaje = "";
  flag_guardar = 1;
}

//----------------Función para grabar en la EEPROM-------------------
void grabar(int addr, String a) {
  int tamano = a.length(); 
  char inchar[50]; 
  a.toCharArray(inchar, tamano+1);
  for (int i = 0; i < tamano; i++) {
    EEPROM.write(addr+i, inchar[i]);
  }
  for (int i = tamano; i < 50; i++) {
    EEPROM.write(addr+i, 255);
  }
  EEPROM.commit();
}

//-----------------Función para leer la EEPROM------------------------
String leer(int addr) {
   byte lectura;
   String strlectura;
   for (int i = addr; i < addr+50; i++) {
      lectura = EEPROM.read(i);
      if (lectura != 255) {
        strlectura += (char)lectura;
      }
   }
   return strlectura;
}
//---------------------------ESCANEAR----------------------------
void escanear() {  
  int n = WiFi.scanNetworks(); //devuelve el número de redes encontradas
  Serial.println("escaneo terminado");
  if (n == 0) { //si no encuentra ninguna red
    Serial.println("no se encontraron redes");
    mensaje = "no se encontraron redes";
  }  
  else
  {
    Serial.print(n);
    Serial.println(" redes encontradas");
    mensaje = "";
    for (int i = 0; i < n; ++i)
    {
      // agrega al STRING "mensaje" la información de las redes encontradas 
      mensaje = (mensaje) + "<p>" + String(i + 1) + ": " + WiFi.SSID(i) + " </p>\r\n";
//      mensaje = (mensaje) + "<p>" + String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") Ch: " + WiFi.channel(i) + " Enc: " + WiFi.encryptionType(i) + " </p>\r\n";
      //WiFi.encryptionType 5:WEP 2:WPA/PSK 4:WPA2/PSK 7:open network 8:WPA/WPA2/PSK
      delay(20);
    }
    Serial.println(mensaje);
    Wificonf();
    mensaje = "";
  }
}
//--------------------------------------------------------------------------

// --- Redondear una float o una double
  float redondear(float valor, int decimales) {
    double _potencia = pow(10, decimales);
    return (roundf(valor * _potencia) / _potencia);
  };
//--------------------------------------------------------------------------


void setup() {

  //AGREGADO OLED Y NEOPIXEL
  Serial.begin(115200);

  pinMode(LED_D6, OUTPUT);
  pinMode(2, OUTPUT); // D7 
  EEPROM.begin(512);


  // put your setup code here, to run once:
  pixels.begin();  // This initializes the NeoPixel library.
  pixels.show();
  pixels.setBrightness(150);

  pixels.fill(pixels.Color(0, 0, 0), 0, NUMPIXELS);
  pixels.show();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.display();

  display.setCursor(0, 55);  // Start at top-left corner

  display.setTextSize(1);  // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.println(firmVer);

  display.setCursor(15, 0);  // Start at top-left corner

  display.setTextSize(4);  // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.println(F("ADOX"));

  display.setCursor(0, 30);  // Start at top-left corner

  display.setTextSize(2);  // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.println(F("MonitorCO2"));
  display.display();


  WebServer.init();
  
  setup_wifi();

  modoconf();
  
  WebServer.on("/borrar_memoria", HTTP_GET, [&]() {
    if (SPIFFS.format()) WebServer.send(200, "text/plain", "Memoria borrada OK");
    else WebServer.send(200, "text/plain", "Error al borrar Memoria");
  });

  WebServer.on("/download", File_Download);
  WebServer.on("/date", Config_Date);
  WebServer.on("/sensed", Config_Sensed);

  datos_set = leer(100);
  IP_Puerto_local = leer(150);

  delay(2000);


  display.clearDisplay();
  display.setTextSize(2);  // Draw 2X-scale text

  display.setCursor(0, 0);  // Start at top-left corner
  display.println(F("SensorID:"));
 
  display.setCursor(0, 16);  // Start at top-left corner
  display.println(SensorID);

  display.display();
  delay(2000);

  display.setTextSize(1);  // Draw 2X-scale text

  display.setCursor(0, 32);  // Start at top-left corner
  display.print(F("IP: "));
  display.println(WiFi.localIP().toString());

  display.setCursor(0, 40);  // Start at top-left corner
  display.println(F("SSID: "));

  display.setCursor(0, 50);  // Start at top-left corner
  display.println(WiFi.SSID());

  display.display();
  delay(3000);

  mySerial.begin(BAUDRATE);  // (Uno example) device to MH-Z19 serial start
  myMHZ19.begin(mySerial);   // *Serial(Stream) refence must be passed to library begin().

  myMHZ19.autoCalibration(true);  // Turn auto calibration ON (OFF autoCalibration(false))

  ////////////int a = connect();
  // connect();

  timer_1ms.attach_ms(1, Timer_1ms);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    RTC_OK = 0;
  }
  else RTC_OK = 1;

  if (!SPIFFS.begin())
  {
    Serial.println("[Storage] Couldn't mount file system.");
  }

  fin_AP_tic = 900000; // 15 minutos con AP

}

void loop() {
  WebServer.handleClient();

  if (!CO2_tic) {
    CO2_tic = 2000;
    long CO2_aux = myMHZ19.getCO2();  // Request CO2 (as ppm
    Serial.println(CO2_aux);

    if (CO2_aux > 0) {

//      for (int i = 1 ; i < 11 ; i++){
//        Serial.print(muestras_CO2[i]);
//        Serial.print(" ");                
//      }
//      Serial.println("|");                

      muestras_CO2[index_CO2] = CO2_aux;
      index_CO2++;
      if (index_CO2 == 21) index_CO2 = 1;

      CO2_aux = 0;
      for (int i = 1 ; i < 21 ; i++){
        CO2_aux += muestras_CO2[i];        
      }
      muestras_CO2[0] = CO2_aux / 20;
    }
    else {
      
    }
  }

  if (!OLED_tic) {
    OLED_tic = 2000;
    testdrawstyles();
  }

  if (!LEDS_tic) {
    LEDS_tic = 500;

    if (muestras_CO2[0] == 0) {
      cont_amarillo = 0;
      estado_CO2 = 0;

      pixels.fill(pixels.Color(0, 0, 255), 0, NUMPIXELS);
      pixels.show();
    } else if (muestras_CO2[0] < 700) {
      cont_amarillo = 0;
      estado_CO2 = 1;

      pixels.fill(pixels.Color(0, 255, 0), 0, NUMPIXELS);
      pixels.show();
    } else if (muestras_CO2[0] < 1400) {
      estado_CO2 = 2;

      pixels.fill(pixels.Color(255, 100, 0), 0, NUMPIXELS);
      pixels.show();

      cont_amarillo++;
      if (cont_amarillo > 1000) {
        cont_amarillo = 0;
        digitalWrite(LED_D6, HIGH);  // sonalert encendido
        delay(100);
        digitalWrite(LED_D6, LOW);  // sonalert apagado
      }
    } else {
      estado_CO2 = 3;

      pixels.fill(pixels.Color(0, 0, 0), 0, NUMPIXELS);
      pixels.show();

      digitalWrite(LED_D6, HIGH);  // sonalert encendido
      delay(100);
      digitalWrite(LED_D6, LOW);  // sonalert apagado

      pixels.fill(pixels.Color(255, 0, 0), 0, NUMPIXELS);
      pixels.show();

      cont_amarillo = 0;
    }
  }
  //FIN AGREGADO OLED Y NEOPIXEL

  if (!connect_tic) {
    Serial.println("+");
    Serial.println(datos_set);
    Serial.println("+");
    if(datos_set == "5_m") connect_tic = 300000;
    else if(datos_set == "10_m") connect_tic = 600000;
    else if(datos_set == "15_m") connect_tic = 900000;
    else if(datos_set == "20_m") connect_tic = 1200000;
    else if(datos_set == "30_m") connect_tic = 1800000;
    else if(datos_set == "45_m") connect_tic = 2700000;
    else if(datos_set == "1_h") connect_tic = 3600000;
    else if(datos_set == "2_h") connect_tic = 7200000;
    else if(datos_set == "5_h") connect_tic = 18000000;
    else if(datos_set == "12_h") connect_tic = 43200000;
    else
    {
      datos_set = "5_m";
      connect_tic = 300000;      
    }
    if (WiFi.status() == WL_CONNECTED) connect();
    flg_guardar_dato = 1;
  }
  
  if(flag_guardar) {
    leer(0).toCharArray(ssid, 50);
    leer(50).toCharArray(pass, 50);
    WiFi.mode(WIFI_AP_STA); //para que no inicie el SoftAP en el modo normal
    WiFi.begin(ssid, pass);
    Serial.println("------------------------------------------------");
    Wificonf();
    flag_guardar = 0;
    fin_AP_tic = 300000; // 5 minutos con AP
  }
  
  if(!fin_AP_tic && !flag_fin_AP ) {
    if(Status == WL_CONNECTED)
    {
        WiFi.mode(WIFI_STA); 
        WiFi.begin(ssid, pass);
        flag_fin_AP = 1;
    }else fin_AP_tic = 300000; 
  }

  Status = WiFi.status();

  if (Status != Status_ant)
  {
    Status_ant = Status;
    Serial.println("****************************");     
    if (Status == WL_IDLE_STATUS) status_string = "Estado IDLE";//0
    else if (Status == WL_NO_SSID_AVAIL) status_string = "No encuentra el SSID";//1 
    else if (Status == WL_SCAN_COMPLETED) status_string = "Escaneo completado";//2
    else if (Status == WL_CONNECTED) status_string = "Conectado";//3
    else if (Status == WL_CONNECT_FAILED) status_string = "Error al conectar";//4
    else if (Status == WL_CONNECTION_LOST) status_string = "Conexion perdida";//5
    else if (Status == WL_DISCONNECTED) status_string = "Desconectado";//6
    else status_string = String(Status);
    Serial.println(status_string);   
    Serial.println(ssid);
    Serial.println(pass);      
  }

  if (flg_guardar_dato && RTC_OK == 1) {

    borrar_archivos_viejos();

    FSInfo fsInfo;
    SPIFFS.info(fsInfo);
    Serial.print("FS Total Bytes: ");
    Serial.println(fsInfo.totalBytes);
    Serial.print("FS Used Bytes: ");
    Serial.println(fsInfo.usedBytes);
          
    DateTime now = rtc.now();
    Serial.println(now.timestamp(DateTime::TIMESTAMP_FULL));

    if (fsInfo.usedBytes < MAX_BYTES) {
      File file = SPIFFS.open("/Log_"+ now.timestamp(DateTime::TIMESTAMP_DATE) + "_" + SensorID + ".csv", "a");
  
      if (file) {
        file.print(now.timestamp(DateTime::TIMESTAMP_TIME));
        file.print(';');
  
        if (muestras_CO2[0] == 0)
          file.println("0");
        else if (muestras_CO2[0] < 400)
          file.println("400");
        else
          file.println(muestras_CO2[0]);
        
        Serial.println(file.size());
        file.close();      
      }
      else {
        Serial.println("Fallo al abrir archivo");
      }
    }
    else Serial.println("Memoria llena");

    flg_guardar_dato = 0;
  }

}

void connect() {
  int8_t Temp;

  Serial.print("SensorID:\n");
  Serial.print(SensorID + "\n");

  Serial.print("CO2 (ppm): ");
  Serial.println(muestras_CO2[0]);
  
  fechaUltimoDato = rtc.now();
  
  if (muestras_CO2[0] == 0)
    ultimoCO2 = 0;
  else if (muestras_CO2[0] < 400)
    ultimoCO2 = 400;
  else
    ultimoCO2 = muestras_CO2[0];

  Temp = myMHZ19.getTemperature();  // Request Temperature (as Celsius)
  Serial.print("Temperature (C): ");
  Serial.println(Temp);

  long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssi);

  int quality = dBmtoPercentage(rssi);
  Serial.println(quality);

  /*****************************************Envio al sistema nuevo por metodo POST****************************************/
  respuestaPost = "";
  String url_post = "http://data.ambientecontrolado.com.ar/device/measure";
  String data_post = "{\"data\":{\"extraData\":{\"chipId\":\"" + SensorID + "\",\"co2\":"+ String(ultimoCO2) + ",\"ssid\":\""+ WiFi.SSID() + "\",\"ip\":\""+ WiFi.localIP().toString() +"\",\"signal\":" + String(quality) + ",\"firmwareVersion\":\"" +  firmVer + "\"}}}";

  Serial.println("URL: " + url_post);
  Serial.println("DATA: " + data_post);

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  http_post.begin(client_post, url_post); //HTTP
  http_post.addHeader("Content-Type", "application/json");

  Serial.print("[HTTP] POST...\n");
  // start connection and send HTTP header and body
  int httpCode_post = http_post.POST(data_post);

  // httpCode will be negative on error
  if (httpCode_post > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode_post);

    // file found at server
    if (httpCode_post == HTTP_CODE_OK) {
      const String& payload = http_post.getString();
      Serial.println("received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");
    }
    respuestaPost = String(httpCode_post) + "<br>" + http_post.getString();
  }
  else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http_post.errorToString(httpCode_post).c_str());
  }

  http_post.end();
  
  /*****************************************Envio al local por metodo POST ****************************************/

  respuestaPostLocal= "";
  
  if (IP_Puerto_local != "")
  {
    url_post = "http://" + IP_Puerto_local + "/device/measure";
  
    Serial.println("URL: " + url_post);
    Serial.println("DATA: " + data_post);
  
    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    http_post2.begin(client_post2, url_post); //HTTP
    http_post2.addHeader("Content-Type", "application/json");
  
    Serial.print("[HTTP] POST...\n");
    // start connection and send HTTP header and body
    httpCode_post = http_post2.POST(data_post);
  
    // httpCode will be negative on error
    if (httpCode_post > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode_post);
  
      // file found at server
      if (httpCode_post == HTTP_CODE_OK) {
        const String& payload = http_post2.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
  //    respuestaPostLocal = "";//****************************************************
      respuestaPostLocal = String(httpCode_post) + "<br>" + http_post2.getString();
    }
    else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http_post2.errorToString(httpCode_post).c_str());
    }
  
    http_post2.end();
  }

 /********************************Envio al servidor Calidaddelaireadox por metodo GET**************************************/
  respuestaGet = "";
  
  Serial.print("[HTTP] begin...\n");

  String url = "http://159.203.150.67/calidaddelaireadox/services/Services.php?acc=AD&id=" + SensorID + "&co2=" + String(ultimoCO2) + "&temp=" + String(Temp) + "&bateria=" + String(ESP.getVcc()) + "&wifi=" + String(quality);

  Serial.println("URL: " + url);

  if (http_get.begin(client_get, url))
  {
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode_get = http_get.GET();
    // httpCode will be negative on error
    if (httpCode_get > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode_get);

      if (httpCode_get == HTTP_CODE_OK) {
        const String& payload = http_get.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
      respuestaGet = String(httpCode_get) + "<br>" + http_get.getString();

      http_get.end();
    }
    else {
      Serial.printf("[HTTP] Unable to connect\n");
    }
  }
}

/*
   Written by Ahmad Shamshiri
    with lots of research, this sources was used:
   https://support.randomsolutions.nl/827069-Best-dBm-Values-for-Wifi
   This is approximate percentage calculation of RSSI
   Wifi Signal Strength Calculation
   Written Aug 08, 2019 at 21:45 in Ajax, Ontario, Canada
*/

int dBmtoPercentage(long dBm) {
  int quality;
  if (dBm <= RSSI_MIN) {
    quality = 0;
  } else if (dBm >= RSSI_MAX) {
    quality = 100;
  } else {
    quality = 2 * (dBm + 100);
  }

  return quality;
}  //dBmtoPercentage



void testdrawstyles(void) {
  display.clearDisplay();

    if (muestras_CO2[0] > 999) {
      display.setTextSize(5);  // Normal 1:1 pixel scale
      display.setCursor(5, 0);             // Start at top-left corner
    } else {
      display.setTextSize(5);  // Normal 1:1 pixel scale
      display.setCursor(35, 0);             // Start at top-left corner
    }

    display.setTextColor(SSD1306_WHITE);  // Draw white text

    if (muestras_CO2[0] == 0) {
      display.println(F("---"));
    } else if (muestras_CO2[0] < 400) {
      display.println(F("400"));
    } else {
      display.println(muestras_CO2[0]);
    }

    display.setTextSize(3);  // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);

    if (cont_display == 0) {
      display.println(F("PPM CO2"));
      cont_display = 1;
    } else if (cont_display == 1) {
      if (estado_CO2 == 0) {
        display.println(F("SIN MED"));
      } else if (estado_CO2 == 1) {
        display.println(F("NORMAL"));
      } else if (estado_CO2 == 2) {
        display.println(F("ALTA"));
      } else {
        display.println(F("ALARMA"));
      }
      cont_display = 2;
    } else if (cont_display == 2) {
      if (WiFi.status() == 0) {
        display.println(F("DESC"));
      } else if (WiFi.status() == 3) {
        display.println(F("CONEC"));
      }
      cont_display = 0;
    }

  display.display();
  //  delay(2000);
}

void borrar_archivos_viejos (void) {
  String filename = "";
  String filename_aux = "";
  Dir dir = SPIFFS.openDir("/");
  DateTime now = rtc.now();

  while (dir.next()) {
    filename = dir.fileName();
    filename_aux = filename.substring(4,14) + "T00:00:00";
    DateTime fecha_archivo(filename_aux.c_str());
//    Serial.print(fecha_archivo.timestamp(DateTime::TIMESTAMP_FULL));
    TimeSpan antiguedad_archivo = now - fecha_archivo;
//    Serial.print("  ");
//    Serial.print(antiguedad_archivo.days(), DEC);
//    Serial.print(" days ");
    if (antiguedad_archivo.days() > 30)
    {
      if (SPIFFS.remove(filename))
        Serial.println("El archivo "+ filename + " se borro con exito");
      else
        Serial.println("Error al borrar archivo: " + filename);
    }
//    Serial.println();
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Index() {
    SendHTML_Header();
    webpage += F("<h2>Indice</h2>");
    webpage += F("<h3>Chip ID: ") + SensorID + F("</h3>");
    webpage += F("<h3>Versión ") + firmVer + F("</h3>");
    webpage += F("<a href='/download'>[Download]</a><br><br>");
    webpage += F("<a href='/date'>[Fecha]</a><br><br>");
    webpage += F("<a href='/wifi'>[WiFi]</a><br><br>");
    webpage += F("<a href='/sensed'>[Configuración de envío]</a><br><br>");
    webpage += F("<a href='/ultimodato'>[Último dato enviado]</a><br><br>");
    webpage += F("</body></html>");
    SendHTML_Content();
    SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DatoEnviado() {
    SendHTML_Header();
    if(Status == WL_CONNECTED)
    {
      connect_tic = 0;
      webpage += F("<h2>Dato enviado con exito.</h2>");
    }
    else 
    {
      connect_tic = 0;          
      webpage += F("<h2>No hay conexión, dato guardado.</h2>");
    }
    
    webpage += F("<a href='/ultimodato'>[Back]</a><br><br>");

    webpage += F("</body></html>");
    SendHTML_Content();
    SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void UltimoDatoEnviado() {
  
    SendHTML_Header();
    
    webpage += F("<h3>");
    webpage += F("El dato enviado fue: <br> CO2: ") + String(ultimoCO2) + F(" ppm");
    webpage += F("<br>Fecha: ") + fechaUltimoDato.timestamp(DateTime::TIMESTAMP_DATE) + F(" Hora: ") + fechaUltimoDato.timestamp(DateTime::TIMESTAMP_TIME);
    webpage += F("</h3>");
    webpage += F("<p>");
    webpage += F("Respuesta Servidor Post:<br>") + respuestaPost;
    webpage += F("<br><br>Respuesta Servidor Local Post:\n<br>") + respuestaPostLocal;
    webpage += F("<br><br>Respuesta Servidor Get:<br>") + respuestaGet;
    webpage += F("</p>");

    
    webpage += F("<a href='/envio'>[Enviar un nuevo dato]</a><br><br>");
    
    webpage += F("<a href='/'>[Back]</a><br><br>");

    webpage += F("</body></html>");
    SendHTML_Content();
    SendHTML_Stop();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Download() { // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  if (WebServer.args() > 0 ) { // Arguments were received
    if (WebServer.hasArg("download")) SD_file_download(WebServer.arg(0));
    if (WebServer.hasArg("borrar")) SD_file_borrar(WebServer.arg(0));
  }
  else SelectInput("Descarga de Archivos", "Presione \"descargar\" para bajar el archivo o \"borrar\" para borrarlo", "download", "download");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Config_Sensed() { // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  if (WebServer.args() > 0 ) { // Arguments were received
    if (WebServer.hasArg("Actualizar")) Set_Sensado(WebServer.arg(0));
    else if (WebServer.hasArg("Guardar")) SetDatosEnvio(WebServer.arg(0));
  }
  else {    
    SendHTML_Header();

    webpage += F("<FORM action='/sensed'>"); // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
    webpage += F("<h1>Configuracion de envio al servidor:</h1>");
    webpage += F("<p>http://XXX.XXX.X.XXX:XXXX/device/measure</p>");
    webpage += F("<p>IP_Puerto_local: ");
    webpage += F("<input type='text' name='IP_Puerto_local' value='") + IP_Puerto_local + F("'><br></p>");    
    webpage += F("<input type='submit' name='Guardar' value='Guardar'><br><br>");
    webpage += F("</FORM>");


    webpage += F("<FORM action='/sensed'>"); // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
    webpage += F("<h1>Seleccione el intervalo de sensado:</h1>");
    webpage += F("<p>(Actualmente se esta sensando cada ");
    if(datos_set == "5_m") webpage += F("5 minutos)</p>");
    else if(datos_set == "10_m") webpage += F("10 minutos)</p>");
    else if(datos_set == "15_m") webpage += F("15 minutos)</p>");
    else if(datos_set == "20_m") webpage += F("20 minutos)</p>");
    else if(datos_set == "30_m") webpage += F("30 minutos)</p>");
    else if(datos_set == "45_m") webpage += F("45 minutos)</p>");
    else if(datos_set == "1_h") webpage += F("1 hora)</p>");
    else if(datos_set == "2_h") webpage += F("2 horas)</p>");
    else if(datos_set == "5_h") webpage += F("5 horas)</p>");
    webpage += F("<select name='Sensado' id='Sensado'>");
    webpage += F("<option value='5_m'>5 min</option>");
    webpage += F("<option value='10_m'>10 min</option>");
    webpage += F("<option value='15_m'>15 min</option>");
    webpage += F("<option value='20_m'>20 min</option>");
    webpage += F("<option value='30_m'>30 min</option>");
    webpage += F("<option value='45_m'>45 min</option>");
    webpage += F("<option value='1_h'>1 hora</option>");
    webpage += F("<option value='2_h'>2 horas</option>");
    webpage += F("<option value='5_h'>5 horas</option>");
    webpage += F("</select><br>");
    webpage += F("<input type='submit' id='Actualizar' name='Actualizar' value='Actualizar'><br><br>");
    webpage += F("</FORM>");

    webpage += F("<a href='/'>[Back]</a><br><br>");
    webpage += F("</body></html>");

    SendHTML_Content();
    SendHTML_Stop();
    
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Config_Date() { // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  if (WebServer.args() > 0 ) { // Arguments were received
    if (WebServer.hasArg("currentDateTime")) Set_Date_Time(WebServer.arg(0));
  }
  else {    
    SendHTML_Header();
    webpage += F("<h2>Ajustar Fecha y Hora</h2>");

    DateTime now = rtc.now();
    webpage += F("<p>Fecha y Hora actual: ") + now.timestamp(DateTime::TIMESTAMP_DATE) + F("&nbsp;&nbsp;&nbsp;&nbsp;") + now.timestamp(DateTime::TIMESTAMP_TIME) + F("</p>");
    webpage += F("<p>Seleccione nueva Fecha y Hora: </p>");
    webpage += F("<FORM action='/date'>"); // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
    webpage += F("<input type='datetime-local' id='currentDateTime' name='currentDateTime'>");
    webpage += F("<input type='submit'><br>");
    webpage += F("<a href='/'>[Back]</a><br><br>");

    webpage += F("</FORM></body></html>");

    SendHTML_Content();
    SendHTML_Stop();
    
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_download(String filename) {
  File download = SPIFFS.open("/" + filename, "r");
  if (download) {
    WebServer.sendHeader("Content-Type", "text/text");
    WebServer.sendHeader("Content-Disposition", "attachment; filename=" + filename);
    WebServer.sendHeader("Connection", "close");
    WebServer.streamFile(download, "application/octet-stream");
    download.close();
  } else ReportFileNotPresent("download");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_borrar(String filename) {
    SendHTML_Header();
    if (SPIFFS.remove("/" + filename))
      webpage += F("<h3>El archivo se borro con exito</h3>");
    else
      webpage += F("<h3>Error al borrar archivo</h3>");
    webpage += F("<a href='/download'>[Back]</a><br><br>");
    webpage += F("</body></html>");
    SendHTML_Content();
    SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Set_Date_Time(const String currentDateTime) {
    rtc.adjust(DateTime(currentDateTime.c_str()));
//    rtc.adjust(DateTime("2021-11-29T13:45"));
    SendHTML_Header();
    webpage += F("<h3>Fecha y hora actulizada</h3>");
    webpage += F("<a href='/date'>[Back]</a><br><br>");
    webpage += F("</body></html>");
    SendHTML_Content();
    SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Set_Sensado(const String sensado_value) {
    grabar(100,sensado_value);
    datos_set = leer(100);
//    datos_tic = 0;
    SendHTML_Header();
    webpage += F("<h1>Intervalo de sensado actualizado</h1>");
    webpage += F("<a href='/sensed'>[Back]</a><br><br>");
    webpage += F("</body></html>");
    SendHTML_Content();
    SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SetDatosEnvio(const String dato1) {
    grabar(150,dato1);
    IP_Puerto_local = leer(150);

    SendHTML_Header();
    webpage += F("<h1>Datos de envío guardado</h1>");
    webpage += F("<a href='/sensed'>[Back]</a><br><br>");
    webpage += F("</body></html>");
    SendHTML_Content();
    SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header() {
  WebServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  WebServer.sendHeader("Pragma", "no-cache");
  WebServer.sendHeader("Expires", "-1");
  WebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  WebServer.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  append_page_header();
  WebServer.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content() {
  WebServer.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop() {
  WebServer.sendContent("");
  WebServer.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String heading2, String command, String arg_calling_name) {
  SendHTML_Header();
  webpage += F("<h3 class='rcorners_m'>"); webpage += heading1 + "</h3>";

  webpage += F("<h3>   Nombre  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  Tamaño   </h3>");
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!

  String filename = "";
  Dir dir = SPIFFS.openDir("/");
  long usedbytes=0;
  while (dir.next()) {
    usedbytes += dir.fileSize();
    filename = dir.fileName();
//    filename.remove(0,1);
    webpage += F("<p>"); webpage += dir.fileName() + F("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;") + dir.fileSize() + F(" bytes &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
    webpage += F("<button type='submit' name='download' value='") + filename + F("'>descargar</button> ");
    webpage += F("<button type='submit' name='borrar' value='") + filename + F("'>borrar</button> </p>");    
  }
  
  webpage += F("<h3>"); webpage += heading2 + "</h3>";
  
  FSInfo fsInfo;
  SPIFFS.info(fsInfo);
  Serial.println(fsInfo.totalBytes);
  webpage += F("<p>Bytes usados: "); webpage += String(usedbytes) + F(" / ")+ String(fsInfo.totalBytes) + F("</p>");

  if (fsInfo.usedBytes >= MAX_BYTES)
      webpage += F("<h3 style='color:Tomato;'>Memoria llena, por favor borre archivos para seguir grabando información</h3>");

//  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
//  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
//  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
    webpage += F("<br><a href='/'>[Back]</a><br><br>");
    
  webpage += F("</FORM></body></html>");
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target) {
  SendHTML_Header();
  webpage += F("<h3>El archivo no existe</h3>");
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  webpage += F("</body></html>");
  SendHTML_Content();
  SendHTML_Stop();
}

void append_page_header() {
  webpage  = F("<!DOCTYPE html><html>");
  webpage += F("<head>");
  webpage += F("<title>File Server</title>"); // NOTE: 1em = 16px
  webpage += F("<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>");
  webpage += F("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>");
  webpage += F("<style>");
  webpage += F("</style></head><body>");
}
