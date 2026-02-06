/*
 * =======================================================================================
 * SISTEMA DE MONITOREO DE TRANSPORTE PÚBLICO - RETO PIÑÓN
 * =======================================================================================
 * 
 * DESCRIPCIÓN:
 * Sistema IoT de monitoreo en tiempo real para transporte público mediante
 * interfaz web interactiva controlada por joystick.
 * 
 * TECNOLOGÍAS UTILIZADAS:
 * - ESP32 con conectividad WiFi (modo Access Point)
 * - Servidor Web embebido (puerto 80) para interfaz HTML/CSS/JS
 * - Servidor WebSocket (puerto 81) para comunicación bidireccional en tiempo real
 * - Joystick analógico para control de posición
 * 
 * CARACTERÍSTICAS PRINCIPALES:
 * - Control de vehículo mediante joystick (2 ejes)
 * - Visualización en navegador web con animaciones fluidas
 * - Actualización a 20Hz (cada 50ms) para movimiento suave
 * - Interfaz responsiva con información de velocidad, coordenadas y marcha
 * - Sistema de múltiples clientes mediante broadcast WebSocket
 * 
 * MODO DE OPERACIÓN:
 * 1. ESP32 crea red WiFi "Auto_Veloz_ESP32"
 * 2. Cliente se conecta y accede a 192.168.4.1
 * 3. Mover joystick controla vehículo en mapa
 * 4. Posición se actualiza en tiempo real en todos los navegadores conectados
 * 
 * CREADO: Enero 2026 - Reto Piñón
 * VERSIÓN: 2.0 - Con comentarios mejorados
 * =======================================================================================
 */

// LIBRERÍAS ESPECÍFICAS PARA ESP32
#include <WiFi.h>              // Control de conectividad WiFi del ESP32
#include <WebServer.h>         // Servidor HTTP para páginas web
#include <WebSocketsServer.h>  // Servidor WebSocket para comunicación en tiempo real

/*
 * =======================================================================================
 * DEFINICIÓN DE PINES - ENTRADAS ANALÓGICAS
 * =======================================================================================
 * 
 * Los pines GPIO 34 y 35 del ESP32 son ADC (Conversores Analógico-Digital)
 * dedicados, ideales para leer sensores analógicos como joysticks.
 */

const int PIN_JOYSTICK_X = 34;  // Pin GPIO 34: Eje X del joystick (horizontal)
                                // Valores: 0 (izquierda) → 4095 (derecha)

const int PIN_JOYSTICK_Y = 35;  // Pin GPIO 35: Eje Y del joystick (vertical)  
                                // Valores: 0 (arriba) → 4095 (abajo)

/*
 * =======================================================================================
 * VARIABLES DE POSICIÓN Y MOVIMIENTO
 * =======================================================================================
 * 
 * Estas variables controlan la posición y movimiento del vehículo en el mapa.
 * El sistema opera en un área de 400x400 píxeles con márgenes de seguridad.
 */

int posX = 211;  // Posición actual X: inicialmente centrada (211/400)
int posY = 205;  // Posición actual Y: inicialmente centrada (205/400)

int oldX = 0;    // Posición previa X: para detectar cambios
int oldY = 0;    // Posición previa Y: para detectar cambios

/*
 * =======================================================================================
 * VARIABLES DE TIMING Y COMUNICACIÓN
 * =======================================================================================
 */

unsigned long lastUpdate = 0;    // Timestamp de última actualización (para control de frecuencia)

char bufferEnvio[64];            // Buffer temporal para enviar datos JSON por WebSocket
                                  // Formato: "{\"x\":211,\"y\":205}"

/*
 * =======================================================================================
 * OBJETOS DE SERVIDORES WEB
 * =======================================================================================
 * 
 * El sistema utiliza dos servidores simultáneamente:
 * - WebServer (puerto 80): Para servir la página HTML/CSS/JavaScript inicial
 * - WebSocketsServer (puerto 81): Para comunicación bidireccional en tiempo real
 */

