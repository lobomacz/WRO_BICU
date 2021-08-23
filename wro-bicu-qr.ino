/****************************************************************
 *  __          _______   ____         ____ _____ _____ _    _  *
 *  \ \        / /  __ \ / __ \       |  _ \_   _/ ____| |  | | *
 *   \ \  /\  / /| |__) | |  | |______| |_) || || |    | |  | | *
 *    \ \/  \/ / |  _  /| |  | |______|  _ < | || |    | |  | | *
 *     \  /\  /  | | \ \| |__| |      | |_) || || |____| |__| | *
 *      \/  \/   |_|  \_\\____/       |____/_____\_____|\____/  *
 *                                                              *
 ****************************************************************/

/*
 * Sketch para reconocimiento y lectura de código QR
 * para la estracción de las coordenadas de las cajas
 * y estación de parqueo del reto WRO Tetrix 2020
 * 
 */

#include <Arduino.h>
#include <esp32cam.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <uri/UriBraces.h>
#include <Wire.h>
#include <WireSlave.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Etiquetas de valores
#define PIN_FLASH 4
#define PIN_SDA 14
#define PIN_SCL 15
#define I2C_SLAVE_ADDR 0x07
#define ERROR_LED 33
#define DATA_LENGTH 250


/*
 * LISTA DE COMANDOS/ORDENES NUMÉRICOS
 * 
 * 0 -> coordenadas de ronda eliminatoria
 * 1 -> coordenadas de ronda final
 * 2 -> detectar color
 * 3 -> rastrear color
 * 
 */


// Credenciales del AP
const char* ssid = "WRO-BICU";
const char* password = "R2D2#C3PO";

// Variables Globales
String modo = "idle";         // Modos: idle,qr,detect,track
char orden;                   // Variable para la orden del controlador
String codigos;
char val[DATA_LENGTH];                // Arreglo de caracteres para enviar datos al controlador


// Iniciamos el servidor web en el puerto 80
WebServer servidor(80);

// Establecemos alta resolución para la imagen
static auto hiRes = esp32cam::Resolution::find(800,600);
//static auto hiRes = esp32cam::Resolution::find(1600,1200);      // Para la máxima resolución

void capturaJpg();

void getCodigoQR(String codigo);

void getOrden(int numBytes);

void procesar();

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // Configuramos el pin del flash y led de error como OUTPUT
  pinMode(PIN_FLASH, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);
  // Iniciamos el monitor serial para ver datos de la ESP32
  Serial.begin(115200);

  // Iniciamos la interfaz I2C
  bool res = WireSlave.begin(PIN_SDA,PIN_SCL,I2C_SLAVE_ADDR);

  if(!res){
    digitalWrite(ERROR_LED, LOW);
    while(1){
      delay(100);  
    }
  } else {
    digitalWrite(ERROR_LED, HIGH);  
  }

  WireSlave.onReceive(getOrden);
  WireSlave.onRequest(procesar);

  // Configuramos la cámara con los parámetros 
  // para pines, resolución, no. de búfers y calidad
  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(hiRes);
    cfg.setBufferCount(2);
    cfg.setJpeg(80);                

    bool ok = Camera.begin(cfg);
    Serial.println(ok ? "CAMARA OK" : "FALLA EN LA CAMARA");
  }
  
  Serial.println("Inicializando AP (Access Point)...");

  // Activamos el AP, cerrado con las credenciales de seguridad
  WiFi.softAP(ssid, password);

  // Recuperamos la dirección IP del AP
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Direccion IP del AP: ");
  Serial.println(IP);


  // Definimos las rutas a ser atendidas por el servidor
  servidor.on("/imagen/", capturaJpg);
  servidor.on("/imagen", capturaJpg);

  servidor.on(UriBraces("/codigo-qr/{}"), [](){
    String codigo = servidor.pathArg(0);
    getCodigoQR(codigo);
    });

  // Ponemos en funcionamiento el servidor
  servidor.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  servidor.handleClient();
  WireSlave.update();
  
  delay(2);
}

void getOrden(int numBytes){
  while(WireSlave.available()>0){
    orden = WireSlave.read();
  }  

  switch(orden){
    case '0':
    case '1':
      // Activamos el flash
      digitalWrite(PIN_FLASH, HIGH);
      modo = "qr";
      break;
    case '2':
      break;
  }
}

void procesar(){
  switch(orden){
    case '0':
    case '1':
      if(codigos.length() > 0){
        WireSlave.write(val);
      }
      break;
  }
}

void getCodigoQR(String codigo){
  servidor.send(200,"text/plain", "Codigos recibidos"); // Enviamos respuesta para cambiar el estado del script
  modo = "idle";
  digitalWrite(PIN_FLASH, LOW);                             // Apagamos el flash
  codigos = codigo;                                         // Ponemos los códigos en la variable global
  codigos.toCharArray(val,DATA_LENGTH);
  Serial.println(codigo);
}

void capturaJpg(){
  
  // Se hace la captura de la imagen
  auto frame = esp32cam::capture();
  // Verificamos que la captura sea exitosa
  if(frame == nullptr){
    Serial.println("ERROR DE CAPTURA");
    servidor.send(503, "", "");  
    return;
  }  
  // Si la captura es exitosa, lo mostramos en el monitor y 
  // apagamos el flash
  Serial.printf("IMAGEN CAPTADA %dx%d %db\n", frame->getWidth(), frame->getHeight(), static_cast<int>(frame->size()));
  // Preparamos la respuesta del servidor
  servidor.setContentLength(frame->size());
  servidor.send(200, "image/jpeg");
  WiFiClient cliente = servidor.client();
  frame->writeTo(cliente);                // Envía la imagen al cliente
}
