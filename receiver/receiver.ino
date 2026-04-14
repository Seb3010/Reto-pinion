/*
 * ESP32 Receptor - Control de Carro con L298N y Servo
 * Hardware: L298N (motor) + Servo SG90 + WiFi UDP
 *
 * Conexiones L298N:
 * - ENA → GPIO2 (PWM velocidad, pin seguro en ESP32-S3)
 * - IN1 → GPIO27 (dirección motor)
 * - IN2 → GPIO26 (dirección motor)
 *
 * Conexiones Servo:
 * - PWM → GPIO18 (50Hz, 60°-120°)
 *
 * Protocolo WiFi UDP:
 * Y:{velocidad}\nX:{angulo}\n
 *
 * WiFi Config:
 * - Crea AP WiFi (SSID: CARRO_WIFI)
 * - Recibe datos por UDP desde el transmisor
 * - IP del AP: 192.168.4.1 (automática en AP Mode)
 * - Puerto UDP: 4210
 * - Canal WiFi: 1
 */

#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi AP Configuration
const char* ssid = "CARRO_WIFI";       // SSID de la red del receptor
const char* password = "12345678";     // Contraseña del AP (mínimo 8 caracteres)
const int udpPort = 4210;              // Puerto UDP

WiFiUDP Udp;
char packetBuffer[255];                 // Buffer para recibir paquetes UDP
int packetCount = 0;                   // Contador de paquetes recibidos

// Configuración de pines L298N
const int PIN_MOTOR_ENA = 2;  // PWM velocidad (pin seguro en ESP32-S3)
const int PIN_MOTOR_IN1 = 27;  // Dirección
const int PIN_MOTOR_IN2 = 26;  // Dirección

// Configuración de pin Servo
const int PIN_SERVO = 18;

// Configuración LEDC (PWM)
const int LEDC_MOTOR_CHANNEL = 0;    // Canal para motor
const int LEDC_SERVO_CHANNEL = 1;     // Canal para servo
const int LEDC_MOTOR_FREQ = 5000;     // 5kHz (rango 1-20kHz)
const int LEDC_SERVO_FREQ = 50;       // 50Hz para servo
const int LEDC_MOTOR_RESOLUTION = 12; // 12-bit (0-4095)
const int LEDC_SERVO_RESOLUTION = 16; // 16-bit (0-65535)

// Variables de control
int velocidadMotor = 0;      // Velocidad: -255 a +255
int anguloServo = 90;        // Ángulo: 60 a 120 grados
int velocidadPWM = 0;        // Valor PWM para LEDC
int servoPulse = 4915;       // Duty estimado servo (centro aprox)
int servoPulseUs = 1500;     // Pulso en microsegundos para diagnóstico

// Failsafe por pérdida de enlace UDP
unsigned long lastPacketMs = 0;
bool failsafeActivo = false;
const unsigned long FAILSAFE_TIMEOUT_MS = 700;

// Configuración de servo SG90 (calibrable)
const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2500;
const int SERVO_CENTER_ANGLE = 90;

// Configuración anti-vibración / arranque motor
// Ajustado para eliminar vibración y asegurar giro estable
const int MOTOR_RX_DEADBAND = 8;       // Menos deadband = más sensibilidad
const int MOTOR_MIN_RUN_PWM = 1800;    // Más potencia mínima para vencer inercia
const int MOTOR_KICK_PWM = 2200;       // Empuje inicial más fuerte
const int MOTOR_KICK_MS = 180;         // Empuje más largo para asegurar arranque
const int MOTOR_DIR_BRAKE_MS = 50;     // Freno más efectivo al cambiar dirección
int motorLastDirection = 0;            // -1 reversa, 0 stop, 1 adelante

/**
 * Extrae un valor entero de un mensaje con formato "KEY:valor"
 * Soporta fin por '\n' o fin de string.
 */
bool extractIntValue(const String& mensaje, const char* key, int& outValue) {
  int idxKey = mensaje.indexOf(key);
  if (idxKey < 0) return false;

  int start = idxKey + strlen(key);
  int end = mensaje.indexOf('\n', start);
  if (end < 0) {
    end = mensaje.length();
  }

  String valueStr = mensaje.substring(start, end);
  valueStr.trim();

  if (valueStr.length() == 0) return false;

  outValue = valueStr.toInt();
  return true;
}

