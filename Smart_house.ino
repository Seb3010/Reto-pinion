// LIBRERÍAS PARA PANTALLA Y SERVOS
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// OBJETOS DE CONTROL
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Pantalla LCD I2C (dirección 0x27, 16 columnas, 2 filas)
Servo myServo;                      // Ventana principal - Control manual con botón verde
Servo myServo_1;                    // Ventana de lluvia - Control automático con sensor
Servo myServo_2;                    // Tercer servo/Puerta - Control manual con botón rojo (toggle)

// ASIGNACIÓN DE PINES
const int BUTTON_PIN = 6;           // Botón Verde: Control manual de ventana principal
const int BUTTON_PIN_RED = 5;       // Botón Rojo: Control toggle del tercer servo
const int SERVO_PIN = 11;           // Servo 1: Ventana principal
const int SERVO_W_PIN = 3;          // Servo 2: Ventana de lluvia
const int SERVO_D_PIN = 10;         // Servo 3: Tercer servo/Puerta
const int SENSOR_RAIN_PIN = A1;     // Sensor de lluvia: Entrada analógica

// ANGULOS DE SERVOS: Posiciones para abrir/cerrar cada servo
const int OPEN_ANGLE = 95;          // Ángulo para abrir ventana principal
const int CLOSE_ANGLE = 180;        // Ángulo para cerrar ventana principal
const int OPEN_ANGLE_W = 95;        // Ángulo para abrir ventana de lluvia
const int CLOSE_ANGLE_W = 0;        // Ángulo para cerrar ventana de lluvia

const int OPEN_ANGLE_D = 100;       // Ángulo para abrir tercer servo/puerta
const int CLOSE_ANGLE_D = 180;      // Ángulo para cerrar tercer servo/puerta

// UMBRALES DE HISTÉRESIS: Evita oscilaciones en sensor de lluvia
// Técnica para evitar encendido/apagado rápido cuando el valor está cerca del umbral
const int UMBRAL_LLUVIA_BAJO = 50;  // Valor bajo para detectar "no hay lluvia"
const int UMBRAL_LLUVIA_ALTO = 150; // Valor alto para detectar "hay lluvia"

// VARIABLES DE ESTADO: Control del sistema
bool ventana_lluvia_abierta = true;  // Estado de ventana de lluvia (true = abierta)
bool tercer_servo_abierto = false;   // Estado del tercer servo (true = abierto) para control toggle

void setup() {
  // INICIALIZACIÓN DE PANTALLA LCD
  lcd.init();                    // Iniciar comunicación I2C con pantalla
  lcd.backlight();               // Activar retroiluminación
  lcd.setCursor(0, 0);          // Colocar cursor en primera línea, primera columna
  lcd.print("Bienvenido");       // Mensaje de bienvenida inicial

  // CONFIGURACIÓN DE ENTRADAS (Botones)
  pinMode(BUTTON_PIN, INPUT_PULLUP);     // Botón Verde: PULLUP interno (LOW cuando presionado)
  pinMode(BUTTON_PIN_RED, INPUT_PULLUP); // Botón Rojo: PULLUP interno (LOW cuando presionado)

  // CONFIGURACIÓN DE SERVOS: Asociar cada objeto servo a su pin correspondiente
  myServo.attach(SERVO_PIN);     // Ventana principal conectada al pin 11
  myServo_1.attach(SERVO_W_PIN); // Ventana de lluvia conectada al pin 3
  myServo_2.attach(SERVO_D_PIN); // Tercer servo/puerta conectada al pin 10

  // POSICIONES INICIALES DE LOS SERVOS
  myServo.write(OPEN_ANGLE);     // Ventana principal inicia ABIERTA
  myServo_1.write(OPEN_ANGLE_W); // Ventana de lluvia inicia ABIERTA
  myServo_2.write(CLOSE_ANGLE_D); // Tercer servo inicia CERRADO

  delay(1500);                   // Mostrar mensaje de bienvenida por 1.5 segundos
  lcd.clear();                   // Limpiar pantalla para mostrar información en tiempo real
}

void loop() {
  // LECTURA DE SENSORES Y BOTONES
  int buttonState = digitalRead(BUTTON_PIN);       // Leer botón verde
  int buttonState_red = digitalRead(BUTTON_PIN_RED); // Leer botón rojo
  int valorHumedad = analogRead(SENSOR_RAIN_PIN);    // Leer sensor de lluvia (0-1023)
  
  // ==========================================
  // SISTEMA 1: CONTROL MANUAL DE VENTANA PRINCIPAL
  // Botón verde controla ventana con lógica MANTENIMIENTO
  // ==========================================
  if (buttonState == LOW) { 
    // Botón presionado: Mover servo a posición CERRADA
    myServo.write(CLOSE_ANGLE);
    // Mostrar estado en pantalla (segunda línea)
    lcd.setCursor(0, 1);
    lcd.print("Manual: ABIERTO  ");
  } 
  else { 
    // Botón solto: Mover servo a posición ABIERTA
    myServo.write(OPEN_ANGLE);
    // Mostrar estado en pantalla (segunda línea)
    lcd.setCursor(0, 1);
    lcd.print("Manual: CERRADO ");
  }

  // ==========================================
  // SISTEMA 2: CONTROL TOGGLE DEL TERCER SERVO
  // Botón rojo funciona como INTERRUPTOR (una presión cambia estado)
  // ==========================================
  if (buttonState_red == LOW) { 
    if (tercer_servo_abierto == false) {
      // Si está cerrado, ABRIR
      myServo_2.write(OPEN_ANGLE_D);
      tercer_servo_abierto = true;
    } else {
      // Si está abierto, CERRAR
      myServo_2.write(CLOSE_ANGLE_D);
      tercer_servo_abierto = false;
    }
    // Anti-rebote: Pequeño delay para evitar múltiples cambios con una sola presión
    delay(300); 
  }

  // ==========================================
  // SISTEMA 3: CONTROL AUTOMÁTICO DE VENTANA DE LLUVIA
  // Usa HISTÉRESIS para evitar oscilaciones rápidas
  // ==========================================

  // DECISIÓN DE CERRAR VENTANA (se detecta lluvia)
  // Solo actúa si ventana está abierta Y el valor está por debajo del umbral bajo
  if (valorHumedad < UMBRAL_LLUVIA_BAJO && ventana_lluvia_abierta == true) {
    myServo_1.write(CLOSE_ANGLE_W);
    ventana_lluvia_abierta = false; // Actualizar estado
    // Mostrar estado en pantalla (primera línea)
    lcd.setCursor(0, 0); 
    lcd.print("Lluvia: ABIERTO ");
  }
  
  // DECISIÓN DE ABRIR VENTANA (ya no hay lluvia)
  // Solo actúa si ventana está cerrada Y el valor está por encima del umbral alto
  else if (valorHumedad > UMBRAL_LLUVIA_ALTO && ventana_lluvia_abierta == false) {
    myServo_1.write(OPEN_ANGLE_W);
    ventana_lluvia_abierta = true; // Actualizar estado
    // Mostrar estado en pantalla (primera línea)
    lcd.setCursor(0, 0); 
    lcd.print("Lluvia: CERRADO ");
  }
  
  delay(5);
}
