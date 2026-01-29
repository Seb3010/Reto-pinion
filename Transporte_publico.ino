// ============================================
// LIBRERÍAS PARA COMUNICACIÓN WIFI Y WEB
// ============================================
#include <WiFi.h>                // Biblioteca para crear y gestionar redes WiFi
#include <WebServer.h>          // Biblioteca para servidor HTTP (entrega páginas web)
#include <WebSocketsServer.h>   // Biblioteca para WebSocket (comunicación bidireccional en tiempo real)

// ============================================
// CONFIGURACIÓN DE PINES DEL JOYSTICK
// ============================================
// Joystick analógico conectado a dos pines ADC del ESP32
// Cada pin lee voltaje (0-4095) representando la posición del joystick
// - Joystick en posición central: ~2000-2200 en ambos ejes
// - Joystick a la derecha: ~4095 en X
// - Joystick a la izquierda: ~0 en X
// - Joystick arriba: ~4095 en Y
// - Joystick abajo: ~0 en Y
const int PIN_JOYSTICK_X = 34; // Pin para leer movimiento horizontal (izquierda-derecha)
const int PIN_JOYSTICK_Y = 35; // Pin para leer movimiento vertical (arriba-abajo)

// ============================================
// VARIABLES DE ESTADO DEL JUEGO
// ============================================
// Posición actual del bus en el mapa (coordenadas en píxeles)
// Iniciamos en el centro del mapa (190, 190)
int posX = 190; // Posición horizontal actual del bus (0-380 píxeles)
int posY = 190; // Posición vertical actual del bus (0-380 píxeles)

// Posiciones anteriores (para detectar si hubo movimiento)
int oldX = 0;  // Posición X anterior (guardada del ciclo anterior)
int oldY = 0;  // Posición Y anterior (guardada del ciclo anterior)

// Controlador de tiempo para actualizar a frecuencia constante
unsigned long lastUpdate = 0;  // Tiempo (en milisegundos) de la última actualización

// Buffer temporal para enviar datos JSON de forma muy rápida
// sprintf() es más rápido que String() para crear mensajes JSON
char bufferEnvio[64]; 

// ============================================
// CONFIGURACIÓN DE SERVIDORES PARA COMUNICACIÓN
// ============================================
// Usamos DOS servidores diferentes con puertos distintos:

// SERVIDOR 1: WebServer (puerto 80) - Para entregar la página web
// - Cuando alguien entra a la IP del ESP32, este servidor entrega el código HTML/CSS/JavaScript
// - Solo se usa UNA vez al cargar la página inicialmente
// - Protocolo HTTP estándar (como cualquier página web normal)
WebServer server(80);

// SERVIDOR 2: WebSocketsServer (puerto 81) - Para comunicación en tiempo real
// - Permite comunicación bidireccional permanente sin recargar la página
// - El ESP32 puede enviar actualizaciones de posición del bus 20 veces por segundo
// - Mantiene conexión abierta constantemente (no se cierra entre mensajes)
// - Mucho más eficiente que hacer peticiones HTTP repetidas
WebSocketsServer webSocket = WebSocketsServer(81);

// ============================================
// PÁGINA WEB EMBEBIDA (HTML + CSS + JAVASCRIPT)
// ============================================
// Esta es la interfaz visual que se muestra en el navegador del usuario
// Se almacena en PROGMEM (memoria flash del ESP32) para no usar RAM
// PROGMEM ahorra memoria RAM crítica ya que el ESP32 tiene poca memoria para programas grandes
// PROGMEM es más lento de leer pero perfecto para texto estático como HTML

