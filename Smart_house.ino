/*
 * =======================================================================================
 * SMART HOUSE - SISTEMA DE AUTOMATIZACIÓN RESIDENCIAL - RETO PIÑÓN
 * =======================================================================================
 * 
 * DESCRIPCIÓN:
 * Sistema de automatización para vivienda inteligente que integra:
 * - Control automático de ventanas según condiciones climáticas
 * - Control manual de elementos mediante botones físicos
 * - Monitoreo visual en tiempo real mediante display LCD
 * - Sistema de detección de lluvia con algoritmo de histéresis
 * 
 * COMPONENTES PRINCIPALES:
 * - 3 servomotores para control de ventanas/puertas
 * - Pantalla LCD I2C para visualización de estados
 * - Sensor de lluvia analógico para detección climática
 * - 2 botones pulsadores para control manual
 * 
 * MODOS DE OPERACIÓN:
 * 1. VENTANA PRINCIPAL: Control manual directo (botón verde)
 * 2. VENTANA DE LLUVIA: Control automático por sensor con histéresis
 * 3. TERCER SERVO: Control tipo toggle/interruptor (botón rojo)
 * 
 * CREADO: Enero 2026 - Reto Piñón
 * VERSIÓN: 2.0 - Con comentarios mejorados
 * =======================================================================================
 */

// LIBRERÍAS EXTERNAS
#include <LiquidCrystal_I2C.h>  // Comunicación con display LCD vía protocolo I2C
#include <Servo.h>             // Control de servomotores (PWM)

/*
 * =======================================================================================
 * DECLARACIÓN DE OBJETOS Y COMPONENTES
 * =======================================================================================
 */

// Display LCD I2C de 16 caracteres x 2 líneas
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Dirección I2C: 0x27, 16 columnas, 2 filas

// Servomotores para control de ventanas y puertas
Servo myServo;      // Servo 1: Ventana principal (Control Manual - Botón Verde)
Servo myServo_1;    // Servo 2: Ventana de lluvia (Control Automático - Sensor)
Servo myServo_2;    // Servo 3: Tercer elemento (Control Toggle - Botón Rojo)

/*
 * =======================================================================================
 * DEFINICIÓN DE PINES DE CONEXIÓN
 * =======================================================================================
 * 
 * Los pines digitales controlan servos y botones.
 * Los pines analógicos leen sensores con valores continuos (0-1023).
 */

// Pines para BOTONES PULSADORES (con resistencia PULLUP interna)
const int BUTTON_PIN = 6;       // Pin 6: Botón VERDE (control ventana principal)
const int BUTTON_PIN_RED = 5;   // Pin 5: Botón ROJO (control tercer servo)

// Pines para SERVOMOTORES (requieren señal PWM)
const int SERVO_PIN = 11;       // Pin 11: Servo 1 - Ventana principal
const int SERVO_W_PIN = 3;      // Pin 3: Servo 2 - Ventana de lluvia
const int SERVO_D_PIN = 10;     // Pin 10: Servo 3 - Tercer elemento/puerta

// Pin para SENSOR ANALÓGICO
const int SENSOR_RAIN_PIN = A1;  // Pin A1: Sensor de lluvia (valores 0-1023)

/*
 * =======================================================================================
 * CONFIGURACIÓN DE ÁNGULOS PARA SERVOMOTORES
 * =======================================================================================
 * 
 * NOTA: Los ángulos pueden variar según el modelo de servo y montaje mecánico.
 * Se recomienda calibrar estos valores según la instalación específica.
 */

// SERVOMOTOR 1: Ventana principal
const int OPEN_ANGLE = 95;       // Ángulo de apertura: 95° (ventana abierta)
const int CLOSE_ANGLE = 180;     // Ángulo de cierre: 180° (ventana cerrada)

// SERVOMOTOR 2: Ventana de lluvia (montaje invertido)
const int OPEN_ANGLE_W = 95;     // Ángulo de apertura: 95° (ventana abierta)
const int CLOSE_ANGLE_W = 0;      // Ángulo de cierre: 0° (ventana cerrada)
                                 // NOTA: Este servo tiene montaje inverso al primero

