
#include <AccelStepper.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define PUL_PIN    2
#define DIR_PIN    3
#define BTN_START  4
#define BTN_DIR    5
#define BTN_PASO_P 6
#define BTN_PASO_N 7

const long PASOS_MOTOR = 1600;
float relacion = 12.0;

AccelStepper motor(AccelStepper::DRIVER, PUL_PIN, DIR_PIN);

bool marcha = false;
bool continuo = false;
bool moviendo = false;

int sentido = 1;
float velocidad = 800;
long pasoSize = 1;

unsigned long ultimoBoton = 0;

// =================================
// SECUENCIAS AUTOMATICAS
// =================================

const int MAX_MOVIMIENTOS = 20;

struct Movimiento {
  float angulo;
  float velocidad;
};

Movimiento secuencia[MAX_MOVIMIENTOS];

int totalMovimientos = 0;
int indiceSecuencia = 0;

bool secuenciaActiva = false;
bool esperando = false;

unsigned long esperaSecuencia = 0;
const unsigned long PAUSA = 500;

// Lectura del monitor serie
char bufferSerie[200];
int indiceBuffer = 0;

// =================================
// CONVERSIONES
// =================================

long gradosAPasos(float g) {
  return (long)round(g / 360.0 * PASOS_MOTOR * relacion);
}

float pasosAGrados(long p) {
  return (float)p * 360.0 / (PASOS_MOTOR * relacion);
}

// =================================
// AYUDA
// =================================

void ayuda() {

  Serial.println(F("\n===== COMANDOS ARDUINO ====="));

  Serial.println(F("s       -> arrancar / parar"));
  Serial.println(F("d       -> cambiar sentido"));
  Serial.println(F("v500    -> velocidad manual"));

  Serial.println(F("\n===== PASO A PASO ====="));

  Serial.println(F("n10     -> tamano paso en pasos"));
  Serial.println(F("a0.5    -> tamano paso en grados"));
  Serial.println(F("+       -> paso adelante"));
  Serial.println(F("-       -> paso atras"));
  Serial.println(F("p100    -> mover 100 pasos"));
  Serial.println(F("g90     -> mover 90 grados"));

  Serial.println(F("\n===== SECUENCIA AUTOMATICA ====="));

  Serial.println(F("q30,200,-30,200,90,800,-90,800"));
  Serial.println(F("Formato: qangulo,velocidad,angulo,velocidad..."));
  Serial.println(F("Maximo 20 movimientos"));
  Serial.println(F("Angulos permitidos: -180 a +180"));
  Serial.println(F("Velocidad: 10 a 3000"));

  Serial.println(F("\n===== OTROS ====="));

  Serial.println(F("r12     -> relacion reduccion"));
  Serial.println(F("0       -> establecer cero"));
  Serial.println(F("i       -> consultar estado"));
  Serial.println(F("?       -> ayuda"));

  Serial.println(F("============================\n"));
}

// =================================
// ESTADO
// =================================

void estado() {

  Serial.print(F("Velocidad: "));
  Serial.println(velocidad);

  Serial.print(F("Relacion: "));
  Serial.println(relacion);

  Serial.print(F("Posicion: "));
  Serial.print(pasosAGrados(motor.currentPosition()), 2);
  Serial.println(F(" grados"));

  Serial.print(F("Paso seleccionado: "));
  Serial.print(pasoSize);
  Serial.print(F(" pasos = "));
  Serial.println(pasosAGrados(pasoSize), 4);

  Serial.print(F("Secuencia activa: "));
  Serial.println(secuenciaActiva ? F("SI") : F("NO"));
}

// =================================
// PARAR
// =================================

void parar() {

  secuenciaActiva = false;
  esperando = false;

  if (continuo) {

    marcha = false;
    continuo = false;

    motor.setCurrentPosition(motor.currentPosition());

    Serial.println(F("PARADO"));

  } else if (moviendo) {

    motor.stop();

    Serial.println(F("Frenando..."));

  } else {

    Serial.println(F("Motor parado"));
  }
}

// =================================
// GIRO CONTINUO
// =================================

void arrancar() {

  secuenciaActiva = false;
  esperando = false;

  motor.setCurrentPosition(motor.currentPosition());

  moviendo = false;
  continuo = true;
  marcha = true;

  motor.setMaxSpeed(velocidad);
  motor.setSpeed(sentido * velocidad);

  Serial.println(F("MARCHA CONTINUA"));
}

// =================================
// MOVIMIENTO POR PASOS
// =================================

void moverPasos(long pasos) {

  secuenciaActiva = false;
  esperando = false;

  if (continuo || moviendo) {

    continuo = false;
    marcha = false;

    motor.setCurrentPosition(motor.currentPosition());
    moviendo = false;
  }

  if (pasos == 0) return;

  motor.setMaxSpeed(velocidad);
  motor.setAcceleration(800);

  motor.move(pasos);

  moviendo = true;
}