// El código HTML está contenido entre R"rawliteral(...)rawliteral"
// Esta sintaxis permite escribir HTML de múltiples líneas sin problemas de comillas
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <!-- META TAGS: Configuración de la página web -->
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <!-- - width=device-width: La página se ajusta al ancho de la pantalla (móvil, tablet, PC) -->
  <!-- - initial-scale=1.0: Zoom inicial al 100% (sin zoom automático) -->
  <!-- - user-scalable=no: Evita que el usuario haga zoom con gestos (mejor control) -->
  
  <title>Reto Pinion - Monitor</title> <!-- Título que aparece en la pestaña del navegador -->
  
  <!-- ============================================ -->
  <!-- ESTILOS CSS: Apariencia visual de la página -->
  <!-- ============================================ -->
  <style>
    /* ESTILO DEL CUERPO DE LA PÁGINA */
    body { 
      background-color: #222;       /* Color de fondo: gris oscuro (estética futurista) */
      margin: 0;                     /* Sin márgenes (página completa de borde a borde) */
      overflow: hidden;              /* Evita barras de desplazamiento */
      display: flex;                 /* Usa Flexbox para centrar contenido */
      justify-content: center;        /* Centra horizontalmente el contenido */
      align-items: center;            /* Centra verticalmente el contenido */
      height: 100vh;                 /* Altura completa de la ventana del navegador */
      color: white;                  /* Color del texto: blanco */
      font-family: sans-serif;       /* Fuente: Arial, Helvetica, o similar */
    }
    
    /* CONTENEDOR DEL MAPA: Cuadro gris donde se mueve el bus */
    #mapa {
      position: relative;            /* Permite posicionar elementos dentro de forma absoluta */
      width: 380px;                  /* Ancho del mapa: 380 píxeles */
      height: 380px;                 /* Alto del mapa: 380 píxeles */
      background-color: #444;        /* Color de fondo del mapa: gris medio */
      border: 5px solid #666;        /* Borde del mapa: 5 píxeles de color gris claro */
      border-radius: 10px;           /* Bordes redondeados (10 píxeles de radio) */
      
      /* PATRÓN DE CUADRÍCULA: Crea líneas horizontales y verticales */
      /* repeating-linear-gradient() crea líneas que se repiten */
      /* 0deg = líneas horizontales, 90deg = líneas verticales */
      /* Cada 20px hay una línea gris (#555) */
      background-image: repeating-linear-gradient(0deg, transparent, transparent 19px, #555 20px), 
                        repeating-linear-gradient(90deg, transparent, transparent 19px, #555 20px);
    }

    /* ============================================ */
    /* EL AUTOBUS: Representación visual del vehículo */
    /* ============================================ */
    #bus {
      position: absolute;            /* Posicionamiento absoluto dentro del mapa */
      width: 30px;                   /* Ancho del bus: 30 píxeles */
      height: 30px;                  /* Alto del bus: 30 píxeles */
      background-color: #ffcc00;     /* Color del bus: amarillo (estándar de autobuses) */
      border: 2px solid #fff;        /* Borde blanco de 2 píxeles para contraste */
      border-radius: 50%;            /* Redondo (50% de radio = círculo perfecto) */
      box-shadow: 0 0 10px rgba(255, 204, 0, 0.8); /* Resplandor amarillo alrededor */
      transform: translate(-50%, -50%); /* Centra el círculo en la coordenada exacta */
      
      /* ============================================ */
      /* ANTI-LAG: Suavizado del movimiento visual */
      /* ============================================ */
      /* transition: anima los cambios de forma suave en lugar de instantánea */
      /* all: aplica a todas las propiedades (left, top, etc.) */
      /* 0.05s: duración de la animación (50 milisegundos) */
      /* linear: velocidad constante (sin aceleración ni desaceleración) */
      transition: all 0.05s linear; 
    }
    
    /* INDICADOR DE ESTADO DE CONEXIÓN (esquina superior izquierda) */
    .status { 
      position: absolute;            /* Posicionamiento absoluto respecto al body */
      top: 10px;                     /* 10 píxeles desde arriba */
      left: 10px;                    /* 10 píxeles desde la izquierda */
      font-size: 12px;               /* Tamaño del texto: 12 píxeles */
      color: #0f0;                   /* Color inicial: verde brillante */
    }
  </style>