// SERVOMOTOR 3: Tercer elemento/puerta
const int OPEN_ANGLE_D = 100;    // Ángulo de apertura: 100° (elemento abierto)
const int CLOSE_ANGLE_D = 180;   // Ángulo de cierre: 180° (elemento cerrado)

/*
 * =======================================================================================
 * CONFIGURACIÓN DE UMBRALES - SISTEMA DE HISTÉRESIS
 * =======================================================================================
 * 
 * EXPLICACIÓN DEL SISTEMA DE HISTÉRESIS:
 * El sistema de histéresis evita oscilaciones rápidas del servo cuando el sensor
 * de lluvia está cerca del umbral de detección (ej: cuando llovizna).
 * 
 * FUNCIONAMIENTO:
 * - Si valor < UMBRAL_LLUVIA_BAJO: Cerrar ventana (lluvia detectada)
 * - Si valor > UMBRAL_LLUVIA_ALTO: Abrir ventana (ambiente seco)
 * - Si UMBRAL_BAJO < valor < UMBRAL_ALTO: Mantener estado actual
 * 
 * BENEFICIOS:
 * - Previene apertura/cierre constante (efecto "temblor")
 * - Protege el servo de desgaste prematuro
 * - Proporciona comportamiento más natural y estable
 */

const int UMBRAL_LLUVIA_BAJO = 50;   // Umbral inferior: 50 (detectar lluvia)
                                    // Valor < 50 = cerrar ventana por seguridad

const int UMBRAL_LLUVIA_ALTO = 150;  // Umbral superior: 150 (detectar seco)
                                    // Valor > 150 = abrir ventana normalmente
                                    // Diferencia de 100 unidades entre umbrales

/*
 * =======================================================================================
 * VARIABLES DE ESTADO DEL SISTEMA
 * =======================================================================================
 * 
 * Estas variables mantienen el estado actual de cada componente y permiten
 * tomar decisiones lógicas basadas en estados previos.
 * 
 * IMPORTANCIA:
 * - Evitan cambiar posición del servo innecesariamente
 * - Implementan lógica de memoria para sistemas tipo toggle
 * - Permite implementar algoritmos complejos como histéresis
 */

// Estado de la ventana de lluvia (SERVOMOTOR 2)
bool ventana_lluvia_abierta = true;  // Estado inicial: VENTANA ABIERTA
                                     // true = abierta, false = cerrada
                                     // Util para algoritmo de histéresis

// Estado del tercer elemento/puerta (SERVOMOTOR 3)
bool tercer_servo_abierto = false;   // Estado inicial: ELEMENTO CERRADO
                                     // true = abierto, false = cerrado
                                     // Esencial para funcionamiento tipo toggle/interruptor

/*
 * =======================================================================================
 * FUNCIÓN SETUP - CONFIGURACIÓN INICIAL DEL SISTEMA
 * =======================================================================================
 * 
 * Esta función se ejecuta una sola vez al iniciar el sistema.
 * Realiza toda la configuración inicial de hardware y establece
 * los estados iniciales de cada componente antes de comenzar la operación.
 */
