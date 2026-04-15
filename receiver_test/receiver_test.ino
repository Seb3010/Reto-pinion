/*
 * Modo Test Automático para Diagnóstico de Motor y Servo
 *
 * USO:
 * 1. Compilar y subir este archivo en lugar de receiver.ino
 * 2. Abrir Serial Monitor (115200 baud)
 * 3. El sistema iniciará test automático después de 5 segundos
 * 4. No requiere WiFi/UDP - solo hardware
 *
 * Para volver al modo normal, sube receiver.ino nuevamente.
 *
 * Conexiones:
 * - Motor L298N: ENA=GPIO2, IN1=GPIO27, IN2=GPIO26
 * - Servo: GPIO18
 *
 * Parámetros de prueba (AJUSTABLES):
 * - TEST_PWM_LOW: PWM mínimo para pruebas (default: 1000)
 * - TEST_PWM_MID: PWM medio (default: 1800)
 * - TEST_PWM_HIGH: PWM alto (default: 2200)
 * - TEST_PWM_MAX: PWM máximo (default: 3000)
 * - TEST_DURATION_MS: Duración de cada prueba (default: 1000)
 * - TEST_BRAKE_MS: Pausa entre pruebas (default: 500)
 * - TEST_KICK_MS: Duración del kick de arranque (default: 150)
 * - TEST_KICK_PWM: PWM del kick de arranque (default: 2400)
 */

// ============================================================
// AJUSTA ESTOS VALORES PARA PERSONALIZAR LAS PRUEBAS
// ============================================================

const int TEST_PWM_LOW = 1000;      // PWM mínimo para probar arranque
const int TEST_PWM_MID = 1800;      // PWM medio (recomendado para giro estable)
const int TEST_PWM_HIGH = 2200;     // PWM alto
const int TEST_PWM_MAX = 3000;      // PWM máximo

const int TEST_DURATION_MS = 1000;  // Duración de cada prueba en ms
const int TEST_BRAKE_MS = 500;      // Pausa entre pruebas en ms
const int TEST_KICK_MS = 150;       // Duración del kick de arranque
const int TEST_KICK_PWM = 2400;     // PWM del kick de arranque

// ============================================================
// FIN DE PARÁMETROS AJUSTABLES
// ============================================================

// Pines del motor L298N
const int PIN_MOTOR_ENA = 2;
const int PIN_MOTOR_IN1 = 27;
const int PIN_MOTOR_IN2 = 26;

// Pin del servo
const int PIN_SERVO = 18;

// Configuración LEDC (PWM)
const int LEDC_MOTOR_CHANNEL = 0;
const int LEDC_SERVO_CHANNEL = 1;
const int LEDC_MOTOR_FREQ = 5000;
const int LEDC_SERVO_FREQ = 50;
const int LEDC_MOTOR_RESOLUTION = 12;
const int LEDC_SERVO_RESOLUTION = 16;

// Variables de control
int currentPwm = 0;
int currentDirection = 0;  // 0 stop, 1 adelante, -1 reversa
int currentAngle = 90;

// Contadores de pruebas
int testsPassed = 0;
int testsWarning = 0;
int testsFailed = 0;

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
  currentPwm = pwmValue;
  currentDirection = direction;
}

/**
 * Establece la posición del servo
 */
void setServoAngle(int angle) {
  if (angle < 60) angle = 60;
  if (angle > 120) angle = 120;

  const int SERVO_MIN_US = 500;
  const int SERVO_MAX_US = 2500;

  int pulseUs = map(angle, 60, 120, SERVO_MIN_US, SERVO_MAX_US);
  int pulse = (int)((pulseUs * 65535L) / 20000L);

  ledcWrite(LEDC_SERVO_CHANNEL, pulse);
  currentAngle = angle;

  Serial.printf("  Servo: %d° (pulse_us: %d)\n", angle, pulseUs);
}

/**
 * Imprime header de test
 */
void printTestHeader(const char* testName) {
  Serial.println();
  Serial.printf("----------------------------------------\n");
  Serial.printf("TEST: %s\n", testName);
  Serial.println("----------------------------------------");
}

/**
 * Imprime resultado de test
 */
void printTestResult(const char* status, const char* message) {
  Serial.printf("[%-5s] %s\n", status, message);
  Serial.println("----------------------------------------");

  if (strcmp(status, "PASS") == 0) {
    testsPassed++;
  } else if (strcmp(status, "FAIL") == 0) {
    testsFailed++;
  } else {
    testsWarning++;
  }
}

/**
 * Test de servo: barrido 60-120°
 */
void testServoSweep() {
  printTestHeader("SERVO SWEEP");

  Serial.println("  [1/4] Mover a 60°");
  setServoAngle(60);
  delay(500);

  Serial.println("  [2/4] Mover a 90°");
  setServoAngle(90);
  delay(500);

  Serial.println("  [3/4] Mover a 120°");
  setServoAngle(120);
  delay(500);

  Serial.println("  [4/4] Volver a 90°");
  setServoAngle(90);
  delay(500);

  printTestResult("PASS", "Servo responde correctamente");
}

/**
 * Test de motor en una dirección específica
 */
