# Reto Piñón - Maqueta de Ciudad Futurista

## Descripción del Proyecto

Este proyecto es una maqueta de ciudad futurista diseñada para el Reto Piñón, una competencia de 
innovación tecnológica. La maqueta integra tres sistemas principales de automatización y monitoreo
en tiempo real, demostrando conceptos de IoT (Internet de las Cosas), automatización residencial 
control de tráfico urbano.

## Sistemas del Proyecto

### 1. Sistema de Transporte Público (`Transporte_publico.ino`)
Monitoreo en tiempo real de transporte público a través de una interfaz web interactiva.

**Características:**
- Control de vehículo a través de joystick (pines 34 y 35)
- Visualización en tiempo real vía navegador web
- Conexión WiFi en modo Access Point (crea su propia red)
- Servidor WebSocket para comunicación bidireccional
- Actualización a 20 Hz (cada 50ms) para movimiento fluido
- Interfaz web embebida con HTML/CSS/JavaScript

**Cómo funciona:**
1. El ESP32 crea una red WiFi llamada "Auto_Veloz_ESP32"
2. Conecta un dispositivo (celular/PC) a esa red
3. Accede a la IP mostrada en el Monitor Serial (normalmente 192.168.4.1)
4. Mueve el joystick para controlar el bus en el mapa
5. La posición se actualiza en tiempo real en el navegador

**Especificaciones:**
- Velocidad máxima: 5 píxeles por actualización
- Rango de movimiento: 0-380 píxeles (área de 400x400)
- Frecuencia de actualización: 50ms
- Protocolo de comunicación: WebSocket en puerto 81

### 2. Sistema de Semáforos (`Semaforos.ino`)
Control automatizado de cruce en Av. Patria y Av. Mariano Otero.

**Características:**
- 4 semáforos independientes (12 LEDs en total)
- Secuencia de seguridad con fases alternas
- Tiempos configurables para cada fase
- Función de emergencia para detener todo el tráfico
- Sistema de seguridad con tiempo de espera entre fases

**Configuración de pines:**
- **Semáforo 1 (Av. Patria Norte):** Rojo=2, Amarillo=3, Verde=4
- **Semáforo 2 (Av. Patria Sur):** Rojo=5, Amarillo=6, Verde=7
- **Semáforo 3 (Av. Mariano Otero Este):** Rojo=8, Amarillo=9, Verde=10
- **Semáforo 4 (Av. Mariano Otero Oeste):** Rojo=11, Amarillo=12, Verde=13

**Secuencia de funcionamiento:**
1. **Fase A:** Av. Patria en VERDE (5s) → AMARILLO (2s) → ROJO (1s seguridad)
2. **Fase B:** Av. Mariano Otero en VERDE (5s) → AMARILLO (2s) → ROJO (1s seguridad)
3. Repetir ciclo continuamente

**Tiempos configurables:**
- Luz verde: 5 segundos
- Luz amarilla: 2 segundos
- Tiempo de seguridad: 1 segundo

### 3. Smart House - Casa Inteligente (`Smart_house.ino`)
Sistema de automatización residencial con ventanas inteligentes.

**Características:**
- 3 servos para control de ventanas/puertas
- Pantalla LCD I2C para información en tiempo real
- Sensor de lluvia con sistema de histéresis
- Control manual con botones
- Modo automático por detección de lluvia
- Control tipo "toggle" (interruptor) con botón rojo

**Configuración de componentes:**
- **Servo 1 (Ventana Principal):** Pin 11, Botón Verde (Pin 6)
- **Servo 2 (Ventana de Lluvia):** Pin 3, Sensor de Lluvia (A1)
- **Servo 3 (Tercer Servo/Puerta):** Pin 10, Botón Rojo (Pin 5)
- **Pantalla LCD:** I2C dirección 0x27 (16x2)

**Modos de operación:**

**Sistema 1 - Ventana Principal (Manual):**
- Botón verde presionado: Ventana se CIERRA
- Botón verde soltado: Ventana se ABRE
- Control de tipo "mantenimiento" (posición depende del estado del botón)

**Sistema 2 - Tercer Servo/Puerta (Toggle):**
- Botón rojo: Funciona como interruptor (ON/OFF)
- Cada presión alterna entre ABIERTO y CERRADO
- Incluye anti-rebote de 300ms

**Sistema 3 - Ventana de Lluvia (Automático):**
- Sensor detecta humedad (0-1023)
- Sistema de histéresis evita oscilaciones:
  - Valor < 50: Cerrar ventana (lluvia detectada)
  - Valor > 150: Abrir ventana (seco)
  - Entre 50-150: Mantener estado actual
- Umbral de histéresis: 100 unidades de diferencia

