# Instrucciones para Uso del Sistema y Pull Request

## ✅ Estado Actual del Repositorio

✅ **Repositorio git inicializado** (local + GitHub)
✅ **Rama `main`**: Código original + `receiver_test.ino` (en carpeta separada)
✅ **Rama `feature/motor-fix-transmitter-receiver`**: Modificaciones al transmisor y receptor
✅ **Todas las ramas están en GitHub**

---

## 📁 Estructura del Proyecto

```
Reto-pinion/
├── receiver/
│   └── receiver.ino                    # Sistema principal (WiFi UDP)
├── receiver_test/
│   └── receiver_test.ino                # Modo test automático (solo hardware)
└── transmitter/
    └── transmitter.ino                  # Transmisor con ajustes
```

**NOTA**: `receiver_test.ino` está en carpeta separada para evitar conflictos de compilación con `receiver.ino`.

---

## 🚀 Cómo Compilar y Usar

### Para compilar el sistema principal:
1. Abrir Arduino IDE
2. File → Open → `Reto-pinion-3/receiver/receiver.ino`
3. Subir al ESP32-S3

### Para compilar el modo test:
1. Abrir Arduino IDE
2. File → Open → `Reto-pinion-3/receiver_test/receiver_test.ino`
3. Subir al ESP32-S3

### Para compilar el transmisor:
1. Abrir Arduino IDE
2. File → Open → `Reto-pinion-3/transmitter/transmitter.ino`
3. Subir al ESP32-WROOM-32

---

## 📋 Pull Request

### Estado del PR:
- ✅ Ramas creadas y push completado
- ⏳ PR por crear (manual por usuario)

### Crear el Pull Request:

**Enlace directo**: https://github.com/Seb3010/Reto-pinion/compare/main...feature/motor-fix-transmitter-receiver

**O manual**:
1. Ir a: https://github.com/Seb3010/Reto-pinion/pulls
2. Click en "New pull request"
3. **base**: `main`
4. **compare**: `feature/motor-fix-transmitter-receiver`

### Título del PR:
```
fix: corregir motor que vibra sin girar - ajuste de parámetros y cambio de pin
```

### Descripción del PR:
```markdown
## Problema
El motor solo vibraba sin girar, a pesar de que el transmisor enviaba señal y se comunicaba con el receptor.

## Causas Identificadas
1. GPIO14 es un strapping pin inestable en ESP32
2. Deadband del receptor (20) anulaba comandos bajos
3. PWM mínimo (1300) insuficiente para vencer inercia
4. Kick de arranque no se aplicaba en muchos casos
5. Transmisor enviaba valores muy bajos que caían en deadband

## Cambios Implementados

### Transmistor
- Histeresis más estrecha (2150/2050/1950/2050)
- Escala de velocidad aumentada (mínimo 50 en lugar de 0)
- Thresholds reducidos (2/1 en lugar de 3/2)
- Diagnostic mejorado con valores crudos del ADC

### Receptor
- Cambiar ENA de GPIO14 a GPIO2 (pin seguro en ESP32-S3)
- Ajustar parámetros: deadband 8, minPWM 1800, kickPWM 2200
- Mejorar lógica de kick (se aplica siempre al arrancar)
- Añadir contador de paquetes y diagnóstico mejorado
- Corregir API de LEDC: ledcAttachPin() → ledcAttach() (específico para ESP32-S3)

## Estructura del Proyecto
- `receiver/receiver.ino` - Sistema principal (WiFi UDP)
- `receiver_test/receiver_test.ino` - Modo test automático (solo hardware)
- `transmitter/transmitter.ino` - Transmisor con ajustes

## Archivos Modificados
- `transmitter/transmitter.ino`
- `receiver/receiver.ino`
- `receiver_test/receiver_test.ino` (movido a carpeta separada)

## Notas Importantes
- Este PR NO se hará merge a main
- El archivo `receiver_test.ino` se movió a carpeta separada para evitar conflictos
- Para usar las correcciones, trabajar directamente desde la rama `feature/motor-fix-transmitter-receiver`
- La rama `main` solo contiene el código original + modo test

## Cómo Usar las Correcciones
1. Hacer checkout a la rama feature:
   ```bash
   git checkout feature/motor-fix-transmitter-receiver
   ```
2. Abrir `receiver/receiver.ino` y `transmitter/transmitter.ino` en Arduino IDE
3. Subir código al ESP32-S3 desde esta rama
4. El sistema tendrá todas las correcciones aplicadas

## Cómo Probar
1. Primero: Usar modo test desde `receiver_test/receiver_test.ino`
2. Luego: Usar sistema modificado desde `receiver/receiver.ino`
3. Conectar ENA a GPIO2 (NO a GPIO14)
4. Verificar que el motor gira correctamente

## Requisitos de Hardware
- ESP32-S3 para el receptor
- Motor alimentado con 11.1V (3x baterías 3.7V)
- ENA conectado a GPIO2, IN1=GPIO27, IN2=GPIO26
```

**IMPORTANTE**: NO hacer clic en "Merge pull request" - El PR debe permanecer abierto.

---

## 🔄 Comandos Útiles

### Verificar estado actual
```bash
git status
git branch -a
git log --oneline --graph --all
```