WebServer server(80);                          // Servidor HTTP estándar para página web inicial
WebSocketsServer webSocket = WebSocketsServer(81); // Servidor WebSocket para actualizaciones en tiempo real
/*
 * =======================================================================================
 * PÁGINA WEB EMBEDIDA - HTML/CSS/JavaScript
 * =======================================================================================
 * 
 * NOTA: Esta página HTML está embebida directamente en el código Arduino.
 * Se almacena en PROGMEM (memoria flash) para no consumir RAM valiosa.
 * 
 * CARACTERÍSTICAS DE LA INTERFAZ:
 * - Diseño responsivo para móviles y escritorio
 * - Mapa con imagen de fondo del área monitoreada
 * - Vehículo animado que se mueve suavemente
 * - Panel de información con coordenadas, velocidad y marcha
 * - Conexión WebSocket con indicador de estado
 * - Animaciones CSS modernas y transiciones suaves
 */

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  
  <title>Reto Pinion - Monitor</title>
  <style>
    body {
      background-color: black;
      margin: 0;
      overflow: hidden;
      display: flex;
      flex-direction: column;
      height: 100vh;
      color: #333;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    }

    header {
      background-color: #00897b;
      box-shadow: 0 4px 15px rgba(0, 137, 123, 0.3);
      padding: 15px 20px;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 70px;
      position: relative;
      animation: slideInFromTop 0.6s cubic-bezier(0.25, 0.46, 0.45, 0.94);
    }

    h1 {
      margin: 0;
      font-size: 28px;
      color: #fff;
      font-weight: 700;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.2);
    }

    .status {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 6px 16px;
      border-radius: 20px;
      background: rgba(255, 255, 255, 0.2);
      font-size: 14px;
      font-weight: 600;
      margin-left: 20px;
      color: #fff;
    }

    .status::before {
      content: "";
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #ef4444;
      animation: pulse 2s infinite;
    }

    .status.connected::before {
      background: #ffffff;
    }

    @keyframes pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.5; transform: scale(1.2); }
    }

    @keyframes slideInFromTop {
      from {
        transform: translateY(-100%);
        opacity: 0;
      }
      to {
        transform: translateY(0);
        opacity: 1;
      }
    }

    @keyframes slideInFromLeft {
      from {
        transform: translateX(-100%);
        opacity: 0;
      }
      to {
        transform: translateX(0);
        opacity: 1;
      }
    }

    @keyframes scaleInFromCenter {
      from {
        transform: translate(-50%, -50%) scale(0);
        opacity: 0;
      }
      to {
        transform: translate(-50%, -50%) scale(1);
        opacity: 1;
      }
    }

    @keyframes slideInFromBottom {
      from {
        transform: translateY(100%);
        opacity: 0;
      }
      to {
        transform: translateY(0);
        opacity: 1;
      }
    }
    
    .map-container {
      display: flex;
      justify-content: center;
      align-items: center;
      flex: 1;
      padding: 10px;
      animation: slideInFromLeft 0.8s cubic-bezier(0.25, 0.46, 0.45, 0.94) 0.2s both;
    }

    #mapa {
      position: relative;
      width: 421px;
      height: 409px;
      background-color: #e8eaed;
      border: 3px solid #dadce0;
      border-radius: 8px;
      overflow: hidden;
      box-shadow: inset 0 2px 8px rgba(0, 0, 0, 0.1), 0 4px 12px rgba(0, 0, 0, 0.15);
    }

    .mapa-img {
      position: absolute;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      object-fit: contain;
      object-position: center;
      z-index: 1;
    }

    footer {
          background-color: #00897b;
          box-shadow: 0 -2px 10px rgba(0, 137, 123, 0.3);
          display: flex;
          justify-content: space-around;
          align-items: center;
          gap: 10px;
          padding: 10px 20px;
          height: auto;
          min-height: 80px;
          animation: slideInFromBottom 0.6s cubic-bezier(0.25, 0.46, 0.45, 0.94) 0.4s both;
        }

        .route-container {
          display: flex;
          justify-content: center;
          align-items: center;
          gap: 8px;
          padding: 7px 17px;
          border: 2px solid rgba(0, 0, 0, 0.4);
          border-radius: 21px;
          background: rgba(255, 255, 255, 0.1);
          backdrop-filter: blur(5px);
        }

        .route-label {
          color: #fff;
          font-size: 10px;
          font-weight: 600;
          margin: 0;
        }

        .route-number {
          color: #fff;
          font-size: 13px;
          font-weight: 700;
          letter-spacing: 1px;
        }

        .coords-container {
          display: flex;
          justify-content: center;
          align-items: center;
          gap: 8px;
          padding: 7px 17px;
          border: 2px solid rgba(0, 0, 0, 0.4);
          border-radius: 21px;
          background: rgba(255, 255, 255, 0.1);
          backdrop-filter: blur(5px);
        }

        .coords-label {
          color: #fff;
          font-size: 10px;
          font-weight: 600;
          margin: 0;
        }

        .coords-value {
          color: #fff;
          font-size: 13px;
          font-weight: 700;
          letter-spacing: 1px;
          font-family: 'Courier New', monospace;
        }

        .speed-container {
          display: flex;
          justify-content: center;
          align-items: center;
          gap: 8px;
          padding: 7px 17px;
          border: 2px solid rgba(0, 0, 0, 0.4);
          border-radius: 21px;
          background: rgba(255, 255, 255, 0.1);
          backdrop-filter: blur(5px);
        }

        .speed-label {
          color: #fff;
          font-size: 10px;
          font-weight: 600;
          margin: 0;
        }

        .speed-value {
          color: #fff;
          font-size: 13px;
          font-weight: 700;
          letter-spacing: 1px;
        }

        .speed-unit {
          color: #fff;
          font-size: 10px;
          font-weight: 500;
          margin-left: 2px;
        }

        .gear-indicator {
          display: inline-block;
          width: 20px;
          height: 20px;
          line-height: 20px;
          text-align: center;
          background: rgba(255, 255, 255, 0.3);
          border-radius: 50%;
          font-size: 12px;
          font-weight: 700;
          color: #fff;
          margin-left: 8px;
        }

        .speed-bar {
          display: flex;
          gap: 4px;
          align-items: center;
        }

        .speed-segment {
          width: 8px;
          height: 8px;
          border-radius: 2px;
          background: rgba(255, 255, 255, 0.2);
          transition: background 0.2s ease;
        }

        .speed-segment.active {
          background: #ffcc00;
        }

        @media (max-width: 480px) {
          footer {
            flex-direction: column;
            gap: 8px;
            padding: 15px;
          }

          .route-container,
          .coords-container,
          .speed-container {
            padding: 6px 11px;
            width: 100%;
            justify-content: space-between;
          }

          .route-label,
          .coords-label,
          .speed-label {
            font-size: 8px;
          }

          .route-number,
          .coords-value,
          .speed-value {
            font-size: 10px;
          }

          .speed-unit {
            font-size: 8px;
          }

          .gear-indicator {
            width: 13px;
            height: 13px;
            line-height: 13px;
            font-size: 7px;
          }
        }

        #bus {
          position: absolute;
          width: 40px;
          height: 40px;
          background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath fill='%23ffcc00' stroke='%23fff' stroke-width='0.5' d='M4 16c0 .88.39 1.67 1 2.22V20a1 1 0 001 1h1a1 1 0 001-1v-1h8v1a1 1 0 001 1h1a1 1 0 001-1v-1.78c.61-.55 1-1.34 1-2.22V6c0-3.5-3.58-4-8-4s-8 .5-8 4v10z'/%3E%3Ccircle cx='6.5' cy='16.5' r='1.5' fill='%23333'/%3E%3Ccircle cx='17.5' cy='16.5' r='1.5' fill='%23333'/%3E%3Cpath fill='%23333' d='M6 8h12v3H6z'/%3E%3C/svg%3E");
          background-size: contain;
          background-repeat: no-repeat;
          background-position: center;
          box-shadow: 0 0 10px rgba(255, 204, 0, 0.8);
          transform: translate(-50%, -50%);
          z-index: 2;
          animation: scaleInFromCenter 0.6s cubic-bezier(0.25, 0.46, 0.45, 0.94) 0.6s both;
        }
      </style>
    </head>
    <body>
      <header>
        <h1>TruckTracker</h1>
        <div class="status" id="estado">Conectando...</div>
      </header>
  
      <div class="map-container">
        <div id="mapa">
          <img src="data:image/jpeg;base64,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">
          <div id="bus"></div>
        </div>
      </div>

      
      <footer>
        <div class="route-container">
          <span class="route-label">RUTA</span>
          <span class="route-number">677</span>
        </div>

        <div class="coords-container">
          <span class="coords-label">COORD</span>
          <span class="coords-value" id="coordenadas">X: 211, Y: 205</span>
        </div>

        <div class="speed-container">
          <span class="speed-label">VEL</span>
          <div class="speed-bar" id="speedBar">
            <div class="speed-segment" data-level="1"></div>
            <div class="speed-segment" data-level="2"></div>
            <div class="speed-segment" data-level="3"></div>
            <div class="speed-segment" data-level="4"></div>
          </div>
          <span class="speed-value"><span id="velocidad">0</span> <span class="speed-unit">km/h</span></span>
          <span class="gear-indicator" id="marcha">N</span>
        </div>
      </footer>

    <script>
      /*
       * ===================================================================================
       * JAVASCRIPT - LÓGICA DE ANIMACIÓN Y COMUNICACIÓN WEBSOCKET
       * ===================================================================================
       */
      
      // INICIALIZAR CONEXIÓN WEBSOCKET
      // El WebSocket se conecta automáticamente al mismo host que sirvió la página
      // pero en el puerto 81 (configurado en el servidor ESP32)
      var connection = new WebSocket('ws://' + location.hostname + ':81/');

      /*
       * -----------------------------------------------------------------------------------
       * MANEJADORES DE EVENTOS WEBSOCKET
       * -----------------------------------------------------------------------------------
       */

      // CONEXIÓN ESTABLECIDA: Mostrar indicador de estado conectado
      connection.onopen = function () {
        document.getElementById('estado').innerHTML = "CONECTADO";
        document.getElementById('estado').classList.add('connected');
      };

      // CONEXIÓN CERRADA: Mostrar indicador de estado desconectado
      connection.onclose = function () {
        document.getElementById('estado').innerHTML = "DESCONECTADO";
        document.getElementById('estado').classList.remove('connected');
      };

      /*
       * -----------------------------------------------------------------------------------
       * VARIABLES DE ANIMACIÓN Y POSICIÓN
       * -----------------------------------------------------------------------------------
       * 
       * El sistema utiliza interpolación lineal (LERP) para movimiento suave
       * entre la posición actual y la posición objetivo recibida por WebSocket.
       */
      
      var currentX = 211;  // Posición actual X del vehículo en pantalla
      var currentY = 205;  // Posición actual Y del vehículo en pantalla
      var targetX = 211;   // Posición objetivo X (recibida por WebSocket)
      var targetY = 205;   // Posición objetivo Y (recibida por WebSocket)
      const lerpFactor = 0.15; // Factor de interpolación (0.15 = 15% de movimiento por frame)

      var lastX = 211;     // Posición X del frame anterior (para cálculo de velocidad)
      var lastY = 205;     // Posición Y del frame anterior (para cálculo de velocidad)
      var currentSpeed = 0; // Velocidad actual calculada

      /*
       * -----------------------------------------------------------------------------------
       * FUNCIONES DE UTILIDAD
       * -----------------------------------------------------------------------------------
       */

      // INTERPOLACIÓN LINEAL (LERP): Calcula punto intermedio entre start y end
      // Permite movimiento suave en lugar de saltos instantáneos
      function lerp(start, end, factor) {
        return start + (end - start) * factor;
      }

      // ACTUALIZAR BARRA DE VELOCIDAD: Enciende segmentos según velocidad
      function updateSpeedBar(speedKmh) {
        var segments = document.querySelectorAll('.speed-segment');
        var activeLevel = Math.min(Math.ceil(speedKmh / 20), 4); // Máximo 4 segmentos

        segments.forEach(function(segment) {
          var level = parseInt(segment.getAttribute('data-level'));
          if (level <= activeLevel) {
            segment.classList.add('active');
          } else {
            segment.classList.remove('active');
          }
        });
      }

      // ACTUALIZAR MARCHA: Calcula marcha automática según velocidad
      function updateGear(speedKmh) {
        var gearElement = document.getElementById('marcha');
        var gear = 'N';

        // Lógica de marchas: cada 20 km/h aumenta la marcha
        if (speedKmh > 0 && speedKmh <= 20) {
          gear = '1';
        } else if (speedKmh > 20 && speedKmh <= 40) {
          gear = '2';
        } else if (speedKmh > 40 && speedKmh <= 60) {
          gear = '3';
        } else if (speedKmh > 60) {
          gear = '4';
        }

        gearElement.textContent = gear;
      }

      /*
       * -----------------------------------------------------------------------------------
       * BUCLE DE ANIMACIÓN PRINCIPAL
       * -----------------------------------------------------------------------------------
       * 
       * Esta función se ejecuta continuamente usando requestAnimationFrame
       * para animación fluida sincronizada con el refresco del navegador (~60Hz).
       */
      function animate() {
        // INTERPOLAR POSICIÓN: Mover gradualmente hacia el objetivo
        currentX = lerp(currentX, targetX, lerpFactor);
        currentY = lerp(currentY, targetY, lerpFactor);
        currentX = Math.round(currentX); // Redondear a píxel entero
        currentY = Math.round(currentY);

        // ACTUALIZAR POSICIÓN VISUAL DEL VEHÍCULO
        var bus = document.getElementById('bus');
        bus.style.left = currentX + 'px'; // Posicionar horizontalmente
        bus.style.top = currentY + 'px';  // Posicionar verticalmente

        // CALCULAR VELOCIDAD BASADA EN MOVIMIENTO
        var deltaX = currentX - lastX; // Diferencia en X desde frame anterior
        var deltaY = currentY - lastY; // Diferencia en Y desde frame anterior
        var pixelSpeed = Math.sqrt(deltaX * deltaX + deltaY * deltaY); // Distancia euclidiana

        // CONVERTIR A KM/H (factor arbitrario para representación visual)
        var speedKmh = Math.round(pixelSpeed * 20);
        currentSpeed = speedKmh;

        // ACTUALIZAR INFORMACIÓN EN PANTALLA
        document.getElementById('coordenadas').textContent = 'X: ' + currentX + ', Y: ' + currentY;
        document.getElementById('velocidad').textContent = speedKmh;
        updateSpeedBar(speedKmh); // Actualizar barra de velocidad
        updateGear(speedKmh);     // Actualizar indicador de marcha

        // GUARDAR POSICIÓN PARA PRÓXIMO CÁLCULO
        lastX = currentX;
        lastY = currentY;

        // PROGRAMAR SIGUIENTE FRAME
        requestAnimationFrame(animate);
      }

      // INICIAR ANIMACIÓN
      animate();

      // INICIALIZAR POSICIÓN ANTERIOR
      lastX = currentX;
      lastY = currentY;

      /*
       * -----------------------------------------------------------------------------------
       * MANEJADOR DE MENSAJES WEBSOCKET
       * -----------------------------------------------------------------------------------
       * 
       * Esta función se ejecuta cada vez que el ESP32 envía datos por WebSocket.
       * Recibe coordenadas actualizadas y las establece como objetivo de animación.
       */
      connection.onmessage = function (event) {
        var datos = JSON.parse(event.data); // Parsear JSON recibido: {"x":211,"y":205}
        targetX = datos.x;  // Establecer posición X objetivo
        targetY = datos.y;  // Establecer posición Y objetivo
      };
    </script>
    </body>
    </html>
    )rawliteral";

