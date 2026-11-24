/*Todas las instrucciones

Abre el Monitor serie a 9600 baudios y escribe el comando (con o sin Enter). Los pulsadores son opcionales.

Comandos por el Monitor serie
Comando	Qué hace	Ejemplo
s	Arranca el giro continuo. Si ya está girando o moviéndose, lo para	s
d	Cambia el sentido del giro continuo	d
v + número	Velocidad en pasos/seg (10 a 3000). Afecta al giro continuo y a los pasos	v500
n + número	Tamaño del paso en pasos de motor	n20
a + número	Tamaño del paso en grados del plato	a0.5
+	Un paso adelante (del tamaño definido con n o a)	+
-	Un paso atrás	-
p + número	Mueve esa cantidad de pasos de motor (negativo = atrás)	p100, p-100
g + número	Mueve esos grados del plato (negativo = atrás)	g90, g-45
r + número	Relación de reducción de tu eje	r12
0	Pone la posición actual como cero	0
i	Muestra tamaño de paso, velocidad, relación y posición	i
?	Muestra la ayuda en pantalla	?
Pulsadores (entre el pin y GND)
Pin	Función
4	Arrancar / parar
5	Cambiar sentido del giro continuo
6	Un paso adelante
7	Un paso atrás
Cómo se usa, paso a paso

Giro continuo

v800 para fijar la velocidad.
s para arrancar.
d para invertir el sentido mientras gira.
s para parar (la parada es inmediata).

Paso a paso con grados

a1 define el paso en 1° del plato.
+ avanza 1°, - retrocede 1°.
Repite + las veces que quieras.

Paso a paso con pasos del motor

n20 define el paso en 20 pasos del motor.
+ y - para moverlo.

Movimiento puntual

g90 gira 90° del plato y se detiene. g-90 vuelve.
p1600 da 1600 pasos de motor (una vuelta del motor a 1/8).
Si quieres frenar un movimiento en curso, escribe s: frena con rampa.

Posición cero

Mueve el plato a la posición que quieras como referencia.
Escribe 0.
Con i ves los grados respecto a ese cero.
Datos importantes
Micropaso: SW5 OFF, SW6 OFF, SW7 ON = 1/8 = 1600 pasos/vuelta del motor.
Relación: ajústala con r cuando midas tu reductora. Si el plato giraba unos 30° por vuelta del motor, será r12.
Paso mínimo: con 1/8 y relación 12, un paso son 0,01875° del plato.
Velocidad máxima: no pases de 3000 pasos/seg para no perder pasos.
Parada: s en giro continuo para en seco; en un movimiento por grados o pasos frena con rampa.
El código no tiene finales de carrera: el plato gira libremente, así que vigila que los cables no se enreden si das muchas vueltas.
Si algo falla
No responde a los comandos: comprueba los 9600 baudios y que el puerto COM sea el correcto.
El motor vibra pero no gira: baja la velocidad (v300) o revisa el cableado de las bobinas.
Gira al revés de lo que esperas: usa d o invierte los cables de una bobina.
Los grados no salen exactos: mide la reducción real y ajusta r.*/


#include <AccelStepper.h>

#define PUL_PIN    2
#define DIR_PIN    3
#define BTN_START  4   // arrancar/parar (giro continuo)
#define BTN_DIR    5   // cambiar sentido (giro continuo)
#define BTN_PASO_P 6   // paso adelante
#define BTN_PASO_N 7   // paso atras

const long PASOS_MOTOR = 1600;   // 1/8 micropaso
float relacion = 12.0;           // reduccion del eje (ajustala)

AccelStepper motor(AccelStepper::DRIVER, PUL_PIN, DIR_PIN);

bool  marcha    = false;   // true = giro continuo activo
bool  continuo  = false;   // true = modo giro continuo
bool  moviendo  = false;   // true = movimiento por pasos/grados en curso
int   sentido   = 1;       // 1 o -1 (solo giro continuo)
float velocidad = 800;     // pasos/seg
long  pasoSize  = 1;       // tamano del paso en pasos de motor

unsigned long ultimoBoton = 0;

long gradosAPasos(float g) {
  return (long)round(g / 360.0 * PASOS_MOTOR * relacion);
}

float pasosAGrados(long p) {
  return (float)p * 360.0 / (PASOS_MOTOR * relacion);
}

void ayuda() {
  Serial.println(F("--- COMANDOS ---"));
  Serial.println(F("s       -> arrancar/parar giro continuo"));
  Serial.println(F("d       -> cambiar sentido (giro continuo)"));
  Serial.println(F("v500    -> velocidad en pasos/seg (10-3000)"));
  Serial.println(F("--- PASO A PASO ---"));
  Serial.println(F("n10     -> tamano de paso = 10 pasos de motor"));
  Serial.println(F("a0.5    -> tamano de paso = 0.5 grados del plato"));
  Serial.println(F("+       -> un paso adelante"));
  Serial.println(F("-       -> un paso atras"));
  Serial.println(F("p100    -> mover 100 pasos (negativo = atras)"));
  Serial.println(F("g90     -> mover 90 grados (negativo = atras)"));
  Serial.println(F("--- OTROS ---"));
  Serial.println(F("r12     -> relacion de reduccion"));
  Serial.println(F("0       -> poner posicion actual como cero"));
  Serial.println(F("i       -> ver estado"));
  Serial.println(F("?       -> esta ayuda"));
}