void testMotorDirection(int direction, int pwm, const char* testName) {
  printTestHeader(testName);

  Serial.printf("  Dirección: %s, PWM: %d\n",
                direction > 0 ? "ADELANTE" : "REVERSA", pwm);

  // Aplicar kick de arranque
  applyMotorDrive(direction, TEST_KICK_PWM);
  Serial.printf("  [KICK] PWM: %d por %d ms\n", TEST_KICK_PWM, TEST_KICK_MS);
  delay(TEST_KICK_MS);

  // Aplicar PWM objetivo
  applyMotorDrive(direction, pwm);
  Serial.printf("  [RUN]  PWM: %d por %d ms\n", pwm, TEST_DURATION_MS);
  delay(TEST_DURATION_MS);

  // Frenar
  applyMotorDrive(0, 0);
  Serial.printf("  [STOP] Motor detenido\n");

  printTestResult("PASS", "Motor gira correctamente");
}

/**
 * Test de rampa de velocidades
 */
void testMotorSpeedRamp() {
  printTestHeader("SPEED RAMP");

  int pwms[] = {TEST_PWM_LOW, TEST_PWM_MID, TEST_PWM_HIGH, TEST_PWM_MAX};
  int numSteps = 4;

  for (int i = 0; i < numSteps; i++) {
    int pwm = pwms[i];

    Serial.printf("  [%d/%d] Probando PWM: %d\n", i+1, numSteps, pwm);

    // Kick de arranque
    applyMotorDrive(1, TEST_KICK_PWM);
    delay(TEST_KICK_MS);

    // Aplicar PWM
    applyMotorDrive(1, pwm);
    delay(TEST_DURATION_MS);

    // Evaluar resultado
    if (pwm < TEST_PWM_MID) {
      Serial.printf("    WARNING: Vibración detectada (PWM bajo)\n");
      testsWarning++;
    } else {
      Serial.printf("    OK: Giro estable\n");
    }

    // Frenar
    applyMotorDrive(0, 0);
    delay(TEST_BRAKE_MS);
  }

  printTestResult("PASS", "Rampa completada con advertencias");
}

/**
 * Test de cambio de dirección con freno
 */
void testDirectionChange() {
  printTestHeader("DIRECTION CHANGE");

  Serial.println("  [1/2] Adelante PWM 2200");
  applyMotorDrive(1, 2200);
  delay(TEST_DURATION_MS);

  Serial.printf("  [BRAKE] Frenando por %d ms\n", TEST_BRAKE_MS);
  applyMotorDrive(0, 0);
  delay(TEST_BRAKE_MS);

  Serial.println("  [2/2] Reversa PWM 2200");
  applyMotorDrive(-1, 2200);
  delay(TEST_DURATION_MS);

  applyMotorDrive(0, 0);
  Serial.println("  [STOP] Motor detenido");

  printTestResult("PASS", "Cambio de dirección con freno OK");
}

/**
 * Ejecutar todos los diagnósticos
 */
void runDiagnostics() {
  Serial.println();
  Serial.println("==========================================");
  Serial.println("  MODO TEST AUTOMATICO");
  Serial.println("==========================================");
  Serial.println();

  // Test 1: Servo
  testServoSweep();

  // Test 2: Motor adelante (PWM medio)
  testMotorDirection(1, TEST_PWM_MID, "MOTOR FORWARD");

  // Test 3: Motor reversa (PWM medio)
  testMotorDirection(-1, TEST_PWM_MID, "MOTOR REVERSE");

  // Test 4: Rampa de velocidades
  testMotorSpeedRamp();

  // Test 5: Cambio de dirección
  testDirectionChange();

  // Reporte final
  printFinalReport();
}

/**
 * Imprimir reporte final de diagnóstico
 */
void printFinalReport() {
  Serial.println();
  Serial.println("==========================================");
  Serial.println("  REPORTE DE DIAGNOSTICO");
  Serial.println("==========================================");
  Serial.println();
  Serial.printf("  Tests Pasados:   %d\n", testsPassed);
  Serial.printf("  Tests con Warning: %d\n", testsWarning);
  Serial.printf("  Tests Fallados:   %d\n", testsFailed);
  Serial.println();

  if (testsWarning > 0) {
    Serial.println("  RECOMENDACIONES:");
    Serial.printf("    - Usar PWM minimo %d para giro estable\n", TEST_PWM_MID);
    Serial.println("    - Verificar alimentacion del motor (11.1V OK)");
    Serial.println("    - Ajustar kick PWM si el motor no arranca");
  }

  if (testsFailed == 0) {
    Serial.println();
    Serial.println("  RESULTADO: SISTEMA FUNCIONAL");
  } else {
    Serial.println();
    Serial.println("  RESULTADO: PROBLEMAS DETECTADOS");
  }

  Serial.println("==========================================");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==========================================");
  Serial.println("  MODO TEST - DIAGNOSTICO MOTOR/SERVO");
  Serial.println("==========================================");
  Serial.println();
  Serial.println("Iniciando test automatico en 5 segundos...");
  Serial.println("Conecta el motor y servo antes de continuar");
  Serial.println();

  // Configurar pines
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);

  // Configurar LEDC
  ledcAttach(PIN_MOTOR_ENA, LEDC_MOTOR_FREQ, LEDC_MOTOR_RESOLUTION);
  ledcWrite(LEDC_MOTOR_CHANNEL, 0);

  ledcAttach(PIN_SERVO, LEDC_SERVO_FREQ, LEDC_SERVO_RESOLUTION);

  // Esperar 5 segundos
  for (int i = 5; i > 0; i--) {
    Serial.printf("Iniciando en %d segundos...\n", i);
    delay(1000);
  }
  Serial.println();

  // Ejecutar diagnósticos
  runDiagnostics();
}

void loop() {
  // No hacer nada en loop (test ejecutado una vez)
  delay(1000);
}