/*
 * =======================================================================================
 * FUNCIÓN SETUP - CONFIGURACIÓN INICIAL DEL SISTEMA IoT
 * =======================================================================================
 * 
 * Esta función inicializa todos los componentes necesarios para el funcionamiento
 * del sistema de monitoreo de transporte público:
 * - Comunicación serie para depuración
 * - Red WiFi en modo Access Point
 * - Servidores web y WebSocket
 */
void setup() {
  // INICIALIZAR COMUNICACIÓN SERIE
  Serial.begin(115200);  // Velocidad estándar para ESP32
  delay(1000);          // Esperar estabilización

  Serial.println("\nIniciando Sistema de Monitoreo de Transporte...");
  
  // CONFIGURACIÓN WiFi - MODO ACCESS POINT
  // El ESP32 actúa como router, creando su propia red WiFi
  WiFi.disconnect(true); // Desconectar de redes existentes
  WiFi.mode(WIFI_AP);    // Establecer modo Access Point
  
  // CREAR RED WIFI
  WiFi.softAP("Auto_Veloz_ESP32", "12345678", 6, 0, 2);
  // Parámetros:
  // - "Auto_Veloz_ESP32": Nombre de la red (SSID)
  // - "12345678": Contraseña WPA2
  // - 6: Canal WiFi (1-13)
  // - 0: Ocultar red (0=visible, 1=oculta)
  // - 2: Máximo de conexiones simultáneas
  
  // OPTIMIZAR WiFi PARA MÁXIMO RENDIMIENTO
  WiFi.setSleep(false); // Desactivar modo ahorro de energía para máxima respuesta

  // MOSTRAR INFORMACIÓN DE CONEXIÓN
  Serial.print("WiFi Access Point creado. IP para clientes: ");
  Serial.println(WiFi.softAPIP()); // Generalmente 192.168.4.1

  // CONFIGURAR SERVIDOR WEB HTTP (PUERTO 80)
  // Define manejador para página principal ("/")
  server.on("/", [](){
    server.send(200, "text/html", index_html); // Enviar página HTML embebida
  });
  server.begin(); // Iniciar servidor web
  Serial.println("Servidor Web iniciado en puerto 80");

  // CONFIGURAR SERVIDOR WEBSOCKET (PUERTO 81)
  webSocket.begin(); // Iniciar servidor WebSocket para comunicación en tiempo real
  Serial.println("Servidor WebSocket iniciado en puerto 81");

  // MENSAJE FINAL DE LISTO
  Serial.println("=== SISTEMA COMPLETAMENTE INICIADO ===");
  Serial.println("1. Conecta tu dispositivo a la WiFi: Auto_Veloz_ESP32");
  Serial.println("2. Usa contraseña: 12345678");
  Serial.println("3. Abre navegador y entra a: http://192.168.4.1");
  Serial.println("4. Usa el joystick para controlar el vehículo");
}

