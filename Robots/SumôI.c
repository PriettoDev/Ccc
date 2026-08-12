#include <WiFi.h>
#include <WebServer.h>

// Canal A da ponte L298N -> Motor ESQUERDO
// Canal B da ponte L298N -> Motor DIREITO
//-----------------------------------------------------------------

//--------------- MOTOR ESQUERDO (Canal A da L298N) ---------------
#define ENA 25 // PWM (velocidade) do motor esquerdo -> pino EN A
#define IN1 26 // direção esquerdo -> pino IN1
#define IN2 27 // direção esquerdo -> pino IN2

//--------------- MOTOR DIREITO (Canal B da L298N) ----------------
#define ENB 14 // PWM (velocidade) do motor direito -> pino EN B
#define IN3 32 // direção direito -> pino IN3
#define IN4 33 // direção direito -> pino IN4

//--------------- SHARP GP2Y0A21 (Analógico) ---------------------
#define SHARP_R 34 // Direita
#define SHARP_L 35 // Esquerda
#define SHARP_M 36 // Frente (meio)

//--------------- Sensores IR de linha (Digital) ----------------
#define IR_DIGITAL_L 39 // Esquerda (branco = LOW)
#define IR_DIGITAL_R 18 // Direita (branco = LOW)

//--------- (VARIÁVEIS) --------
int speedLinear = 150;
int speedTurn = 150;
WebServer server(80);

const char* ap_ssid = "MEPER_YOKOZUNA";
const char* ap_password = "12345678";

// --- memória para "dropout" de curta distância ---
float lastFrontDist = 80.0; // última leitura frontal válida
unsigned long dropoutStart = 0; // instante em que o dropout começou
const unsigned long maxDropoutMs = 500; // por quanto tempo manter o valor antigo (0,5 s)

//----------- (FUNÇÕES DOS MOTORES) ----------
// Vada função controla APENAS UM motor.
void setLeftMotor(int dir1, int dir2, int speed) {
digitalWrite(IN1, dir1);
digitalWrite(IN2, dir2);
analogWrite(ENA, speed);
}

void setRightMotor(int dir1, int dir2, int speed) {
digitalWrite(IN3, dir1);
digitalWrite(IN4, dir2);
analogWrite(ENB, speed);
}

void forward() {
setLeftMotor(HIGH, LOW, speedLinear);
setRightMotor(HIGH, LOW, speedLinear);
}

void backward() {
setLeftMotor(LOW, HIGH, speedLinear);
setRightMotor(LOW, HIGH, speedLinear);
}

void left() {
setLeftMotor(LOW, HIGH, speedTurn); // esquerdo p/ trás
setRightMotor(HIGH, LOW, speedTurn); // direito p/ frente => gira p/ esquerda
}

void right() {
setLeftMotor(HIGH, LOW, speedTurn); // esquerdo p/ frente
setRightMotor(LOW, HIGH, speedTurn); // direito p/ trás => gira p/ direita
}

void stopMotors() {
setLeftMotor(LOW, LOW, 0);
setRightMotor(LOW, LOW, 0);
}

//---------- (SHARP DISTANCE) ----------
float rawSharpDistance(int pin) {
int adc = analogRead(pin);
float voltage = adc * (3.3 / 4095.0);
if (voltage < 0.4) return 80.0;
float distance = 29.988 * pow(voltage, -1.173);
if (distance > 80.0) distance = 80.0;
return distance;
}

//-------- (FILTERED FRONT DISTANCE) --------
float getFilteredFront() {
float raw = rawSharpDistance(SHARP_M);

if (lastFrontDist <= 15.0 && raw >= 30.0) {

float dL = rawSharpDistance(SHARP_L);
float dR = rawSharpDistance(SHARP_R);
if (dL > 30.0 && dR > 30.0) {

if (dropoutStart == 0) {
dropoutStart = millis();
}

if (millis() - dropoutStart < maxDropoutMs) {
return lastFrontDist;
} else {

dropoutStart = 0;
lastFrontDist = raw;
return raw;
}
} else {

dropoutStart = 0;
lastFrontDist = raw;
return raw;
}
} else {

dropoutStart = 0;
lastFrontDist = raw;
return raw;
}
}