**Ángulos de servos:**
- Ventana principal: ABIERTO=95°, CERRADO=180°
- Ventana lluvia: ABIERTO=95°, CERRADO=0°
- Tercer servo: ABIERTO=100°, CERRADO=180°

## Hardware Requerido

### Para Sistema de Transporte Público
- ESP32 (módulo con WiFi)
- Joystick analógico (2 ejes)
- Cable USB para programación

### Para Sistema de Semáforos
- Arduino Uno o compatible
- 12 LEDs (4 rojos, 4 amarillos, 4 verdes)
- 12 resistencias de 220Ω (una por LED)
- Protoboard y cables jumper

### Para Smart House
- Arduino Uno o compatible
- 3 servos (SG90 o similares)
- Pantalla LCD 16x2 con módulo I2C
- Sensor de lluvia (analógico)
- 2 botones pulsadores
- Protoboard y cables jumper

**Total de componentes:**
- 2x Arduino/ESP32
- 3x Servos
- 1x Pantalla LCD I2C
- 12x LEDs
- 12x Resistencias 220Ω
- 1x Joystick analógico
- 1x Sensor de lluvia
- 2x Botones pulsadores
- Protoboard y cables

## Estructura de Archivos

```
Reto-pinion/
├── Transporte_publico.ino    # Sistema de monitoreo de transporte
├── Semaforos.ino            # Sistema de control de semáforos
├── Smart_house.ino          # Sistema de casa inteligente
└── README.md               # Esta documentación
```

## Instrucciones de Uso

### Sistema de Transporte Público

1. Cargar `Transporte_publico.ino` en el ESP32
2. Abrir Monitor Serial (115200 baud)
3. Esperar a que muestre la IP (ej: 192.168.4.1)
4. Conectar dispositivo a WiFi "Auto_Veloz_ESP32" (clave: 12345678)
5. Abrir navegador e ingresar la IP mostrada
6. Mover joystick para controlar el bus

### Sistema de Semáforos

1. Conectar 4 semáforos según el diagrama de pines
2. Cargar `Semaforos.ino` en Arduino
3. Verificar que los LEDs se inicien en rojo (seguridad)
4. Observar el ciclo automático de fases
5. Ajustar tiempos si es necesario

### Smart House

1. Conectar servos, LCD, sensor y botones según configuración
2. Cargar `Smart_house.ino` en Arduino
3. Verificar mensaje de bienvenida en LCD
4. Probar:
   - Botón verde: control ventana principal (presionar/mantener)
   - Botón rojo: control tercer servo (interruptor)
   - Sensor de lluvia: simular con agua/objeto húmedo
5. Observar LCD para confirmar estados

## Características Técnicas

### Comunicación
- **Transporte Público:** WiFi (Access Point) + WebSocket
- **Semaforos:** Comunicación GPIO directa
- **Smart House:** Comunicación GPIO directa + I2C (LCD)

### Frecuencias de Operación
- Transporte Público: 20 Hz (50ms)
- Semaforos: ~8 segundos por ciclo completo
- Smart House: 200 Hz (5ms)

### Algoritmos Implementados
- **Histéresis:** Evita oscilaciones en sensores analógicos
- **Toggle:** Control de interruptor con botones
- **Mantenimiento:** Control de posición por estado del botón
- **Broadcast:** Envío de datos a múltiples clientes (WebSocket)
- **Fases alternas:** Secuencia de seguridad en semáforos

### Seguridad
- Todos los semáforos inician en ROJO
- Tiempo de seguridad entre fases (1s)
- Sistema de histéresis evita errores de sensor

### Eficiencia
- Uso de PROGMEM para ahorrar RAM (Transporte)
- Sistema de actualización optimizado (50ms)
- Envío de datos solo cuando hay cambios

### Experiencia de Usuario
- Interfaz web intuitiva (Transporte)
- Feedback visual en tiempo real (LCD)
- Controles físicos intuitivos (botones)

## Posibles Mejoras Futuras

1. **Transporte Público:**
   - Agregar múltiples vehículos
   - Implementar rutas predefinidas
   - Sistema de detección de colisiones

2. **Semaforos:**
   - Sensores de vehículos para priorización
   - Modo semáforo peatonal
   - Sistema de emergencia manual

3. **Smart House:**
   - Integración con WiFi para control remoto
   - Más sensores (temperatura, luz)
   - Sistema de programación horaria

## Licencia

TruckTracker es un proyecto de codigo abierto disponible para su uso y modificacion. EL copyright y la autoria del material intelctual del proyecto pertencen al Equipo Bongos. Culaquier uso o distribucion debe conservar este aviso y dar credito al Equipo bongos

**Versión:** 1.0  
**Fecha:** Enero 2026  
**Estado:** Funcional