</head>
<body>
  <!-- ELEMENTO QUE MUESTRA EL ESTADO DE LA CONEXIÓN -->
  <div class="status" id="estado">Conectando...</div>
  
  <!-- CONTENEDOR DEL MAPA QUE CONTIENE EL AUTOBUS -->
  <div id="mapa">
    <!-- El autobús inicia en el centro del mapa (190, 190) -->
    <div id="bus" style="left: 190px; top: 190px;"></div>
  </div>

<!-- ============================================ -->
<!-- JAVASCRIPT: LÓGICA DE COMUNICACIÓN Y ANIMACIÓN -->
<!-- ============================================ -->
<script>
  // ============================================
  // CONEXIÓN WEBSOCKET: Canal de comunicación en tiempo real
  // ============================================
  // 'ws://' indica protocolo WebSocket (a diferencia de 'http://')
  // location.hostname es la IP del ESP32 (ej: 192.168.4.1)
  // ':81/' es el puerto donde escucha el WebSocket (configurado en el ESP32)
  var connection = new WebSocket('ws://' + location.hostname + ':81/');
  
  // ============================================
  // EVENTO: Cuando la conexión WebSocket se establece correctamente
  // ============================================
  connection.onopen = function () {
    // Actualizar el texto de estado para indicar que estamos conectados
    document.getElementById('estado').innerHTML = "CONECTADO";
    document.getElementById('estado').style.color = "#0f0";        // Color verde brillante
  };
  
  // ============================================
  // EVENTO: Cuando llega un mensaje del ESP32 (actualización de posición)
  // ============================================
  connection.onmessage = function (event) {
    // event.data contiene el mensaje recibido (texto en formato JSON)
    // JSON.parse() convierte el texto JSON en un objeto JavaScript
    // Ejemplo de JSON recibido: {"x":250, "y":300}
    var datos = JSON.parse(event.data);
    
    // ============================================
    // ACTUALIZAR POSICIÓN VISUAL DEL AUTOBUS
    // ============================================
    // Buscar el elemento del bus en el DOM (Document Object Model)
    var bus = document.getElementById('bus');
    
    // Actualizar coordenadas CSS del bus
    // datos.x y datos.y son los valores que envió el ESP32
    // Se añade "px" porque CSS requiere unidades de medida
    bus.style.left = datos.x + "px";  // Mover horizontalmente
    bus.style.top = datos.y + "px";   // Mover verticalmente
    
    // NOTA: La transición CSS (transition: all 0.05s) hace que el movimiento sea suave
    // en lugar de saltar instantáneamente a la nueva posición
  };
  
  // ============================================
  // EVENTO: Cuando se pierde la conexión WebSocket
  // ============================================
  connection.onclose = function () {
    // Actualizar el texto de estado para indicar desconexión
    document.getElementById('estado').innerHTML = "DESCONECTADO";
    document.getElementById('estado').style.color = "#f00";           // Color rojo brillante
  };
</script>
</body>
</html>
)rawliteral";