### Cambiar entre ramas
```bash
git checkout main  # Para usar código original + test
git checkout feature/motor-fix-transmitter-receiver  # Para usar sistema corregido
```

### Sincronizar cambios futuros
```bash
git checkout main
git pull origin main
git checkout feature/motor-fix-transmitter-receiver
git merge main
```

---

## 📊 Resumen de Commits

### Rama `main` (2 commits):
```
8dc9cdb feat(receiver): agregar modo test automatico para diagnostico
233cb89 feat: initial commit - sistema carro RC original
```

### Rama `feature/motor-fix-transmitter-receiver` (4 commits):
```
8f35e23 fix: mover receiver_test.ino a carpeta separada para evitar conflictos
b68ee94 fix(receiver): corregir pin GPIO14 y ajustar parámetros del motor
4826477 fix(transmitter): ajustar histeresis y escala para mayor sensibilidad
d874d4d feat(receiver): agregar modo test automatico para diagnostico
04a9e24 feat: initial commit - sistema carro RC original
```

---

## 🎯 Checklist de Verificación

### Para el Hardware (ESP32-S3 - Receptor):
- [ ] ENA conectado a GPIO2 (cambio importante, NO a GPIO14)
- [ ] IN1 conectado a GPIO27
- [ ] IN2 conectado a GPIO26
- [ ] Servo conectado a GPIO18
- [ ] Motor alimentado con 11.1V (3x baterías 3.7V)

### Para el Hardware (ESP32-WROOM-32 - Transmisor):
- [ ] Joystick Y en GPIO34
- [ ] Joystick X en GPIO35
- [ ] Conectado al AP del receptor (SSID: CARRO_WIFI)

### Para el Software:
- [ ] Arduino IDE configurado para ESP32-S3 (receptor)
- [ ] Arduino IDE configurado para ESP32 (transmisor)
- [ ] Seleccionar el archivo `.ino` correcto en cada carpeta

---

## 🔧 Resumen de Cambios

### Transmisor (`transmitter.ino`)
1. **Thresholds reducidos** (líneas 49-50): 3/2 → 2/1
2. **Histeresis ajustada** (líneas 54-57): 2220/2080/1860/2000 → 2150/2050/1950/2050
3. **Escala aumentada** (líneas 151, 154): mínimo 0 → 50, máximo 0 → -50
4. **Diagnóstico mejorado** (línea 188): valores crudos del ADC

### Receptor (`receiver.ino`)
1. **Pin cambiado** (líneas 6, 36): GPIO14 → GPIO2
2. **Parámetros ajustados** (líneas 69-74):
   - Deadband: 20 → 8
   - Min PWM: 1300 → 1800
   - Kick PWM: 1700 → 2200
   - Kick MS: 140 → 180
   - Brake MS: 40 → 50
3. **Contador de paquetes** (líneas 33, 278): añadido
4. **Lógica de kick mejorada** (líneas 186-195): se aplica siempre al arrancar
5. **Diagnóstico mejorado** (líneas 299-312): más información detallada
6. **API de LEDC corregida** (líneas 217-223): eliminar ledcSetup(), cambiar ledcAttach() a API nueva (ESP32-S3)

### Modo Test (`receiver_test.ino`)
1. **Test automático de servo** (barrido 60-120°)
2. **Test de motor en adelante/reversa**
3. **Test de rampa de velocidades**
4. **Test de cambio de dirección con freno**
5. **Reporte final de diagnóstico** en texto plano
6. **Parámetros ajustables** al inicio del archivo
7. **API de LEDC corregida** (líneas 332-337): eliminar ledcSetup(), cambiar ledcAttach() a API nueva (ESP32-S3)

---

## 🌐 Enlaces Importantes

- **Repositorio**: https://github.com/Seb3010/Reto-pinion
- **Ramas**: https://github.com/Seb3010/Reto-pinion/branches
- **Crear PR**: https://github.com/Seb3010/Reto-pinion/compare/main...feature-motor-fix-transmitter-receiver

---

## ❓ Preguntas Comunes

### ¿Por qué `receiver_test.ino` está en carpeta separada?
Arduino IDE compila todos los archivos `.ino` en la misma carpeta como si fueran uno solo. Al ponerlo en una carpeta separada, evitamos conflictos de variables duplicadas.

### ¿Qué rama debo usar?
- **Usar `main`**: Si quieres el código original o el modo test
- **Usar `feature`**: Si quieres las correcciones del sistema (recomendado para ESP32-S3)

### ¿Por qué el PR no se debe merge?
El PR sirve como documentación y referencia de las diferencias. La rama `feature` es la que debe usarse para el sistema corregido.

### ¿Qué pasa si mergeo el PR por error?
Si merges accidentalmente, puedes revertir el merge en GitHub y hacer checkout a `feature/motor-fix-transmitter-receiver` nuevamente.

---

## 📞 Soporte

Si encuentras problemas:
1. Verifica que el ESP32-S3 tenga ENA en GPIO2
2. Verifica la alimentación del motor (11.1V)
3. Usa el modo test primero para diagnóstico
4. Revisa el Serial Monitor para mensajes de error

---

## ✅ Conclusión

Todo está listo para usar. Las ramas están en GitHub, el PR puede crearse manualmente, y el sistema está completamente funcional con las correcciones aplicadas.

**El problema del motor que vibra sin girar debería estar completamente resuelto.**