void setup() {
  // CONFIGURACIÓN INICIAL DEL DISPLAY LCD
  lcd.init();                    // Inicializa la comunicación I2C con el display
  lcd.backlight();              // Enciende la retroiluminación del LCD
  lcd.setCursor(0, 0);          // Posiciona cursor en columna 0, fila 0
  lcd.print("Bienvenido");       // Mensaje de bienvenida en primera línea

  // CONFIGURACIÓN DE PINES DIGITALES
  // INPUT_PULLUP habilita resistencia interna de pull-up (~20kΩ)
  // Esto mantiene el pin en ALTO (HIGH) cuando el botón no está presionado
  pinMode(BUTTON_PIN, INPUT_PULLUP);     // Pin 6: Botón VERDE
  pinMode(BUTTON_PIN_RED, INPUT_PULLUP); // Pin 5: Botón ROJO

  // CONFIGURACIÓN DE SERVOMOTORES
  // Conecta cada objeto servo a su pin físico correspondiente
  myServo.attach(SERVO_PIN);     // Servo 1 al pin 11 (Ventana principal)
  myServo_1.attach(SERVO_W_PIN);  // Servo 2 al pin 3 (Ventana de lluvia)
  myServo_2.attach(SERVO_D_PIN);  // Servo 3 al pin 10 (Tercer elemento/puerta)

  // ESTABLECER POSICIONES INICIALES DE SERVOS
  myServo.write(OPEN_ANGLE);         // Ventana principal: posición abierta (95°)
  myServo_1.write(OPEN_ANGLE_W);      // Ventana lluvia: posición abierta (95°)
  myServo_2.write(CLOSE_ANGLE_D);     // Tercer elemento: posición cerrada (180°)

  delay(1500);                       // Espera 1.5 segundos para mostrar mensaje
  lcd.clear();                       // Limpia pantalla para mostrar información en loop
}

/*
 * =======================================================================================
 * FUNCIÓN LOOP - CICLO PRINCIPAL DE OPERACIÓN
 * =======================================================================================
 * 
 * Esta función se ejecuta continuamente en un bucle infinito después de setup().
 * Implementa el control en tiempo real de los tres sistemas independientes:
 * 1. Ventana principal (control manual directo)
 * 2. Tercer elemento (control tipo toggle/interruptor)
 * 3. Ventana de lluvia (control automático con histéresis)
 * 
 * FRECUENCIA DE ACTUALIZACIÓN: ~200Hz (cada 5ms)
 * Esto proporciona respuesta rápida a las entradas del usuario y cambios ambientales.
 */
