/*
 * ESP32 Transmisor - Control Remoto de Carro
 * Hardware: Joystick analógico + WiFi UDP
 *
 * Conexiones:
 * - Joystick Eje Y → GPIO34 (ADC1_CH6)
 * - Joystick Eje X → GPIO35 (ADC1_CH7)
 *
 * Protocolo WiFi UDP:
 * Y:{velocidad}\nX:{angulo}\n
 *
 * Velocidad: -255 a +255 (negativo=reversa, positivo=adelante)
 * Ángulo: 60° a 120°
 *
 * WiFi Config:
 * - Conecta al AP del receptor (SSID: CARRO_WIFI)
 * - Enviará datos por UDP al receptor
 * - IP del receptor: 192.168.4.1
 * - Puerto UDP: 4210
 */

#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi Configuration
const char* ssid = "CARRO_WIFI";         // SSID del AP del receptor
const char* password = "12345678";       // Contraseña del AP
const char* receptorIP = "192.168.4.1";  // IP fija del AP (ESP32 en AP Mode)
const int udpPort = 4210;                // Puerto UDP

WiFiUDP Udp;
bool wifiConectado = false;

// Configuración de pines
const int PIN_JOYSTICK_Y = 34;  // ADC1_CH6
const int PIN_JOYSTICK_X = 35;  // ADC1_CH7

// Variables de control
int yRaw = 0;           // Valor crudo del eje Y (0-4095)
int xRaw = 0;           // Valor crudo del eje X (0-4095)
int velocidadMotor = 0; // Velocidad motor: -255 a +255
int anguloServo = 90;   // Ángulo servo: 60 a 120 grados

// Valores previos para detección de cambios
int prevVelocidad = 0;
int prevAngulo = 90;

// Umbral para evitar envío excesivo
const int THRESHOLD_VELOCIDAD = 2;  // Más sensible para respuesta rápida
const int THRESHOLD_ANGULO = 1;     // Más sensible para respuesta rápida
const unsigned long HEARTBEAT_MS = 100;  // Envío periódico para mantener enlace activo

// Histeresis motor (evita vibración por ruido cerca del centro)
// Ajustado para mayor sensibilidad y enviar valores más altos
const int MOTOR_ENTER_FORWARD = 2150;  // Reducido para mayor sensibilidad
const int MOTOR_EXIT_FORWARD = 2050;   // Reducido
const int MOTOR_ENTER_REVERSE = 1950;  // Aumentado
const int MOTOR_EXIT_REVERSE = 2050;   // Aumentado

// Zona muerta dedicada para servo
const int SERVO_DEAD_MIN = 1920;
const int SERVO_DEAD_MAX = 2120;

// Protección anti-inversión brusca
const unsigned long MOTOR_REVERSE_GUARD_MS = 120;

unsigned long lastSendMs = 0;
unsigned long lastMotorStateChangeMs = 0;
int motorState = 0;  // -1 reversa, 0 centro, 1 adelante

// Función para verificar estado de WiFi
void checkWiFiStatus() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {  // Cada 5 segundos
    if (WiFi.status() == WL_CONNECTED) {
      if (!wifiConectado) {
        wifiConectado = true;
        Serial.println("✓ WiFi RE-conectado!");
      }
    } else {
      wifiConectado = false;
      Serial.println("✗ WiFi desconectado! Intentando reconectar...");
      WiFi.reconnect();
    }
    lastCheck = millis();
  }
}

void setup() {
  // Inicializar Serial USB para debug
  Serial.begin(115200);
  Serial.println("=== Transmisor WiFi (Joystick) ===");
  Serial.println("Iniciando sistema...");

  // WiFi - Station Mode (se conecta al AP del receptor)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando a ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✓ WiFi conectado!");
  Serial.printf("IP del transmisor: %s\n", WiFi.localIP().toString().c_str());
  wifiConectado = true;

  // UDP
  Udp.begin(udpPort);
  Serial.printf("UDP iniciado en puerto %d\n", udpPort);
  Serial.printf("Enviaré a receptor IP: %s:%d\n", receptorIP, udpPort);

  // Los pines ADC no requieren configuración explícita
  // ADC1 en ESP32 usa los pines GPIO34, GPIO35, etc.

  Serial.println("¡Sistema listo!");
}