void estado() {
  Serial.print(F("Paso: ")); Serial.print(pasoSize);
  Serial.print(F(" pasos = ")); Serial.print(pasosAGrados(pasoSize), 4);
  Serial.println(F(" grados del plato"));
  Serial.print(F("Velocidad: ")); Serial.println(velocidad);
  Serial.print(F("Relacion: ")); Serial.println(relacion);
  Serial.print(F("Posicion: ")); Serial.print(motor.currentPosition());
  Serial.print(F(" pasos = ")); Serial.print(pasosAGrados(motor.currentPosition()), 2);
  Serial.println(F(" grados"));
}

void arrancar() {
  motor.setCurrentPosition(motor.currentPosition());  // anula movimientos pendientes
  moviendo = false;
  continuo = true;
  marcha = true;
  motor.setMaxSpeed(velocidad);
  motor.setSpeed(sentido * velocidad);
  Serial.println(F("MARCHA"));
}

void parar() {
  if (continuo) {
    marcha = false;
    continuo = false;
    motor.setCurrentPosition(motor.currentPosition());  // parada inmediata
    Serial.println(F("PARADO"));
  } else if (moviendo) {
    motor.stop();                                       // frena con rampa
    Serial.println(F("Frenando..."));
  }
}

// Movimiento relativo en pasos (positivo o negativo)
void moverPasos(long pasos) {
  if (pasos == 0) {
    Serial.println(F("Movimiento de 0 pasos (angulo demasiado pequeno)"));
    return;
  }
  if (continuo) {                       // salir del modo continuo
    marcha = false;
    continuo = false;
    motor.setCurrentPosition(motor.currentPosition());
  }
  motor.setMaxSpeed(velocidad);
  motor.move(pasos);
  moviendo = true;
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(30);                // parseFloat no se queda esperando
  pinMode(BTN_START,  INPUT_PULLUP);
  pinMode(BTN_DIR,    INPUT_PULLUP);
  pinMode(BTN_PASO_P, INPUT_PULLUP);
  pinMode(BTN_PASO_N, INPUT_PULLUP);

  motor.setMinPulseWidth(10);
  motor.setMaxSpeed(velocidad);
  motor.setAcceleration(800);
  ayuda();
}

void leerSerie() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == '\n' || c == '\r' || c == ' ') return;
  float valor = Serial.parseFloat();
  while (Serial.available()) Serial.read();   // vaciar buffer

  switch (c) {
    case 's':
      if (continuo || moviendo) parar(); else arrancar();
      break;

    case 'd':
      sentido = -sentido;
      if (continuo) motor.setSpeed(sentido * velocidad);
      Serial.println(sentido > 0 ? F("Sentido +") : F("Sentido -"));
      break;

    case 'v':
      if (valor >= 10 && valor <= 3000) {
        velocidad = valor;
        motor.setMaxSpeed(velocidad);
        if (continuo) motor.setSpeed(sentido * velocidad);
        Serial.print(F("Velocidad: ")); Serial.println(velocidad);
      } else Serial.println(F("Velocidad fuera de rango"));
      break;

    case 'n':
      if (valor >= 1) {
        pasoSize = (long)valor;
        Serial.print(F("Paso = ")); Serial.print(pasoSize);
        Serial.print(F(" pasos = ")); Serial.print(pasosAGrados(pasoSize), 4);
        Serial.println(F(" grados"));
      } else Serial.println(F("Minimo 1 paso"));
      break;

    case 'a': {
      long p = gradosAPasos(fabs(valor));
      if (p < 1) {
        Serial.print(F("Muy pequeno. Minimo = "));
        Serial.print(pasosAGrados(1), 4);
        Serial.println(F(" grados"));
        p = 1;
      }
      pasoSize = p;
      Serial.print(F("Paso = ")); Serial.print(pasosAGrados(pasoSize), 4);
      Serial.print(F(" grados ("));
      Serial.print(pasoSize); Serial.println(F(" pasos)"));
      break;
    }

    case '+':
      moverPasos(pasoSize);
      break;

    case '-':
      moverPasos(-pasoSize);
      break;

    case 'p':
      moverPasos((long)valor);
      Serial.print(F("Moviendo pasos: ")); Serial.println((long)valor);
      break;

    case 'g':
      moverPasos(gradosAPasos(valor));
      Serial.print(F("Moviendo grados: ")); Serial.println(valor);
      break;

    case 'r':
      if (valor > 0) {
        relacion = valor;
        Serial.print(F("Relacion: ")); Serial.println(relacion);
      }
      break;

    case '0':
      motor.setCurrentPosition(0);
      Serial.println(F("Cero establecido"));
      break;

    case 'i':
      estado();
      break;

    case '?':
      ayuda();
      break;
  }
}

void leerBotones() {
  if (millis() - ultimoBoton < 250) return;   // antirrebote

  if (digitalRead(BTN_START) == LOW) {
    ultimoBoton = millis();
    if (continuo || moviendo) parar(); else arrancar();
  }
  if (digitalRead(BTN_DIR) == LOW) {
    ultimoBoton = millis();
    sentido = -sentido;
    if (continuo) motor.setSpeed(sentido * velocidad);
    Serial.println(sentido > 0 ? F("Sentido +") : F("Sentido -"));
  }
  if (digitalRead(BTN_PASO_P) == LOW) {
    ultimoBoton = millis();
    moverPasos(pasoSize);
  }
  if (digitalRead(BTN_PASO_N) == LOW) {
    ultimoBoton = millis();
    moverPasos(-pasoSize);
  }
}

void loop() {
  leerSerie();
  leerBotones();

  if (continuo && marcha) {
    motor.runSpeed();                   // giro continuo
  } else {
    motor.run();                        // pasos / grados con rampa
    if (moviendo && motor.distanceToGo() == 0) {
      moviendo = false;
      Serial.print(F("Listo. Posicion: "));
      Serial.print(pasosAGrados(motor.currentPosition()), 2);
      Serial.println(F(" grados"));
    }
  }
}