// ============================================
// FUNCIÓN SETUP: Configuración inicial del sistema
// ============================================
// Esta función se ejecuta UNA SOLA VEZ al encender o reiniciar el ESP32
// Aquí configuramos todos los componentes antes de iniciar el ciclo principal
void setup() {
  // ============================================
  // INICIALIZAR COMUNICACIÓN SERIAL
  // ============================================
  Serial.begin(115200);  // Abre puerto serial a 115200 baudios (velocidad estándar de ESP32)
  delay(1000);          // Espera 1 segundo para que el puerto se estabilice

  // ============================================
  // 1. CONFIGURACIÓN DE WIFI EN MODO ACCESS POINT
  // ============================================
  Serial.println("\nIniciando Sistema...");
  
  // LIMPIEZA DE CONFIGURACIÓN PREVIA
  WiFi.disconnect(true);  // Desconecta cualquier conexión WiFi previa
  WiFi.mode(WIFI_AP);     // Establece el ESP32 como PUNTO DE ACCESO (modo AP)
                          // - Modo AP: El ESP32 actúa como router WiFi
                          // - Crea su propia red WiFi (no se conecta a Internet)
                          // - Los dispositivos se conectan al ESP32, no al revés
  
  // ============================================
  // CREAR LA RED WIFI DEL SISTEMA
  // ============================================
  // WiFi.softAP() tiene 5 parámetros:
  // Parámetro 1: "Auto_Veloz_ESP32" - Nombre de la red WiFi (SSID) que verán los usuarios
  // Parámetro 2: "12345678"         - Contraseña de la red WiFi (mínimo 8 caracteres)
  // Parámetro 3: 6                  - Canal WiFi (1-13, canal 6 es estándar para evitar interferencias)
  // Parámetro 4: 0                  - SSID visible (0 = visible, 1 = oculto)
  // Parámetro 5: 2                  - Máximo de conexiones simultáneas (2 dispositivos pueden conectarse)
  WiFi.softAP("Auto_Veloz_ESP32", "12345678", 6, 0, 2);
  
  // DESACTIVAR MODO DE AHORRO DE ENERGÍA
  WiFi.setSleep(false);  // Mantiene el WiFi encendido permanentemente
                        // Importante: El modo sleep interrumpe la comunicación por milisegundos
                        // Al desactivarlo, garantizamos comunicación constante sin interrupciones
                        // Esto es crítico para WebSocket que requiere conexión continua

  // ============================================
  // MOSTRAR INFORMACIÓN DE LA RED CREADA
  // ============================================
  Serial.print("WiFi Listo. IP: ");
  // WiFi.softAPIP() devuelve la IP del punto de acceso
  // Normalmente es 192.168.4.1 (IP estándar para modo AP de ESP32)
  Serial.println(WiFi.softAPIP());

  // ============================================
  // 2. CONFIGURACIÓN DEL SERVIDOR WEB (PÁGINA)
  // ============================================
  // server.on() define qué hacer cuando alguien visita una URL específica
  // "/" significa la página principal (ej: 192.168.4.1/)
  server.on("/", [](){
    // Cuando alguien entra a la IP, enviar la página HTML
    // 200 = Código HTTP de éxito (OK)
    // "text/html" = Tipo de contenido (página web)
    // index_html = Contenido de la página web (la variable definida arriba)
    server.send(200, "text/html", index_html);
  });
  server.begin();  // Iniciar el servidor web (empieza a escuchar peticiones)

  // ============================================
  // 3. CONFIGURACIÓN DEL SERVIDOR WEBSOCKET (DATOS)
  // ============================================
  webSocket.begin();  // Iniciar el servidor WebSocket en el puerto 81
                       // El WebSocket estará escuchando conexiones en ws://192.168.4.1:81/
                       // No definimos manejador de eventos porque solo ENVIAMOS datos, no recibimos
  
  // ============================================
  // CONFIRMACIÓN DE INICIALIZACIÓN COMPLETA
  // ============================================
  Serial.println("TODO LISTO. Conéctate y entra a 192.168.4.1");
}