/*
 * =======================================================================================
 * FUNCIÓN LOOP - CICLO PRINCIPAL DE OPERACIÓN
 * =======================================================================================
 * 
 * Esta función se ejecuta continuamente y gestiona:
 * - Procesamiento de eventos WebSocket
 * - Atención a peticiones HTTP
 * - Lectura del joystick y actualización de posición
 * - Envío de datos a clientes conectados
 * 
 * FRECUENCIA DE ACTUALIZACIÓN: ~20Hz (cada 50ms)
 * Proporciona movimiento suave y respuesta adecuada sin sobrecargar la red
 */
void loop() {
  // PROCESAR EVENTOS DE COMUNICACIÓN
  webSocket.loop();      // Manejar conexiones WebSocket activas
  server.handleClient(); // Atender peticiones HTTP entrantes

  // CONTROL DE FRECUENCIA DE ACTUALIZACIÓN
  // 33ms = ~30Hz para mejor suavidad (anteriormente 50ms = 20Hz)
  if (millis() - lastUpdate > 33) {
    lastUpdate = millis(); // Actualizar timestamp para siguiente ciclo

    /*
     * -----------------------------------------------------------------------------------
     * LECTURA Y PROCESAMIENTO DEL JOYSTICK
     * -----------------------------------------------------------------------------------
     * 
     * El joystick proporciona valores analógicos de 0-4095:
     * - Centro: ~2048 (sin movimiento)
     * - Extremos: 0 (máximo en una dirección) y 4095 (máximo opuesto)
     * 
     * Se establecen zonas muertas alrededor del centro para evitar movimiento no deseado.
     */

    // LEER VALORES DEL JOYSTICK
    int xVal = analogRead(PIN_JOYSTICK_X); // Leer eje X (horizontal)
    int yVal = analogRead(PIN_JOYSTICK_Y); // Leer eje Y (vertical)

    // VARIABLES PARA CÁLCULO DE VELOCIDAD
    int velX = 0; // Velocidad horizontal (-4 a +4 píxeles por ciclo)
    int velY = 0; // Velocidad vertical (-4 a +4 píxeles por ciclo)

    /*
     * PROcesamiento EJE X - MOVIMIENTO HORIZONTAL
     * 
     * Zonas de detección:
     * - Centro muerto: 1800-2200 (considerado como sin movimiento)
     * - Derecha: >2200 (velocidad proporcional a la distancia del centro)
     * - Izquierda: <1800 (velocidad negativa proporcional)
     */

    // DETECTAR MOVIMIENTO HACIA LA DERECHA
    if (xVal > 2200) {
      // Mapear valor de joystick a velocidad (2200-4095 → 1-4)
      velX = map(xVal, 2200, 4095, 1, 4);    // Velocidad positiva = derecha
    } 
    // DETECTAR MOVIMIENTO HACIA LA IZQUIERDA
    else if (xVal < 1800) {
      // Mapear valor de joystick a velocidad (1800-0 → -1 a -4)
      velX = map(xVal, 1800, 0, -1, -4);     // Velocidad negativa = izquierda
    }
    // NOTA: Si está entre 1800-2200, velX permanece en 0 (sin movimiento horizontal)

    /*
     * PROcesamiento EJE Y - MOVIMIENTO VERTICAL
     * 
     * Lógica similar al eje X pero para movimiento vertical
     */

    // DETECTAR MOVIMIENTO HACIA ABAJO
    if (yVal > 2200) {
      // Mapear valor de joystick a velocidad (2200-4095 → 1-4)
      velY = map(yVal, 2200, 4095, 1, 4);    // Velocidad positiva = abajo
    } 
    // DETECTAR MOVIMIENTO HACIA ARRIBA
    else if (yVal < 1800) {
      // Mapear valor de joystick a velocidad (1800-0 → -1 a -4)
      velY = map(yVal, 1800, 0, -1, -4);     // Velocidad negativa = arriba
    }
    // NOTA: Si está entre 1800-2200, velY permanece en 0 (sin movimiento vertical)

    /*
     * -----------------------------------------------------------------------------------
     * ACTUALIZACIÓN DE POSICIÓN Y LÍMITES
     * -----------------------------------------------------------------------------------
     */

    // ACTUALIZAR POSICIÓN ACUMULADA
    posX += velX; // Sumar velocidad horizontal a posición actual
    posY += velY; // Sumar velocidad vertical a posición actual

    // APLICAR LÍMITES DE MAPA (márgenes de seguridad)
    // Esto evita que el vehículo salga del área visible
    posX = constrain(posX, 20, 347);  // Límites horizontales: mínimo 20, máximo 347
    posY = constrain(posY, 20, 389);  // Límites verticales: mínimo 20, máximo 389
    // NOTA: Estos límites dependen del tamaño del mapa y elemento visual

    /*
     * -----------------------------------------------------------------------------------
     * ENVÍO DE DATOS POR WEBSOCKET
     * -----------------------------------------------------------------------------------
     * 
     * Solo se envían datos si hay cambios de posición para optimizar ancho de banda.
     * El sistema envía las coordenadas a TODOS los clientes conectados (broadcast).
     */

    // VERIFICAR SI HAY CAMBIOS DE POSICIÓN
    if (posX != oldX || posY != oldY) {
      // FORMATEAR MENSAJE JSON
      // Usamos sprintf para formatear eficientemente en el buffer predefinido
      sprintf(bufferEnvio, "{\"x\":%d,\"y\":%d}", posX, posY);
      // Formato resultante: {"x":211,"y":205}

      // ENVIAR A TODOS LOS CLIENTES WEBSOCKET CONECTADOS
      webSocket.broadcastTXT(bufferEnvio); // Broadcast a múltiples clientes simultáneos

      // ACTUALIZAR POSICIONES ANTERIORES PARA PRÓXIMA COMPARACIÓN
      oldX = posX;
      oldY = posY;
    }
    
    // NOTA: El bucle continúa automáticamente, procesando joystick y actualizando
    // posición continuamente mientras el sistema está activo
  }
}