/**
 * Aplica dirección + PWM al puente H
 */
void applyMotorDrive(int direction, int pwmValue) {
  if (direction > 0) {
    digitalWrite(PIN_MOTOR_IN1, HIGH);
    digitalWrite(PIN_MOTOR_IN2, LOW);
  } else if (direction < 0) {
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, HIGH);
  } else {
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, LOW);
    pwmValue = 0;
  }

  ledcWrite(LEDC_MOTOR_CHANNEL, pwmValue);
  velocidadPWM = pwmValue;
}

/**
 * Barrido de servo para validar cableado/alimentación sin depender de UDP
 */
void testServoSweep() {
  Serial.println("[SERVO TEST] Iniciando barrido 60->90->120->90");
  setServoAngle(60);
  delay(600);
  setServoAngle(90);
  delay(600);
  setServoAngle(120);
  delay(600);
  setServoAngle(90);
  delay(600);
  Serial.println("[SERVO TEST] Fin de barrido");
}

/**
 * Establece la posición del servo
 * @param angle Ángulo deseado (60-120 grados)
 */
void setServoAngle(int angle) {
  // Limitar ángulo al rango válido
  if (angle < 60) angle = 60;
  if (angle > 120) angle = 120;
  
  // Mapear ángulo a pulso SG90 (calibrable) y luego a duty 16-bit en 50Hz (20ms)
  int pulseUs = map(angle, 60, 120, SERVO_MIN_US, SERVO_MAX_US);
  int pulse = (int)((pulseUs * 65535L) / 20000L);
  servoPulse = pulse;
  servoPulseUs = pulseUs;
  
  // Escribir valor PWM al servo
  ledcWrite(LEDC_SERVO_CHANNEL, pulse);
  
  // Debug
  Serial.printf("Servo: %d° (pulse_us: %d, duty: %d)\n", angle, pulseUs, pulse);
}

/**
 * Controla la velocidad y dirección del motor
 * @param speed Velocidad: -255 a +255 (negativo=reversa, positivo=adelante)
 */
void setMotorSpeed(int speed) {
  // Limitar velocidad al rango válido
  if (speed > 255) speed = 255;
  if (speed < -255) speed = -255;

  // Zona muerta para evitar vibración por comandos bajos
  if (abs(speed) <= MOTOR_RX_DEADBAND) {
    applyMotorDrive(0, 0);
    motorLastDirection = 0;
    Serial.printf("Motor: %d (STOP por deadband)\n", speed);
    return;
  }

  int direction = (speed > 0) ? 1 : -1;
  int targetPwm = map(abs(speed), 0, 255, 0, 4095);
  int appliedPwm = max(targetPwm, MOTOR_MIN_RUN_PWM);

  // Cambio de dirección: freno corto antes de invertir
  if (motorLastDirection != 0 && direction != motorLastDirection) {
    applyMotorDrive(0, 0);
    delay(MOTOR_DIR_BRAKE_MS);
  }

  // Empuje inicial para vencer inercia (kick siempre al arrancar o cambiar dirección)
  bool needsKick = (motorLastDirection == 0 && direction != 0) ||
                   (direction != motorLastDirection && motorLastDirection != 0);

  if (needsKick) {
    int kickPwm = max(appliedPwm, MOTOR_KICK_PWM);
    applyMotorDrive(direction, kickPwm);
    delay(MOTOR_KICK_MS);
  }

  applyMotorDrive(direction, appliedPwm);
  motorLastDirection = direction;

  // Debug
  Serial.printf("Motor: %d (PWM target:%d applied:%d dir:%d kick:%s)\n",
                speed, targetPwm, velocidadPWM, direction, needsKick ? "YES" : "NO");
}