// ============================================
// FUNCIÓN LOOP: Ciclo principal de ejecución
// ============================================
// Esta función se ejecuta INFINITAMENTE, una y otra vez, mientras el ESP32 esté encendido
// Es el "corazón" del sistema donde ocurre toda la lógica en tiempo real
void loop() {
  // ============================================
  // MANTENIMIENTO DE LA CONEXIÓN DE RED (PRIORIDAD ABSOLUTA)
  // ============================================
  // Estas dos funciones DEBEN ejecutarse CONTINUAMENTE, sin interrupciones
  // Si no se ejecutan, la conexión WiFi se pierde y el sistema deja de funcionar
  
  webSocket.loop();      // Procesa eventos del WebSocket (conexiones, desconexiones, mensajes)
                        // Mantiene la conexión WebSocket viva y funcional
                        // Si se omite, la conexión se cierra automáticamente después de unos segundos
  
  server.handleClient(); // Procesa peticiones HTTP del servidor web
                        // Responde cuando alguien carga la página web inicialmente
                        // Solo es necesario cuando alguien solicita la página

  // ============================================
  // CONTROL DE TIEMPO: Actualización a frecuencia constante
  // ============================================
  // millis() devuelve el número de milisegundos desde que el ESP32 se encendió
  // lastUpdate guarda el tiempo de la última actualización
  // La condición verifica si han pasado 50 milisegundos desde la última actualización
  
  // ¿POR QUÉ 50ms?
  // - 50ms = 20 actualizaciones por segundo (20 FPS)
  // - 20 FPS es ideal para movimiento suave en redes WiFi domésticas
  // - Más rápido (ej: 10ms) puede causar congestión en la red WiFi
  // - Más lento (ej: 100ms) hace que el movimiento parezca "lag" o entrecortado
  // - 50ms es el punto dulce: balance entre fluidez y estabilidad de red
  
  // ¿POR QUÉ NO USAR delay(50)?
  // - delay() bloquea TOUTA la ejecución del código
  // - Mientras hace delay, NO se ejecuta webSocket.loop() ni server.handleClient()
  // - Esto causaría desconexiones de WiFi
  // - Usar millis() permite que webSocket.loop() se ejecute mientras esperamos
  if (millis() - lastUpdate > 50) {
    
    // ============================================
    // ACTUALIZAR MARCADOR DE TIEMPO
    // ============================================
    lastUpdate = millis();  // Guardar el tiempo actual para la siguiente comparación

    // ============================================
    // 1. LECTURA DEL JOYSTICK: Capturar posición del controlador
    // ============================================
    // analogRead() lee el voltaje en el pin ADC del ESP32
    // El ESP32 tiene ADC de 12 bits, por lo que devuelve valores de 0 a 4095
    // - 0 = 0 voltios (joystick a la izquierda/arriba)
    // - 2048 = ~1.65 voltios (joystick en el centro)
    // - 4095 = ~3.3 voltios (joystick a la derecha/abajo)
    
    int xVal = analogRead(PIN_JOYSTICK_X);  // Leer eje horizontal (izquierda-derecha)
    int yVal = analogRead(PIN_JOYSTICK_Y);  // Leer eje vertical (arriba-abajo)

    // ============================================
    // 2. FÍSICA SUAVE: Calcular velocidad basada en posición del joystick
    // ============================================
    // El concepto es: MÁS lejos del centro = MÁS velocidad
    // Esto permite control de velocidad variable, no solo "rápido o lento"
    // - Joystick ligeramente desplazado = velocidad lenta (control preciso)
    // - Joystick muy desplazado = velocidad máxima (movimiento rápido)
    
    int velX = 0;  // Velocidad horizontal (negativa = izquierda, positiva = derecha)
    int velY = 0;  // Velocidad vertical (negativa = arriba, positiva = abajo)

    // ============================================
    // CÁLCULO DE VELOCIDAD EN EL EJE X (Horizontal)
    // ============================================
    // Se usan UMBRALES para evitar movimiento cuando el joystick está en reposo
    // - Rango central (1500-2300): Considerado como "sin movimiento" (zona muerta)
    // - Fuera de este rango: El joystick está activamente empujado
    
    // CASO 1: Joystick empujado a la DERECHA (valores altos)
    // xVal > 2300 significa que el joystick está a la derecha del centro
    // map() convierte el rango de voltaje a un rango de velocidad:
    // - Entrada: 2300 a 4095 (posición del joystick)
    // - Salida: 1 a 6 (velocidad en píxeles por actualización)
    // - Resultado: 1 píxel = movimiento suave, 6 píxeles = movimiento rápido
    if (xVal > 2300) {
      velX = map(xVal, 2300, 4095, 1, 6);
    }
    // CASO 2: Joystick empujado a la IZQUIERDA (valores bajos)
    // xVal < 1500 significa que el joystick está a la izquierda del centro
    // - Entrada: 1500 a 0 (posición del joystick)
    // - Salida: -1 a -6 (velocidad negativa = movimiento hacia la izquierda)
    else if (xVal < 1500) {
      velX = map(xVal, 1500, 0, -1, -6);
    }
    // CASO 3: Joystick en el CENTRO (1500-2300)
    // velX permanece en 0 (no hay movimiento horizontal)

    // ============================================
    // CÁLCULO DE VELOCIDAD EN EL EJE Y (Vertical)
    // ============================================
    // Lógica idéntica al eje X pero para arriba/abajo
    
    // CASO 1: Joystick empujado ABAJO (valores altos)
    // Nota: En la pantalla, valores positivos en Y significan ABAJO
    if (yVal > 2300) {
      velY = map(yVal, 2300, 4095, 1, 6);
    }
    // CASO 2: Joystick empujado ARRIBA (valores bajos)
    // Nota: En la pantalla, valores negativos en Y significan ARRIBA
    else if (yVal < 1500) {
      velY = map(yVal, 1500, 0, -1, -6);
    }

    // ============================================
    // ACTUALIZAR POSICIÓN DEL BUS
    // ============================================
    // Sumar la velocidad calculada a la posición actual
    // Esto mueve el bus gradualmente en la dirección indicada
    
    posX += velX;  // Nueva posición X = Posición anterior + Velocidad X
    posY += velY;  // Nueva posición Y = Posición anterior + Velocidad Y

    // ============================================
    // LÍMITES DEL MAPA: Evitar que el bus salga del área visible
    // ============================================
    // constrain() restringe un valor a estar dentro de un rango específico
    // Esto evita que el bus salga del contenedor de 380x380 píxeles
    // Sin esto, el bus podría desaparecer de la pantalla
    
    posX = constrain(posX, 0, 380);  // X siempre entre 0 y 380
    posY = constrain(posY, 0, 380);  // Y siempre entre 0 y 380

    // ============================================
    // 3. ENVÍO DE DATOS AL NAVEGADOR: Solo si hubo movimiento
    // ============================================
    // OPTIMIZACIÓN: Solo enviamos datos si la posición cambió
    // Esto ahorra ancho de banda y mejora rendimiento de la red WiFi
    // Si el bus no se movió, no tiene sentido enviar datos redundantes
    
    if (posX != oldX || posY != oldY) {
      
      // ============================================
      // CREAR MENSAJE JSON DE FORMA OPTIMIZADA
      // ============================================
      // sprintf() crea una cadena de texto formateada
      // Es mucho más rápido que usar concatenación de String
      // String en Arduino es lento y consume memoria RAM
      // sprintf() es directo y eficiente (escritura directa en memoria)
      
      // Formato JSON: {"x":250,"y":300}
      // - { y } delimitan el objeto JSON
      // - "x": es la clave para posición horizontal
      // - %d es un marcador para número decimal entero (será reemplazado por posX)
      // - "y": es la clave para posición vertical
      // - Segundo %d será reemplazado por posY
      sprintf(bufferEnvio, "{\"x\":%d,\"y\":%d}", posX, posY);
      
      // ============================================
      // ENVIAR MENSAJE A TODOS LOS CLIENTES CONECTADOS
      // ============================================
      // broadcastTXT() envía el mensaje a TODOS los dispositivos conectados simultáneamente
      // Esto es útil si múltiples personas están monitoreando el mismo bus
      // TXT indica que es un mensaje de texto (no binario)
      webSocket.broadcastTXT(bufferEnvio);
      
      // ============================================
      // GUARDAR POSICIÓN ACTUAL PARA PRÓXIMA COMPARACIÓN
      // ============================================
      // Esto permite que el sistema detecte si hubo movimiento en el siguiente ciclo
      oldX = posX;  // Actualizar posición X anterior
      oldY = posY;  // Actualizar posición Y anterior
    }
    // Si el bus no se movió (posX == oldX && posY == oldY), NO enviamos nada
    // El navegador simplemente mantiene el bus en la última posición conocida
  }
  // El loop se repite infinitamente, manteniendo el sistema en funcionamiento continuo
}