// =================================
// SIGUIENTE MOVIMIENTO
// =================================

void siguienteMovimiento() {

  if (indiceSecuencia >= totalMovimientos) {

    secuenciaActiva = false;
    moviendo = false;
    esperando = false;

    Serial.println(F("\n===== SECUENCIA TERMINADA ====="));

    return;
  }

  float angulo = secuencia[indiceSecuencia].angulo;

  velocidad = secuencia[indiceSecuencia].velocidad;

  motor.setMaxSpeed(velocidad);
  motor.setAcceleration(800);

  long pasos = gradosAPasos(angulo);

  motor.move(pasos);

  moviendo = true;
  esperando = false;

  Serial.print(F("Movimiento "));
  Serial.print(indiceSecuencia + 1);

  Serial.print(F(" | Angulo: "));
  Serial.print(angulo);

  Serial.print(F(" | Velocidad: "));
  Serial.println(velocidad);

  indiceSecuencia++;
}

// =================================
// INICIAR SECUENCIA
// =================================

void iniciarSecuencia() {

  if (totalMovimientos == 0) {
    Serial.println(F("No hay movimientos"));
    return;
  }

  // Cancelar cualquier movimiento anterior
  motor.setCurrentPosition(motor.currentPosition());

  marcha = false;
  continuo = false;
  moviendo = false;

  indiceSecuencia = 0;
  secuenciaActiva = true;
  esperando = false;

  Serial.println(F("\n===== INICIANDO SECUENCIA ====="));

  siguienteMovimiento();
}

// =================================
// LEER NUMERO
// =================================

bool leerNumero(char *texto, float &numero) {

  while (isspace(*texto)) texto++;

  if (*texto == '\0') return false;

  char *final;

  double valor = strtod(texto, &final);

  while (isspace(*final)) final++;

  if (*final != '\0') return false;

  numero = (float)valor;

  return true;
}

// =================================
// INTERPRETAR SECUENCIA CON COMAS
// =================================

void leerSecuencia(char *texto) {

  Movimiento temporal[MAX_MOVIMIENTOS];

  int cantidad = 0;

  char *token = strtok(texto, ",");

  while (token != NULL) {

    float angulo;

    if (!leerNumero(token, angulo)) {
      Serial.println(F("Error: angulo incorrecto"));
      return;
    }

    token = strtok(NULL, ",");

    if (token == NULL) {
      Serial.println(F("Error: falta velocidad"));
      return;
    }

    float vel;

    if (!leerNumero(token, vel)) {
      Serial.println(F("Error: velocidad incorrecta"));
      return;
    }

    if (angulo < -180 || angulo > 180) {
      Serial.println(F("Error: angulo maximo +/-180"));
      return;
    }

    if (vel < 10 || vel > 3000) {
      Serial.println(F("Error: velocidad entre 10 y 3000"));
      return;
    }

    if (cantidad >= MAX_MOVIMIENTOS) {
      Serial.println(F("Error: maximo 20 movimientos"));
      return;
    }

    temporal[cantidad].angulo = angulo;
    temporal[cantidad].velocidad = vel;

    cantidad++;

    token = strtok(NULL, ",");
  }

  if (cantidad == 0) {
    Serial.println(F("Secuencia vacia"));
    return;
  }

  // Copiar secuencia validada

  for (int i = 0; i < cantidad; i++) {
    secuencia[i] = temporal[i];
  }

  totalMovimientos = cantidad;

  Serial.print(F("Secuencia cargada: "));
  Serial.print(totalMovimientos);
  Serial.println(F(" movimientos"));

  iniciarSecuencia();
}

// =================================
// PROCESAR COMANDOS SERIE
// =================================

