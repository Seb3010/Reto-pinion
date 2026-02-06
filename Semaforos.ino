/*
 * =======================================================================================
 * SISTEMA DE CONTROL DE SEMÁFOROS - RETO PIÑÓN
 * =======================================================================================
 * 
 * DESCRIPCIÓN:
 * Sistema de control automatizado para cruce de avenidas principales:
 * - Av. Patria (eje Norte-Sur)
 * - Av. Mariano Otero (eje Este-Oeste)
 * 
 * CARACTERÍSTICAS PRINCIPALES:
 * - Gestiona 4 semáforos independientes (12 LEDs en total)
 * - Implementa secuencia de seguridad con fases alternadas
 * - Previene colisiones mediante tiempo de seguridad entre fases
 * - Operación continua y automática
 * 
 * LÓGICA DE OPERACIÓN:
 * - FASE A: Av. Patria en verde, Mariano Otero en rojo
 * - FASE B: Mariano Otero en verde, Av. Patria en rojo
 * - Transición por amarillo + tiempo de seguridad rojo total
 * 
 * CREADO: Enero 2026 - Reto Piñón
 * VERSIÓN: 2.0 - Con comentarios mejorados
 * =======================================================================================
 */

/*
 * =======================================================================================
 * DEFINICIÓN DE PINES - CONFIGURACIÓN DE HARDWARE
 * =======================================================================================
 * 
 * NOTA: Cada semáforo requiere 3 pines digitales independientes
 * - ROJO: Indica detención obligatoria (STOP)
 * - AMARILLO: Precaución, preparación para detenerse
 * - VERDE: Permite el avance de vehículos
 */

// SEMÁFORO 1: Av. Patria (Sentido Norte - dirección hacia arriba en el mapa)
const int s1_rojo = 2;      // LED Rojo - Semáforo 1
const int s1_amarillo = 3;  // LED Amarillo - Semáforo 1  
const int s1_verde = 4;     // LED Verde - Semáforo 1

// SEMÁFORO 2: Av. Patria (Sentido Sur - dirección hacia abajo en el mapa)
const int s2_rojo = 5;      // LED Rojo - Semáforo 2
const int s2_amarillo = 6;  // LED Amarillo - Semáforo 2
const int s2_verde = 7;     // LED Verde - Semáforo 2

// SEMÁFORO 3: Av. Mariano Otero (Sentido Este - dirección hacia la derecha)
const int s3_rojo = 8;      // LED Rojo - Semáforo 3
const int s3_amarillo = 9;  // LED Amarillo - Semáforo 3
const int s3_verde = 10;    // LED Verde - Semáforo 3

// SEMÁFORO 4: Av. Mariano Otero (Sentido Oeste - dirección hacia la izquierda)
const int s4_rojo = 11;     // LED Rojo - Semáforo 4
const int s4_amarillo = 12; // LED Amarillo - Semáforo 4
const int s4_verde = 13;    // LED Verde - Semáforo 4

/*
 * =======================================================================================
 * CONFIGURACIÓN DE TIEMPOS - PARÁMETROS DE OPERACIÓN
 * =======================================================================================
 * 
 * Estos tiempos definen la duración de cada fase del ciclo de semáforos.
 * Pueden ajustarse según el flujo vehicular o requerimientos específicos.
 */

const int tiempo_verde = 5000;      // Duración luz verde: 5 segundos (5000 ms)
                                   // Permite el paso de vehículos en la vía activa

const int tiempo_amarillo = 2000;   // Duración luz amarilla: 2 segundos (2000 ms)
                                   // Fase de transición y precaución

const int tiempo_seguridad = 1000;  // Tiempo de seguridad: 1 segundo (1000 ms)
                                   // Todos los semáforos en rojo para evitar colisiones
                                   // Permite que los últimos vehículos crucen completamente

/*
 * =======================================================================================
 * FUNCIÓN SETUP - CONFIGURACIÓN INICIAL DEL SISTEMA
 * =======================================================================================
 * 
 * Esta función se ejecuta una sola vez al iniciar el sistema.
 * Realiza toda la configuración inicial de hardware y establece
 * un estado seguro antes de comenzar la operación normal.
 */