void setup() {
  // Inicializar Serial USB para debug
  Serial.begin(115200);
  Serial.println("=== Receptor Carro ESP32 ===");
  Serial.println("Iniciando sistema...");
  
  // Configurar pines de dirección del motor
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  
  // Configurar LEDC para el motor PWM
  ledcSetup(LEDC_MOTOR_CHANNEL, LEDC_MOTOR_FREQ, LEDC_MOTOR_RESOLUTION);
  ledcAttachPin(PIN_MOTOR_ENA, LEDC_MOTOR_CHANNEL);
  ledcWrite(LEDC_MOTOR_CHANNEL, 0); // Iniciar detenido
  
  // Configurar LEDC para el servo PWM
  ledcSetup(LEDC_SERVO_CHANNEL, LEDC_SERVO_FREQ, LEDC_SERVO_RESOLUTION);
  ledcAttachPin(PIN_SERVO, LEDC_SERVO_CHANNEL);
  
  // Arrancar seguro con motor detenido
  setMotorSpeed(0);

  // Inicializar servo en posición central
  setServoAngle(SERVO_CENTER_ANGLE);
  testServoSweep();

  // Crear AP WiFi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, 1);  // Canal 1

  Serial.println("✓ AP WiFi creado!");
  Serial.printf("SSID: %s\n", ssid);
  Serial.printf("Contraseña: %s\n", password);
  Serial.printf("IP del AP: %s\n", WiFi.softAPIP().toString().c_str()); // Siempre 192.168.4.1
  Serial.println("El transmisor debe conectarse a este AP");

  // UDP
  Udp.begin(udpPort);
  Serial.printf("UDP escuchando en puerto %d\n", udpPort);
  lastPacketMs = millis();

  Serial.println("¡Sistema listo! Esperando datos del transmisor...");
}

void loop() {
  // Verificar si hay datos disponibles en UDP
  int packetSize = Udp.parsePacket();

  if (packetSize) {
    // Leer paquete UDP
    int len = Udp.read(packetBuffer, 254);
    packetBuffer[len] = '\0';  // Terminar string

    // Imprimir comando recibido para debug
    Serial.printf("Recibido %d bytes: %s\n", packetSize, packetBuffer);

    // Procesar comando si no está vacío
    if (len > 0) {
      // Parsear comandos
      String mensaje = String(packetBuffer);

      int valorY = 0;
      int valorX = 0;

      if (extractIntValue(mensaje, "Y:", valorY)) {
        velocidadMotor = valorY;
        setMotorSpeed(velocidadMotor);
        Serial.printf("→ Velocidad establecida: %d\n", velocidadMotor);
      }

      if (extractIntValue(mensaje, "X:", valorX)) {
        anguloServo = valorX;
        setServoAngle(anguloServo);
        Serial.printf("→ Ángulo establecido: %d°\n", anguloServo);
      }

      lastPacketMs = millis();
      failsafeActivo = false;
      packetCount++;  // Incrementar contador de paquetes
    }
  }

  // Failsafe: si se pierde enlace UDP, detener motor y centrar servo
  unsigned long packetAgeMs = millis() - lastPacketMs;
  if (packetAgeMs > FAILSAFE_TIMEOUT_MS && !failsafeActivo) {
    velocidadMotor = 0;
    anguloServo = SERVO_CENTER_ANGLE;
    setMotorSpeed(velocidadMotor);
    setServoAngle(anguloServo);
    failsafeActivo = true;
    Serial.printf("⚠ FAILSAFE activado (sin paquetes por %lu ms)\n", packetAgeMs);
  }

  // Diagnóstico periódico mejorado
  static unsigned long lastDiagMs = 0;
  if (millis() - lastDiagMs > 500) {
    bool deadbandActive = (abs(velocidadMotor) <= MOTOR_RX_DEADBAND);
    Serial.printf("[DIAG] speed=%d pwm=%d in1=%d in2=%d angle=%d pulse=%dus dead=%s age=%lums pkts=%d\n",
                  velocidadMotor,
                  velocidadPWM,
                  digitalRead(PIN_MOTOR_IN1),
                  digitalRead(PIN_MOTOR_IN2),
                  anguloServo,
                  servoPulseUs,
                  deadbandActive ? "YES" : "NO",
                  packetAgeMs,
                  packetCount);
    lastDiagMs = millis();
  }
}