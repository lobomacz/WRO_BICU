#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <WirePacker.h>
#include <WireSlaveRequest.h>

// Etiquetas de valores
#define R_GRAD 57.296
#define CENTRO 10
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
 * 0 -> leer coordenadas
 * 1 -> detectar color
 * 
 */

// Variables temporales de datos de comandos I2C
// no son útiles en ejecución real
char orden;
String dato = "";
// Variables de detección/reconocimiento de colores
String colores = "rojo,verde,amarillo,naranja";
String color_buscar = "azul";

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
double m_vector_actual;       // Magnitud(módulo) del vector de origen (distancia)
double m_vector_destino;      // Magnitud(módulo) del vector de destino (distancia)
double direccion_destino;     // Dirección del nuevo vector hacia el punto destino
double angulo_giro;           // Angulo entre el nuevo vector y el vector actual
char coordenadas[4][6][4];    // Tabla de coordenadas
String icolores[6] = {"NA","NA","NA","NA","NA","NA"};               // Lista para registrar colores detectados por índice
bool detectados[6] = {true,false,false,false,false,false};          // Lista de indices con colores reconocidos
bool despachados[6] = {true,false,false,false,false,false};         // Lista de puntos que ya se despacharon/no se visitarán
bool grupos_falsos[4] = {false,false,false,false};                  // Lista de indices de grupos de coordenadas falsos para uso en la ronda final


// Si estamos en la ronde final, cambiar el valor a true y volver a cargar el sketch
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

int buscaIndice(bool gr);

void corregirRuta();

void detectarColor();

void reconocerColor();

void solicitarDatos();

void setup() {
  Wire.begin();
  Serial.begin(9600);
}

void loop() {

  delay(300);

}

/*
 * FUNCIONES PERSONALIZADAS
 */

// Función que se llama para detectar el color del cubo
// tomado de la parte superior de la caja. 
void detectarColor(){
  orden = '0';
  do{
    solicitarDatos();  
  }while(colores.indexOf(dato) == -1);
  color_buscar = dato;
}

// Función que se llama al aproximarse a una caja para 
// verificar si es la caja que se busca.
void reconocerColor(){
  orden = '0';
  do{
    solicitarDatos();  
  }while(colores.indexOf(dato) == -1);
  
  if(dato == color_buscar){
    // Realizar acción correspondiente a la caja correcta.
    // Sustituir el cubo en la parte superior y reconocer el 
    // color del nuevo cubo para calcular la nueva ruta.

    // Ponemos la coordenada actual como despachada.
    despachados[coord_actual] = true;
  } else {
    // Si la caja no es del color que buscamos, guardamos 
    // la referencia en caso de necesitarla posteriormente
    detectados[coord_actual] = true;      // Pone true en el índice de la coordenada actual de la lista de detectados
    icolores[coord_actual] = dato;        // Pone el valor de dato(color) en la lista de índices de colores
  }

  trazaRuta();                            // Una vez reconocido el color de la caja, se traza la nueva ruta.
}

// Función que solicita un dato de la ESP32-CAM y lo
// recupera en la variable.
void solicitarDatos(){
    WirePacker packer;
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

    // Damos un tiempo de espera considerable entre el envío
    // de la orden y el envío de la solicitud del dato.
    if(orden == '0'){
      delay(5000);
    } else {
      delay(2000);      
    }
    
    WireSlaveRequest slaveReq(Wire, ESP32_ADDR, DATA_LENGTH);
    //slaveReq.setRetryDelay(5);

    bool exito = slaveReq.request();

    if(exito){
      dato = "";          // Vaciamos la variable de cualquier dato anterior;
      
      // De acuerdo la ronda, se procesará
      // el dato QR de manera distinta
      if(!ronda_final && orden == '0'){
        grupo_actual = 0;
        while(slaveReq.available()>0){
          char i = slaveReq.read();
          dato.concat(i);
        }
        splitQR(dato,')');
      } else if (ronda_final && orden == '0'){
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



// Esta función se ejecuta sólo si se está en la ronda final
// Pone el grupo actual en la lista de falsos y seleccioona un
// nuevo grupo de coordenadas para calcular una nueva ruta
// hacia un nuevo punto del nuevo grupo.
void corregirRuta(){
  if(ronda_final){
    grupos_falsos[grupo_actual] = true;
    grupo_actual = buscaIndice(true);
    trazaRuta();
  }  
}

// Busca eleatoriamente un índice de grupo que no sea falso
// o un índice de coordenada que no haya sido detectado
// ni que haya sido despachado para calcular el siguiente destino.
int buscaIndice(bool gr){
  int indice;
  if(gr){
    do{
     indice = random(4);
    }while(grupos_falsos[indice]==true);
  } else {
    do{
     indice = random(1,6);
    }while(detectados[indice]==true || despachados[indice]==true);
  }
  
  return indice;
}

// Calcula los parámetros para la nueva ruta del robot
void trazaRuta(){
  // El punto destino pasa a ser el punto actual.
  coord_anterior = coord_actual;
  origenX = destinoX;
  origenY = destinoY;
  vector_actual[0] = vector_destino[0];
  vector_actual[1] = vector_destino[1];
  m_vector_actual = m_vector_destino;

  // Calculamos el punto medio del segmento a partir de las coordenadas
  int p1[2];
  int p2[2];
  int m[2];                 // Coordenadas del punto medio de la caja

  int indice = -1;
  if(color_buscar != "azul"){
    // Revisamos si se conoce el índice del el color que se busca
    for(int i = 1;i<6;i++){
      if(icolores[i] == color_buscar){
        indice = i;
      }
    }
  } else {
    // Obtenemos un índice aleatorio entre los que no se han despachado 
    // y no corresponden al color buscado.
    indice = buscaIndice(false);
  }

  if(indice >=0){
    // Recuperamos las coordenadas X,Y para los dos puntos de la caja
    p1[0] = coordenadas[grupo_actual][indice][1];
    p1[1] = coordenadas[grupo_actual][indice][0];
    p2[0] = coordenadas[grupo_actual][indice][3];
    p2[1] = coordenadas[grupo_actual][indice][2];

    // Calculamos el punto medio de la caja
    m[0] = int((p1[0]+p2[0])/2);
    m[1] = int((p1[1]+p2[1])/2);

    // Determinamos el eje y sentido a desplazar el punto central de la caja
    int zx = CENTRO - m[0];
    int zy = CENTRO - m[1];
    zx = abs(zx);
    zy = abs(zy);

    if(zx > zy){
        if(m[0] > CENTRO){
          m[0] -= 2;
        } else {
          m[0] += 2;  
        }
    } else {
      if(m[1] > CENTRO){
        m[1] -= 2;
      } else {
        m[1] += 2;  
      }
    }

    // Obtenemos las coordenadas X,Y del punto destino
    destinoX = m[0];
    destinoY = m[1];
    // Calculamos los parámetros de navegación: vector, distancia, dirección y ángulo
    calculaVector(origenX,origenY,destinoX,destinoY,false);
    m_vector_destino = calculaModulo();
    direccion_destino = calculaDireccion();
    angulo_giro = calculaAngulo();
  }
}

double calculaAngulo(){
  return acos((vector_actual[0]*vector_destino[0]+vector_actual[1]*vector_destino[1])/(m_vector_actual*m_vector_destino)) * R_GRAD;
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
