# Makefile para el proyecto de respaldo
CXX = g++
# Añadimos -fopenmp para habilitar el soporte de OpenMP
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -fopenmp

# Rutas de inclusión y librerías
# Añadimos -lcurl para vincular la librería cURL
# -lzip y -ltbb ya estaban, pero los mantenemos explícitos.
# -lz es para zlib, una dependencia común de libzip.
# Para nlohmann/json, es un header-only library, así que no necesitas -l.
LIBS = -lcurl -lzip -ltbb -lz
TARGET = backup_tool

# --- CONFIGURACIÓN DE INCLUDES PARA nlohmann/json.hpp ---
# Si usaste Vcpkg (Opción 1):
# Ajusta VCPKG_ROOT y VCPKG_TRIPLET a tu configuración real de Vcpkg.
# VCPKG_ROOT = /path/to/your/vcpkg # Ejemplo: /home/tu_usuario/vcpkg
# VCPKG_TRIPLET = x64-linux # O x64-windows, arm64-osx, etc.
# JSON_INCLUDE_PATH = -I$(VCPKG_ROOT)/installed/$(VCPKG_TRIPLET)/include

# Si usaste la instalación manual (Opción 2) y pusiste json.hpp en ./include/nlohmann/
JSON_INCLUDE_PATH = -I./include

# Si json.hpp está directamente en el mismo directorio que CloudStorage.cpp y otros .cpp,
# NO necesitas añadir -I, pero la inclusión en CloudStorage.cpp debería ser #include "json.hpp"
# en lugar de #include <nlohmann/json.hpp>. Es mejor usar la estructura de directorios.
# --------------------------------------------------------

# Añadir JSON_INCLUDE_PATH a CXXFLAGS
CXXFLAGS += $(JSON_INCLUDE_PATH)

# Archivos fuente
SOURCES = main.cpp \
          StorageHandler.cpp \
          LocalStorage.cpp \
          CloudStorage.cpp \
          UsbStorage.cpp \
          utils.cpp

# Archivos objeto
OBJECTS = $(SOURCES:.cpp=.o)

# Regla principal
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS) -fopenmp

# Regla para archivos objeto
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Dependencias (headers)
# NOTA: Los archivos .hpp (como nlohmann/json.hpp y curl/curl.h) NO deben listarse aquí.
# Solo se incluyen en los archivos .cpp donde se usan.
main.o: StorageHandler.h utils.h
StorageHandler.o: StorageHandler.h LocalStorage.h CloudStorage.h UsbStorage.h
LocalStorage.o: LocalStorage.h StorageHandler.h utils.h
CloudStorage.o: CloudStorage.h StorageHandler.h utils.h
UsbStorage.o: UsbStorage.h StorageHandler.h utils.h
utils.o: utils.h

# Limpiar archivos generados
clean:
	rm -f $(OBJECTS) $(TARGET)

# Instalar dependencias (Ubuntu/Debian)
# Añadimos libcurl4-openssl-dev para la librería cURL
# Para nlohmann/json, es un header-only library, así que no hay un paquete -dev específico
# para instalarlo, solo necesitas la cabecera. Si usas vcpkg, sería 'vcpkg install nlohmann-json'.
install-deps:
	sudo apt-get update
	sudo apt-get install -y libzip-dev libtbb-dev zenity libcurl4-openssl-dev

# Reglas que no son archivos
.PHONY: clean install-deps