void procesarComando(char *linea) {

  while (isspace(*linea)) linea++;

  if (*linea == '\0') return;

  char comando = linea[0];

  float valor = 0;

  if (comando == 'q') {

    leerSecuencia(linea + 1);
    return;
  }

  if (comando != 's' &&
      comando != 'd' &&
      comando != '+' &&
      comando != '-' &&
      comando != 'i' &&
      comando != '?' &&
      strcmp(linea, "0") != 0) {

    if (!leerNumero(linea + 1, valor)) {
      Serial.println(F("Comando incorrecto"));
      return;
    }
  }

  switch (comando) {

    case 's':

      if (continuo || moviendo || secuenciaActiva) {
        parar();
      } else {
        arrancar();
      }

      break;

    case 'd':

      sentido = -sentido;

      if (continuo) {
        motor.setSpeed(sentido * velocidad);
      }

      Serial.println(sentido > 0 ? F("Sentido +") : F("Sentido -"));

      break;

    case 'v':

      if (valor >= 10 && valor <= 3000) {

        velocidad = valor;
        motor.setMaxSpeed(velocidad);

        if (continuo) {
          motor.setSpeed(sentido * velocidad);
        }

        Serial.print(F("Velocidad: "));
        Serial.println(velocidad);

      } else {
        Serial.println(F("Velocidad fuera de rango"));
      }

      break;

    case 'n':

      if (valor >= 1) {

        pasoSize = (long)valor;

        Serial.print(F("Paso: "));
        Serial.println(pasoSize);

      } else {
        Serial.println(F("Minimo 1 paso"));
      }

      break;

    case 'a':

      if (valor > 0) {

        pasoSize = gradosAPasos(valor);

        if (pasoSize < 1) pasoSize = 1;

        Serial.print(F("Paso en grados: "));
        Serial.println(pasosAGrados(pasoSize), 4);

      } else {
        Serial.println(F("Angulo incorrecto"));
      }

      break;

    case '+':

      moverPasos(pasoSize);
      break;

    case '-':

      moverPasos(-pasoSize);
      break;

    case 'p':

      moverPasos((long)valor);

      Serial.println(F("Movimiento en pasos"));

      break;

    case 'g':

      if (valor >= -180 && valor <= 180) {

        moverPasos(gradosAPasos(valor));

      } else {
        Serial.println(F("Maximo +/-180 grados"));
      }

      break;

    case 'r':

      if (valor > 0) {

        relacion = valor;

        Serial.print(F("Relacion: "));
        Serial.println(relacion);

      }

      break;

    case '0':

      secuenciaActiva = false;
      esperando = false;

      motor.setCurrentPosition(0);

      moviendo = false;
      continuo = false;
      marcha = false;

      Serial.println(F("Cero establecido"));

      break;

    case 'i':

      estado();
      break;

    case '?':

      ayuda();
      break;

    default:

      Serial.println(F("Comando desconocido"));

      break;
  }
}

// =================================
// LEER MONITOR SERIE
// =================================

void leerSerie() {

  while (Serial.available()) {

    char c = Serial.read();

    if (c == '\n' || c == '\r') {

      if (indiceBuffer > 0) {

        bufferSerie[indiceBuffer] = '\0';

        procesarComando(bufferSerie);

        indiceBuffer = 0;
      }

    } else {

      if (indiceBuffer < sizeof(bufferSerie) - 1) {

        bufferSerie[indiceBuffer++] = c;

      } else {

        indiceBuffer = 0;

        Serial.println(F("Error: comando demasiado largo"));
      }
    }
  }
}

// =================================
// BOTONES FISICOS
// =================================

void leerBotones() {

  if (millis() - ultimoBoton < 250) return;

  if (digitalRead(BTN_START) == LOW) {

    ultimoBoton = millis();

    if (continuo || moviendo || secuenciaActiva) {
      parar();
    } else {
      arrancar();
    }

    return;
  }

  if (digitalRead(BTN_DIR) == LOW) {

    ultimoBoton = millis();

    sentido = -sentido;

    if (continuo) {
      motor.setSpeed(sentido * velocidad);
    }

    Serial.println(sentido > 0 ? F("Sentido +") : F("Sentido -"));

    return;
  }

  if (digitalRead(BTN_PASO_P) == LOW) {

    ultimoBoton = millis();

    moverPasos(pasoSize);

    return;
  }

  if (digitalRead(BTN_PASO_N) == LOW) {

    ultimoBoton = millis();

    moverPasos(-pasoSize);

    return;
  }
}

// =================================
// SETUP
// =================================

void setup() {

  Serial.begin(9600);

  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_DIR, INPUT_PULLUP);
  pinMode(BTN_PASO_P, INPUT_PULLUP);
  pinMode(BTN_PASO_N, INPUT_PULLUP);

  motor.setMinPulseWidth(10);

  motor.setMaxSpeed(velocidad);
  motor.setAcceleration(800);

  ayuda();
}

// =================================
// LOOP PRINCIPAL
// =================================

void loop() {

  leerSerie();

  leerBotones();

  // Giro continuo

  if (continuo && marcha) {

    motor.runSpeed();

  } else {

    motor.run();

    // Movimiento terminado

    if (moviendo && motor.distanceToGo() == 0) {

      moviendo = false;

      Serial.print(F("Listo. Posicion: "));
      Serial.print(pasosAGrados(motor.currentPosition()), 2);
      Serial.println(F(" grados"));

      if (secuenciaActiva) {

        esperando = true;

        esperaSecuencia = millis() + PAUSA;
      }
    }
  }

  // Esperar antes del siguiente movimiento

  if (secuenciaActiva &&
      esperando &&
      (long)(millis() - esperaSecuencia) >= 0) {

    siguienteMovimiento();
  }
}