void loop() {
  // LECTURA DE ENTRADAS (sensores y botones)
  int buttonState = digitalRead(BUTTON_PIN);        // Leer estado botón verde (HIGH=suelto, LOW=presionado)
  int buttonState_red = digitalRead(BUTTON_PIN_RED); // Leer estado botón rojo
  int valorHumedad = analogRead(SENSOR_RAIN_PIN);    // Leer valor sensor lluvia (0-1023, donde mayor=seco)
  
  /*
   * ===================================================================================
   * SISTEMA 1: VENTANA PRINCIPAL - CONTROL MANUAL DIRECTO (SERVOMOTOR 1)
   * ===================================================================================
   * 
   * LÓGICA DE OPERACIÓN:
   * - Botón Verde PRESIONADO (LOW) → Ventana se CIERRA (posición de seguridad)
   * - Botón Verde SUELTO (HIGH) → Ventana se ABRE (posición normal)
   * 
   * CARACTERÍSTICAS:
   * - Control de tipo "mantenimiento" (posición depende del estado actual del botón)
   * - Sin memoria (vuelve a abrir automáticamente al soltar botón)
   * - Ideal para control manual directo de ventanas
   */
  if (buttonState == LOW) {
    // Botón VERDE presionado: MANTENER VENTANA CERRADA
    myServo.write(CLOSE_ANGLE);       // Mover servo a posición cerrada (180°)
    lcd.setCursor(0, 1);               // Posicionar cursor en columna 0, fila 1
    lcd.print("Manual: CERRADO ");     // Mostrar estado actual en LCD
  } 
  else {
    // Botón VERDE suelto: MANTENER VENTANA ABIERTA
    myServo.write(OPEN_ANGLE);        // Mover servo a posición abierta (95°)
    lcd.setCursor(0, 1);               // Posicionar cursor en columna 0, fila 1
    lcd.print("Manual: ABIERTO  ");    // Mostrar estado actual en LCD
  }

  /*
   * ===================================================================================
   * SISTEMA 2: TERCER ELEMENTO - CONTROL TOGGLE/INTERRUPTOR (SERVOMOTOR 3)
   * ===================================================================================
   * 
   * LÓGICA DE OPERACIÓN:
   * - Botón Rojo funciona como interruptor eléctrico (ON/OFF)
   * - Cada presión ALTERNA el estado (abierto ↔ cerrado)
   * - Estado se mantiene hasta próxima presión
   * 
   * CARACTERÍSTICAS:
   * - Control tipo toggle con memoria de estado
   * - Requiere debounce para evitar múltiples detecciones por una sola presión
   * - Ideal para puertas, luces, o elementos con dos estados estables
   */
  if (buttonState_red == LOW) {
    if (tercer_servo_abierto == false) {
      // Estado actual: CERRADO → Cambiar a ABIERTO
      myServo_2.write(OPEN_ANGLE_D);     // Mover a posición abierta (100°)
      tercer_servo_abierto = true;        // Actualizar variable de estado
    } 
    else {
      // Estado actual: ABIERTO → Cambiar a CERRADO
      myServo_2.write(CLOSE_ANGLE_D);     // Mover a posición cerrada (180°)
      tercer_servo_abierto = false;       // Actualizar variable de estado
    }
    delay(300); // ANTI-REBOTE: Esperar 300ms para evitar múltiples detecciones
                // Previnea que una sola presión del botón registre múltiples veces
  }

  /*
   * ===================================================================================
   * SISTEMA 3: VENTANA DE LLUVIA - CONTROL AUTOMÁTICO CON HISTÉRESIS (SERVOMOTOR 2)
   * ===================================================================================
   * 
   * EXPLICACIÓN DEL ALGORITMO DE HISTÉRESIS:
   * Este sistema previene oscilaciones del servo cuando el sensor está en zona ambigua
   * (valores intermedios entre seco y húmedo, como durante llovizna ligera).
   * 
   * LÓGICA DE DECISIÓN:
   * - valor < UMBRAL_BAJO (50) Y ventana abierta → CERRAR ventana (lluvia detectada)
   * - valor > UMBRAL_ALTO (150) Y ventana cerrada → ABRIR ventana (ambiente seco)
   * - ENTRE umbrales (50-150) → MANTENER estado actual (sin cambios)
   * 
   * BENEFICIOS:
   * - Evita apertura/cierre constante (efecto "temblor")
   * - Reduce desgaste mecánico del servo
   * - Comportamiento más natural y predecible
   */

  // CONDICIÓN 1: DETECTAR LLUVIA Y CERRAR VENTANA
  if (valorHumedad < UMBRAL_LLUVIA_BAJO && ventana_lluvia_abierta == true) {
    // Sensor detecta humedad (valor bajo) Y ventana está abierta
    myServo_1.write(CLOSE_ANGLE_W);        // Cerrar ventana (posición 0°)
    ventana_lluvia_abierta = false;        // Actualizar estado: ahora está cerrada
    lcd.setCursor(0, 0);                   // Posicionar cursor en columna 0, fila 0
    lcd.print("Lluvia: CERRADO ");         // Mostrar estado actual en LCD
  }
  
  // CONDICIÓN 2: DETECTAR AMBIENTE SECO Y ABRIR VENTANA
  else if (valorHumedad > UMBRAL_LLUVIA_ALTO && ventana_lluvia_abierta == false) {
    // Sensor detecta sequedad (valor alto) Y ventana está cerrada
    myServo_1.write(OPEN_ANGLE_W);         // Abrir ventana (posición 95°)
    ventana_lluvia_abierta = true;         // Actualizar estado: ahora está abierta
    lcd.setCursor(0, 0);                   // Posicionar cursor en columna 0, fila 0
    lcd.print("Lluvia: ABIERTO ");         // Mostrar estado actual en LCD
  }
  // NOTA: Si el valor está ENTRE los umbrales (50-150), no se hace nada
  // Esto mantiene el estado actual y previene oscilaciones
  
  delay(5); // Pequeña pausa de 5ms para estabilidad del sistema
           // Proporciona frecuencia de actualización ~200Hz
}
