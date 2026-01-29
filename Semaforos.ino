/* SISTEMA DE SEMÁFOROS: Control de cruce en Av. Patria y Av. Mariano Otero
 * Gestiona 4 semáforos independientes para evitar colisiones en el cruce
 * Implementa secuencia de seguridad con fases alternas
 */

// DEFINICIÓN DE PINES: Asignación de pines digitales para cada LED de los semáforos
// Cada semáforo tiene 3 luces: ROJO (STOP), AMARILLO (PRECAUCIÓN), VERDE (AVANCE)

// SEMÁFORO 1: Av. Patria (Sentido Norte - hacia arriba)
const int s1_rojo = 2;
const int s1_amarillo = 3;
const int s1_verde = 4;

// SEMÁFORO 2: Av. Patria (Sentido Sur - hacia abajo)
const int s2_rojo = 5;
const int s2_amarillo = 6;
const int s2_verde = 7;

// SEMÁFORO 3: Av. Mariano Otero (Sentido Este - derecha)
const int s3_rojo = 8;
const int s3_amarillo = 9;
const int s3_verde = 10;

// SEMÁFORO 4: Av. Mariano Otero (Sentido Oeste - izquierda)
const int s4_rojo = 11;
const int s4_amarillo = 12;
const int s4_verde = 13;

// TIMERS DE CONTROL: Duración de cada fase del semáforo
const int tiempo_verde = 5000;      // 5 segundos de luz verde (vehículos avanzan)
const int tiempo_amarillo = 2000;   // 2 segundos de luz amarillo (preparación al stop)
const int tiempo_seguridad = 1000;  // 1 segundo de todos en rojo (seguridad entre fases)

void setup() {
  // CONFIGURACIÓN DE PINES: Definir los 12 pines como SALIDA
  // Cada semáforo necesita controlar 3 LEDs independientes
  // Semáforo 1 (Av. Patria Norte)
  pinMode(s1_rojo, OUTPUT); pinMode(s1_amarillo, OUTPUT); pinMode(s1_verde, OUTPUT);
  // Semáforo 2 (Av. Patria Sur)
  pinMode(s2_rojo, OUTPUT); pinMode(s2_amarillo, OUTPUT); pinMode(s2_verde, OUTPUT);
  // Semáforo 3 (Av. Mariano Otero Este)
  pinMode(s3_rojo, OUTPUT); pinMode(s3_amarillo, OUTPUT); pinMode(s3_verde, OUTPUT);
  // Semáforo 4 (Av. Mariano Otero Oeste)
  pinMode(s4_rojo, OUTPUT); pinMode(s4_amarillo, OUTPUT); pinMode(s4_verde, OUTPUT);
  
  // INICIALIZACIÓN DE SEGURIDAD: Todos los semáforos en ROJO
  // Evita accidentes si el sistema se enciende en medio de una fase
  todosRojo();
  delay(2000);
}

void loop() {
  // ==========================================
  // FASE A: AV. PATRIA AVANZA (S1 y S2 Verdes)
  // Vehículos en Av. Patria pueden circular, Mariano Otero debe detenerse
  // ==========================================
  
  // 1. ACTIVAR LUZ VERDE en Av. Patria (ambos sentidos)
  // Apagar rojo y encender verde
  digitalWrite(s1_rojo, LOW);
  digitalWrite(s1_verde, HIGH);
  
  digitalWrite(s2_rojo, LOW);
  digitalWrite(s2_verde, HIGH);
  
  // Mantener Av. Mariano Otero en ROJO (prevención de colisiones)
  digitalWrite(s3_rojo, HIGH);
  digitalWrite(s4_rojo, HIGH);
  
  delay(tiempo_verde);
  
  // 2. TRANSICIÓN A AMARILLO en Av. Patria
  // Preparación para detener el tráfico
  digitalWrite(s1_verde, LOW);
  digitalWrite(s1_amarillo, HIGH);
  
  digitalWrite(s2_verde, LOW);
  digitalWrite(s2_amarillo, HIGH);
  
  delay(tiempo_amarillo);
  
  // 3. DETENCIÓN COMPLETA en Av. Patria
  // Activar rojo y apagar amarillo para seguridad total
  digitalWrite(s1_amarillo, LOW);
  digitalWrite(s1_rojo, HIGH);
  
  digitalWrite(s2_amarillo, LOW);
  digitalWrite(s2_rojo, HIGH);
  
  delay(tiempo_seguridad);
  
  // ================================================
  // FASE B: AV. MARIANO OTERO AVANZA (S3 y S4 Verdes)
  // Ahora Mariano Otero tiene paso, Patria debe detenerse
  // ================================================
  
  // 4. ACTIVAR LUZ VERDE en Av. Mariano Otero
  // Apagar rojos y encender verdes
  digitalWrite(s3_rojo, LOW);
  digitalWrite(s3_verde, HIGH);
  
  digitalWrite(s4_rojo, LOW);
  digitalWrite(s4_verde, HIGH);
  
  // Av. Patria ya permanece en ROJO desde el paso anterior
  
  delay(tiempo_verde);
  
  // 5. TRANSICIÓN A AMARILLO en Av. Mariano Otero
  digitalWrite(s3_verde, LOW);
  digitalWrite(s3_amarillo, HIGH);
  
  digitalWrite(s4_verde, LOW);
  digitalWrite(s4_amarillo, HIGH);
  
  delay(tiempo_amarillo);
  
  // 6. DETENCIÓN COMPLETA en Av. Mariano Otero
  digitalWrite(s3_amarillo, LOW);
  digitalWrite(s3_rojo, HIGH);
  
  digitalWrite(s4_amarillo, LOW);
  digitalWrite(s4_rojo, HIGH);
  
  delay(tiempo_seguridad);
}

// FUNCIÓN DE EMERGENCIA: Poner TODOS los semáforos en ROJO
// Útil para inicialización segura o situaciones de emergencia
// Evita cualquier circulación en el cruce
void todosRojo() {
  // Semáforo 1: Apagar verde/amarillo, encender rojo
  digitalWrite(s1_verde, LOW); digitalWrite(s1_amarillo, LOW); digitalWrite(s1_rojo, HIGH);
  // Semáforo 2: Apagar verde/amarillo, encender rojo
  digitalWrite(s2_verde, LOW); digitalWrite(s2_amarillo, LOW); digitalWrite(s2_rojo, HIGH);
  // Semáforo 3: Apagar verde/amarillo, encender rojo
  digitalWrite(s3_verde, LOW); digitalWrite(s3_amarillo, LOW); digitalWrite(s3_rojo, HIGH);
  // Semáforo 4: Apagar verde/amarillo, encender rojo
  digitalWrite(s4_verde, LOW); digitalWrite(s4_amarillo, LOW); digitalWrite(s4_rojo, HIGH);
}