void loop() {
  // Verificar estado WiFi
  checkWiFiStatus();

  // Leer valores del joystick (ADC 12-bit: 0-4095)
  yRaw = analogRead(PIN_JOYSTICK_Y);
  xRaw = analogRead(PIN_JOYSTICK_X);

  // Eje Y: Control de velocidad con histeresis real
  int desiredMotorState = motorState;
  if (motorState == 0) {
    if (yRaw >= MOTOR_ENTER_FORWARD) desiredMotorState = 1;
    else if (yRaw <= MOTOR_ENTER_REVERSE) desiredMotorState = -1;
  } else if (motorState > 0) {
    if (yRaw <= MOTOR_EXIT_FORWARD) desiredMotorState = 0;
  } else {  // motorState < 0
    if (yRaw >= MOTOR_EXIT_REVERSE) desiredMotorState = 0;
  }

  // No permitir inversión inmediata por ruido
  if (desiredMotorState != motorState) {
    bool cambioDirecto = (desiredMotorState != 0 && motorState != 0);
    if (cambioDirecto && (millis() - lastMotorStateChangeMs) < MOTOR_REVERSE_GUARD_MS) {
      desiredMotorState = 0;
    } else {
      motorState = desiredMotorState;
      lastMotorStateChangeMs = millis();
    }
  }

  if (motorState > 0) {
    int yClamped = constrain(yRaw, MOTOR_EXIT_FORWARD, 4095);
    velocidadMotor = map(yClamped, MOTOR_EXIT_FORWARD, 4095, 50, 255);  // ↑ mínimo 50 para evitar deadband
  } else if (motorState < 0) {
    int yClamped = constrain(yRaw, 0, MOTOR_EXIT_REVERSE);
    velocidadMotor = map(yClamped, 0, MOTOR_EXIT_REVERSE, -255, -50);  // ↑ máximo -50 para evitar deadband
  } else {
    velocidadMotor = 0;
  }

  // Eje X: Control de dirección (ángulo servo) con zona muerta dedicada
  if (xRaw > SERVO_DEAD_MAX) {
    // Derecha
    anguloServo = map(xRaw, SERVO_DEAD_MAX, 4095, 90, 120);
  } else if (xRaw < SERVO_DEAD_MIN) {
    // Izquierda
    anguloServo = map(xRaw, 0, SERVO_DEAD_MIN, 60, 90);
  } else {
    // Centro
    anguloServo = 90;
  }

  // Enviar datos si:
  // 1) hubo cambio significativo (respuesta rápida), o
  // 2) toca heartbeat periódico (mantener enlace/failsafe del receptor)
  bool cambioSignificativo =
      (abs(velocidadMotor - prevVelocidad) > THRESHOLD_VELOCIDAD ||
       abs(anguloServo - prevAngulo) > THRESHOLD_ANGULO);
  bool heartbeatVencido = (millis() - lastSendMs) >= HEARTBEAT_MS;

  if (wifiConectado && (cambioSignificativo || heartbeatVencido)) {

    // Enviar por UDP
    Udp.beginPacket(receptorIP, udpPort);
    Udp.printf("Y:%d\n", velocidadMotor);
    Udp.printf("X:%d\n", anguloServo);
    Udp.endPacket();

    // Debug por Serial USB (con valores crudos del ADC para diagnóstico)
    Serial.printf("UDP enviado - Y_raw:%d speed:%d state:%d X_raw:%d angle:%d (%s)\n",
                  yRaw, velocidadMotor, motorState, xRaw, anguloServo,
                  cambioSignificativo ? "cambio" : "heartbeat");

    // Actualizar valores previos
    prevVelocidad = velocidadMotor;
    prevAngulo = anguloServo;
    lastSendMs = millis();
  }

  // Pequeño delay para estabilidad
  delay(50);
}