void setup() {
  // CONFIGURACIÓN DE PINES DIGITALES
  // Todos los pines de LED se configuran como OUTPUT (salida digital)
  // Esto permite que Arduino controle el estado (encendido/apagado) de cada LED
  
  // Configuración Semáforo 1 (Av. Patria Norte)
  pinMode(s1_rojo, OUTPUT);    // Pin 2: Control LED rojo semáforo 1
  pinMode(s1_amarillo, OUTPUT); // Pin 3: Control LED amarillo semáforo 1
  pinMode(s1_verde, OUTPUT);   // Pin 4: Control LED verde semáforo 1
  
  // Configuración Semáforo 2 (Av. Patria Sur)
  pinMode(s2_rojo, OUTPUT);    // Pin 5: Control LED rojo semáforo 2
  pinMode(s2_amarillo, OUTPUT); // Pin 6: Control LED amarillo semáforo 2
  pinMode(s2_verde, OUTPUT);   // Pin 7: Control LED verde semáforo 2
  
  // Configuración Semáforo 3 (Av. Mariano Otero Este)
  pinMode(s3_rojo, OUTPUT);    // Pin 8: Control LED rojo semáforo 3
  pinMode(s3_amarillo, OUTPUT); // Pin 9: Control LED amarillo semáforo 3
  pinMode(s3_verde, OUTPUT);   // Pin 10: Control LED verde semáforo 3
  
  // Configuración Semáforo 4 (Av. Mariano Otero Oeste)
  pinMode(s4_rojo, OUTPUT);    // Pin 11: Control LED rojo semáforo 4
  pinMode(s4_amarillo, OUTPUT); // Pin 12: Control LED amarillo semáforo 4
  pinMode(s4_verde, OUTPUT);   // Pin 13: Control LED verde semáforo 4
  
  // INICIALIZACIÓN SEGURA DEL SISTEMA
  // Establece todos los semáforos en ROJO antes de comenzar la operación
  // Esto previene accidentes si el sistema se enciende en medio de una fase
  todosRojo();  // Llama a función que enciende todos los LEDs rojos
  
  delay(2000); // Espera 2 segundos para estabilización y visualización inicial
}

/*
 * =======================================================================================
 * FUNCIÓN LOOP - CICLO PRINCIPAL DE OPERACIÓN
 * =======================================================================================
 * 
 * Esta función se ejecuta continuamente en un bucle infinito después de setup().
 * Implementa el ciclo completo de semáforos con dos fases principales y 
 * transiciones seguras para evitar colisiones en el cruce.
 * 
 * SECUENCIA COMPLETA:
 * FASE A → AMARILLO → ROJO TOTAL → FASE B → AMARILLO → ROJO TOTAL → (repetir)
 */
void loop() {
  /*
   * ===================================================================================
   * FASE A: AV. PATRIA ACTIVA (S1 y S2 en VERDE)
   * ===================================================================================
   * 
   * Durante esta fase:
   * - Av. Patria (norte-sur) tiene luz VERDE en ambos sentidos
   * - Av. Mariano Otero (este-oeste) permanece en ROJO
   * - Los vehículos pueden cruzar por Av. Patria
   */
  
  // 1. ACTIVAR SEMÁFOROS DE AV. PATRIA EN VERDE
  // Semáforo 1 (Norte): Cambiar de ROJO a VERDE
  digitalWrite(s1_rojo, LOW);     // Apagar LED rojo
  digitalWrite(s1_verde, HIGH);    // Encender LED verde
  
  // Semáforo 2 (Sur): Cambiar de ROJO a VERDE  
  digitalWrite(s2_rojo, LOW);     // Apagar LED rojo
  digitalWrite(s2_verde, HIGH);    // Encender LED verde
  
  // 2. MANTENER AV. MARIANO OTERO EN ROJO (prevención de colisiones)
  // Estos semáforos ya estaban en rojo desde la fase anterior
  // Se mantienen encendidos para seguridad
  digitalWrite(s3_rojo, HIGH);     // Semáforo 3 (Este) en ROJO
  digitalWrite(s4_rojo, HIGH);     // Semáforo 4 (Oeste) en ROJO
  
  delay(tiempo_verde); // Mantener esta configuración por 5 segundos
  
  /*
   * -----------------------------------------------------------------------------------
   * TRANSICIÓN DE AV. PATRIA: VERDE → AMARILLO → ROJO
   * -----------------------------------------------------------------------------------
   * Esta transición alerta a los conductores que deben prepararse para detenerse
   */
  
  // 3. PASAR A AMARILLO en semáforos de Av. Patria
  // Semáforo 1 (Norte): VERDE → AMARILLO
  digitalWrite(s1_verde, LOW);      // Apagar LED verde
  digitalWrite(s1_amarillo, HIGH);   // Encender LED amarillo
  
  // Semáforo 2 (Sur): VERDE → AMARILLO
  digitalWrite(s2_verde, LOW);      // Apagar LED verde  
  digitalWrite(s2_amarillo, HIGH);   // Encender LED amarillo
  
  delay(tiempo_amarillo); // Mantener amarillo por 2 segundos
  
  // 4. DETENER COMPLETAMENTE AV. PATRIA
  // Semáforo 1 (Norte): AMARILLO → ROJO
  digitalWrite(s1_amarillo, LOW); // Apagar LED amarillo
  digitalWrite(s1_rojo, HIGH);    // Encender LED rojo
  
  // Semáforo 2 (Sur): AMARILLO → ROJO  
  digitalWrite(s2_amarillo, LOW); // Apagar LED amarillo
  digitalWrite(s2_rojo, HIGH);    // Encender LED rojo
  
  delay(tiempo_seguridad); // Tiempo de seguridad: todos en rojo por 1 segundo
  
  /*
   * ===================================================================================
   * FASE B: AV. MARIANO OTERO ACTIVA (S3 y S4 en VERDE)
   * ===================================================================================
   * 
   * Durante esta fase:
   * - Av. Mariano Otero (este-oeste) tiene luz VERDE en ambos sentidos
   * - Av. Patria (norte-sur) permanece en ROJO
   * - Los vehículos pueden cruzar por Av. Mariano Otero
   */
  
  // 5. ACTIVAR SEMÁFOROS DE AV. MARIANO OTERO EN VERDE
  // Semáforo 3 (Este): Cambiar de ROJO a VERDE
  digitalWrite(s3_rojo, LOW);     // Apagar LED rojo
  digitalWrite(s3_verde, HIGH);    // Encender LED verde
  
  // Semáforo 4 (Oeste): Cambiar de ROJO a VERDE
  digitalWrite(s4_rojo, LOW);     // Apagar LED rojo
  digitalWrite(s4_verde, HIGH);    // Encender LED verde
  
  // NOTA: Av. Patria ya permanece en ROJO desde el paso anterior
  
  delay(tiempo_verde); // Mantener esta configuración por 5 segundos
  
  /*
   * -----------------------------------------------------------------------------------
   * TRANSICIÓN DE AV. MARIANO OTERO: VERDE → AMARILLO → ROJO
   * -----------------------------------------------------------------------------------
   * Transición simétrica a la de Av. Patria para completar el ciclo
   */
  
  // 6. PASAR A AMARILLO en semáforos de Av. Mariano Otero
  // Semáforo 3 (Este): VERDE → AMARILLO
  digitalWrite(s3_verde, LOW);      // Apagar LED verde
  digitalWrite(s3_amarillo, HIGH);   // Encender LED amarillo
  
  // Semáforo 4 (Oeste): VERDE → AMARILLO
  digitalWrite(s4_verde, LOW);      // Apagar LED verde
  digitalWrite(s4_amarillo, HIGH);   // Encender LED amarillo
  
  delay(tiempo_amarillo); // Mantener amarillo por 2 segundos
  
  // 7. DETENER COMPLETAMENTE AV. MARIANO OTERO
  // Semáforo 3 (Este): AMARILLO → ROJO
  digitalWrite(s3_amarillo, LOW); // Apagar LED amarillo
  digitalWrite(s3_rojo, HIGH);    // Encender LED rojo
  
  // Semáforo 4 (Oeste): AMARILLO → ROJO
  digitalWrite(s4_amarillo, LOW); // Apagar LED amarillo
  digitalWrite(s4_rojo, HIGH);    // Encender LED rojo
  
  delay(tiempo_seguridad); // Tiempo de seguridad: todos en rojo por 1 segundo
  
  // El ciclo completo ahora se repetirá automáticamente al reiniciar loop()
}

