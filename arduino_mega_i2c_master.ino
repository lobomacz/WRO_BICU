#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <WirePacker.h>
#include <WireSlaveRequest.h>

// Etiquetas de valores
#define R_GRAD 57.296
#define PIN_SDA 20
#define PIN_SCL 21
#define ESP32_ADDR 0x07
#define COORD_LENGTH 54
#define NUM_GROUPS 4
#define NUM_COORDS 6
#define DATA_LENGTH 250

/*
 * LISTA DE COMANDOS NUMÉRICOS
 * 
 * 0 -> coordenadas de ronda eliminatoria
 * 1 -> coordenadas de ronda final
 * 2 -> detectar color
 * 3 -> rastrear color
 * 
 */

// Variables temporales de datos de comandos I2C
// no son útiles en ejecución real
char orden;
String dato = "";

// Variables del sistema de navegación
int grupo_actual;             // Indice del grupo de coordenadas que se está usando
int coord_anterior;           // Indice de donde se obtuvo el punto origen
int coord_actual;             // Indice de donde se obtuvo el punto destino
byte origenX;                 // X del punto origen
byte origenY;                 // Y del punto origen
byte destinoX;                // X del punto destino
byte destinoY;                // Y del punto destino
int vector_actual[2];         // Componentes del vector actual
int vector_destino[2];        // Componentes del vector destino
double m_vector_actual;        // Magnitud(módulo) del vector de origen (distancia)
double m_vector_destino;       // Magnitud(módulo) del vector de destino (distancia)
double direccion_destino;      // Dirección del nuevo vector hacia el punto destino
double angulo_giro;           // Angulo entre el nuevo vector y el vector actual
char coordenadas[4][6][4];    // Tabla de coordenadas
int iazul = 0, irojo = 0, inaranga = 0, iverde = 0, iamarillo = 0;  // Indice de coordenadas de colores detectados
int grupos_falsos[4] = {-1,-1,-1,-1};   // Lista de indices de grupos de coordenadas falsos para uso en la ronda final



// Si estamos en la ronde final, cambiar el valor y volver a cargar el sketch
bool ronda_final = false;

/*
 * PROTOTIPOS DE LAS FUNCIONES PERSONALIZADAS
 */
 
void splitQR(String code, char delimiter);

void calculaSalida();

void calculaVector(byte x1, byte y1, byte x2, byte y2, bool salida);

double calculaDireccion();

double calculaModulo();

double calculaAngulo();

void trazaRuta();

void setup() {
  Wire.begin();
  Serial.begin(9600);
}

void loop() {

  if(Serial.available()>0){
    WirePacker packer;
    orden = Serial.read();
    Serial.println("Transmision iniciada...");
    packer.write(orden);
    packer.end();
    Wire.beginTransmission(ESP32_ADDR);
    while(packer.available()){
      Wire.write(packer.read());
    }  
    Wire.endTransmission();
    Serial.print("Se envio la orden ");
    Serial.println(orden);
    delay(2000);                       
    // Incrementar tiempo de espera entre la orden y la solicitud 
    // en caso de no recibir datos
    
    WireSlaveRequest slaveReq(Wire, ESP32_ADDR, DATA_LENGTH);
    //slaveReq.setRetryDelay(5);

    bool exito = slaveReq.request();

    if(exito){
      dato = "";                // Vaciamos la variable de cualquier dato anterior;
      // De acuerdo a la orden enviada, se procesará
      // el dato QR de manera distinta
      if(orden == '0'){
        grupo_actual = 0;
        while(slaveReq.available()>0){
          char i = slaveReq.read();
          dato.concat(i);
        }
        splitQR(dato,')');
      } else if (orden == '1'){
        for(int i=0;i<4;i++){
          grupo_actual = i;
          int j = 0;
          while(j<COORD_LENGTH){
            char i = slaveReq.read();
            dato.concat(i);
          }  
          splitQR(dato,')');
          if(dato.length()>0){
            Serial.print("Respuesta >> ");
            Serial.println(dato);
            Serial.println("===========================");
            dato = "";
          }
        }
      } else {
        while(slaveReq.available()>0){
          char i = slaveReq.read();
          dato.concat(i);
        }
      }

      if(dato.length()>0){
        Serial.print("Respuesta >> ");
        Serial.println(dato);
        Serial.println("===========================");
        dato = "";
      }
      
    } else {
      Serial.println("Error en la solicitud");
      Serial.println(slaveReq.lastStatusToString());  
    }
  }

  delay(300);

}

/*
 * FUNCIONES PERSONALIZADAS
 */

// Calcula los parámetros para la nueva ruta del robot
void trazaRuta(){
  

  
  vector_actual[0]  
}

double calculaAngulo(){
  return pow(cos((vector_actual[0]*vector_destino[0]+vector_actual[1]*vector_destino[1])/(m_vector_actual*m_vector_destino)),-1) * R_GRAD;
}

double calculaModulo(){
  return sqrt(sq(vector_destino[0])+sq(vector_destino[1]));  
}

double calculaDireccion(){
  return atan(fabs(vector_destino[0])/fabs(vector_destino[1])) * R_GRAD;
}

// Calcula el primer vector al salir de la estación de parqueo
// Si está en ronda final, se selecciona un grupo al azar.
// Si es ronda de clasificación, se elige el primer grupo.
void calculaSalida(){
    if(ronda_final){
      coord_actual = random(4);
    } else {
      coord_actual = 0;
    }
    origenX = coordenadas[coord_actual][0][3];
    origenY = coordenadas[coord_actual][0][2];
    destinoX = coordenadas[coord_actual][0][1];
    destinoY = coordenadas[coord_actual][0][0]; 

    calculaVector(origenX, origenY, destinoX, destinoY, true);
}

// Función que calcula el módulo del vector destino
// los parámetros son los valores xy de cada punto
// del vector.
void calculaVector(byte x1, byte y1, byte x2, byte y2, bool salida){
    if(salida){
      vector_actual[0] = x2-x1;
      vector_actual[1] = y2-y1;  
    } else {
      vector_destino[0] = x2-x1;
      vector_destino[1] = y2-y1;  
    }
}

// Función que divide el string de coordenadas y 
// guarda los valores en la tabla de coordenadas
void splitQR(String code, char delimiter){

  String tabla_coords[NUM_COORDS];
  int inicio = 0;
  int alto;
  int indice = 0;

  while(indice < NUM_COORDS){

    alto = code.indexOf(delimiter, inicio);
    String coord = code.substring(inicio+1, alto);
    tabla_coords[indice] = coord;
    inicio = alto + 1;
    indice++;
    
    }
  int ultima_coord = 0;
  for(int i=0;i<NUM_COORDS;i++){
      char c[7];
      tabla_coords[i].toCharArray(c,7);
      int val = 0;
      for(int j=0;j<7;j++){
        if(c[j] == ','){
          continue;  
        }else{
            coordenadas[grupo_actual][ultima_coord][val] = c[j];
            val++;
        }  
      }
      ultima_coord++;
  }
}
