# MControlCenter (Edición Personalizada)

Aplicación libre y de código abierto (GPLv3) para GNU/Linux que permite monitorear, limpiar y configurar el hardware de portátiles **MSI**.

![Screen Shot of MCC](https://github.com/user-attachments/assets/1e1dcb9b-aa8e-4410-8c77-f9554c1840cb)

---

## 📌 Créditos y Proyecto Base

Esta es una versión modificada y adaptada específicamente para mi computadora personal, basada en el excelente trabajo de los siguientes proyectos:

* **Proyecto original (GUI y servicio):** [MControlCenter](https://github.com/dmitry-s93/MControlCenter) creado por **[dmitry-s93 (Dmitry Serov)](https://github.com/dmitry-s93)** y sus colaboradores.
* **Driver del Kernel de Linux:** [msi-ec](https://github.com/BeardOverflow/msi-ec) desarrollado por **[BeardOverflow](https://github.com/BeardOverflow)** y la comunidad.

---

## ✨ Cambios y Mejoras Realizadas en esta Versión

Además de las funciones originales de MControlCenter, se implementaron las siguientes mejoras personalizadas:

### 1. 🧹 Nueva Pestaña de "Limpieza" (`Cleaning`)
* **Despolvoreado de Ventiladores (Hardware):**
  * Ciclo temporizado (15s, 30s, 45s o 60s) que activa los ventiladores al 100% (*Cooler Boost*) para expulsar polvo acumulado en los disipadores.
  * Barra de progreso en tiempo real, botón de parada de emergencia y restauración automática del estado previo de los ventiladores al finalizar.
* **Limpiador del Sistema GNU/Linux (Software):**
  * Escaneo y liberación de espacio en disco para:
    * Caché de miniaturas (`~/.cache/thumbnails`).
    * Papelera de reciclaje (`~/.local/share/Trash`).
    * Caché de navegadores y aplicaciones (Chrome, Chromium, Firefox, Mesa shader cache, pip, npm, Flatpak).
    * Caché de gestores de paquetes (`APT`, `Pacman`, `Flatpak` huérfanos).
    * Registros antiguos de `systemd-journald` (reducción segura a 50 MB).
  * **Liberación de memoria RAM inactiva:** Limpieza de *Page Cache* y *Buffers* del kernel (`drop_caches`).

### 2. ❄️ Perfil Térmico Calibrado para 32 °C Ambiente (Clima Cálido)
* Curva de ventilación adaptada para entornos con temperatura ambiente elevada (~32 °C) que evita la absorción pasiva de calor (*heat soak*) y el estrangulamiento térmico (*thermal throttling*):
  * **< 46 °C:** 45% (flujo continuo y silencioso en reposo).
  * **46 °C – 54 °C:** 55%
  * **54 °C – 62 °C:** 68%
  * **62 °C – 70 °C:** 80%
  * **70 °C – 76 °C:** 90%
  * **76 °C – 82 °C:** 96%
  * **≥ 82 °C:** 100% (máxima potencia antes de alcanzar temperaturas críticas).
* Botón de acceso rápido **"Perfil 32°C (Clima cálido)"** integrado directamente en la pestaña *Fan control*.

### 3. 🔋 Diagnóstico y Salud de la Batería
* Nuevo panel informativo en la pestaña *Battery* que lee directamente de `/sys/class/power_supply/BAT1/`:
  * Porcentaje real de **Salud de la batería** (*Battery Health %*).
  * Comparativa entre capacidad de diseño de fábrica y capacidad máxima actual (`mAh` / `mWh`).
  * Conteo de ciclos de carga, fabricante, modelo y tecnología de celda.

### 4. 🪟 Interfaz Compacta y Traducción al Español
* Eliminación de barras deslizantes (*scrollbars*) innecesarias mediante un diseño compacto y ventana redimensionable (**590 × 520 px** por defecto) para visualizar todos los controles de un vistazo.
* Traducción completa al español de todos los nuevos módulos e indicadores.

---

## ⚙️ Características Generales

* Visualización de temperaturas de CPU/GPU y velocidad de ventiladores (RPM) en tiempo real.
* Cambio de perfiles de energía (*High Performance*, *Balanced*, *Silent*, *Super Battery*) e integración con `power-profiles-daemon` (PPD).
* Cambio automático de perfil al conectar o desconectar el cargador.
* Límite de carga máxima de batería (60%, 80%, 100% o personalizado) para prolongar su vida útil.
* Control de retroiluminación de teclado, brillo, cámara web, *USB Power Share* e intercambio de teclas `Fn` ⇄ `Super`.

---

## 🛠️ Requisitos e Instalación

### Pre-requisitos
1. Contar con el driver del kernel [`msi-ec`](https://github.com/BeardOverflow/msi-ec) y/o el módulo `ec_sys` con soporte de escritura (`write_support=1`).
2. Dependencias de compilación (Qt 6 y CMake):
   * **Ubuntu / Debian / Linux Mint:**
     ```bash
     sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev qt6-tools-dev-tools libqt6dbus6
     ```
   * **Arch Linux:** `qt6-base qt6-tools cmake base-devel`
   * **Fedora:** `qt6-qtbase-devel qt6-qttools-devel cmake gcc-c++`

### Compilar e Instalar desde el código fuente

```bash
# 1. Compilar el proyecto
cmake -B build -S .
cmake --build build -j$(nproc)

# 2. Instalar el binario actualizado en el sistema
sudo install -m755 build/mcontrolcenter /usr/bin/mcontrolcenter
sudo install -m755 build/helper/mcontrolcenter-helper /usr/libexec/mcontrolcenter-helper
```

También puedes utilizar los scripts incluidos en la carpeta `scripts/`:
```bash
cd scripts
./build.sh
./create_installer.sh
```

---

## 🚀 Iniciar automáticamente con el sistema

Para que MControlCenter se inicie en cada arranque y aplique tus configuraciones:

```bash
cp /usr/share/applications/mcontrolcenter.desktop ~/.config/autostart/mcontrolcenter.desktop
```

---

## 📄 Licencia

Este proyecto se distribuye bajo la licencia **GNU General Public License v3.0 (GPLv3)**. Consulta el archivo [LICENSE](LICENSE) para más detalles.