/*
 * =======================================================================================
 * FUNCIÓN todosRojo() - ESTADO DE EMERGENCIA Y SEGURIDAD
 * =======================================================================================
 * 
 * DESCRIPCIÓN:
 * Función de seguridad crítica que establece TODOS los semáforos en ROJO.
 * Este estado detiene completamente el tráfico en todas las direcciones.
 * 
 * USOS:
 * 1. Inicialización segura del sistema (evita arranque en estado peligroso)
 * 2. Situaciones de emergencia (accidente, obras, etc.)
 * 3. Modo manual de control de tráfico
 * 4. Transiciones entre fases (prevención de colisiones)
 * 
 * IMPORTANCIA:
 * Es la función más crítica para la seguridad del sistema de tráfico.
 * Cualquier falla en esta función podría resultar en accidentes graves.
 */
void todosRojo() {
  // SEMÁFORO 1 (Av. Patria Norte)
  digitalWrite(s1_verde, LOW);     // Apagar LED verde
  digitalWrite(s1_amarillo, LOW);   // Apagar LED amarillo  
  digitalWrite(s1_rojo, HIGH);      // Encender LED rojo
  
  // SEMÁFORO 2 (Av. Patria Sur)
  digitalWrite(s2_verde, LOW);     // Apagar LED verde
  digitalWrite(s2_amarillo, LOW);   // Apagar LED amarillo
  digitalWrite(s2_rojo, HIGH);      // Encender LED rojo
  
  // SEMÁFORO 3 (Av. Mariano Otero Este)
  digitalWrite(s3_verde, LOW);     // Apagar LED verde
  digitalWrite(s3_amarillo, LOW);   // Apagar LED amarillo
  digitalWrite(s3_rojo, HIGH);      // Encender LED rojo
  
  // SEMÁFORO 4 (Av. Mariano Otero Oeste)
  digitalWrite(s4_verde, LOW);     // Apagar LED verde
  digitalWrite(s4_amarillo, LOW);   // Apagar LED amarillo
  digitalWrite(s4_rojo, HIGH);      // Encender LED rojo
  
  // RESULTADO: Todos los semáforos en rojo → tráfico detenido en todas las direcciones
}