//--------------- SETUP ---------------
void setup() {
Serial.begin(115200);

pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

pinMode(IR_DIGITAL_L, INPUT);
pinMode(IR_DIGITAL_R, INPUT);

analogReadResolution(12);

analogWriteResolution(ENA, 8);
analogWriteResolution(ENB, 8);
analogWriteFrequency(ENA, 5000);
analogWriteFrequency(ENB, 5000);

WiFi.softAP(ap_ssid, ap_password);
server.on("/", []() {
server.send(200, "text/plain", "Robot is running");
});
server.begin();

Serial.println("Access Point iniciado\n");
Serial.println("--- MONITOR DE SENSORES ---");
Serial.println("Dica: Ajuste o parafuso dos IR ate que sobre o preto marque 0 e sobre o branco marque 1 (ou o inverso conforme EDGE_LEVEL).");
Serial.println("SharpL\tSharpM\tSharpR\tIR_L\tIR_R\tComando");
Serial.println("------\t------\t------\t----\t----\t-------");

// 5-second startup with IR checks
unsigned long startTime = millis();
while (millis() - startTime < 5000) {
digitalRead(IR_DIGITAL_L);
digitalRead(IR_DIGITAL_R);
delay(20);
}
Serial.println("Startup complete – beginning search.");
}

//------------ MAIN LOOP ------------
void loop() {

float dLeft = rawSharpDistance(SHARP_L);
float dFront = getFilteredFront();
float dRight = rawSharpDistance(SHARP_R);

int irL = digitalRead(IR_DIGITAL_L);
int irR = digitalRead(IR_DIGITAL_R);

bool whiteLeft = (irL == LOW);
bool whiteRight = (irR == LOW);
String command = "";

if (whiteLeft && whiteRight) {
backward();
command = "IR-B";
delay(500);
stopMotors();
}
else if (whiteLeft) {
right();
command = "IR-R";
delay(300);
stopMotors();
}
else if (whiteRight) {
left();
command = "IR-L";
delay(300);
stopMotors();
}
else {
if (dFront <= 30.0) {
forward();
command = "F";
}
else {
bool turnLeft = (dLeft < dRight);
if (turnLeft) left();
else right();
command = (turnLeft) ? "L" : "R";

int confirmCount = 0;
const int required = 2;
unsigned long turnStart = millis();

while (true) {
float f = getFilteredFront();
int irL_t = digitalRead(IR_DIGITAL_L);
int irR_t = digitalRead(IR_DIGITAL_R);

if (irL_t == LOW || irR_t == LOW) {
stopMotors();
command = "STOP_IR";
break;
}

if (f <= 30.0) {
confirmCount++;
if (confirmCount >= required) {
stopMotors();
command = "F_CONFIRMED";
break;
}
} else {
confirmCount = 0
}

Serial.print(rawSharpDistance(SHARP_L), 1); Serial.print("\t");
Serial.print(f, 1); Serial.print("\t");
Serial.print(rawSharpDistance(SHARP_R), 1); Serial.print("\t");
Serial.print(irL_t); Serial.print("\t");
Serial.print(irR_t); Serial.print("\t");
Serial.println(command);

server.handleClient();
delay(20);

if (millis() - turnStart > 3000) {
stopMotors();
command = "TIMEOUT";
break;
}
}

if (command == "F_CONFIRMED") {
forward();
command = "F";
}
}
}

if (command != "L" && command != "R" && command != "F_CONFIRMED" &&
command != "STOP_IR" && command != "TIMEOUT") {
Serial.print(dLeft, 1); Serial.print("\t");
Serial.print(dFront, 1); Serial.print("\t");
Serial.print(dRight, 1); Serial.print("\t");
Serial.print(irL); Serial.print("\t");
Serial.print(irR); Serial.print("\t");
Serial.println(command);
}

server.handleClient();
delay(10);
}