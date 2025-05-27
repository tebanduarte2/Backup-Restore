#include "UsbStorage.h"
#include "utils.h"

bool UsbStorage::validate() {
    // Implementación futura:
    // - Detectar dispositivos USB montados
    // - Verificar permisos de escritura
    // - Comprobar espacio disponible
    // - Permitir selección si hay múltiples dispositivos
    return true;
}

bool UsbStorage::backup(const std::vector<std::string>& folders) {
    show_message("Iniciando transferencia a USB...\n(Implementación pendiente)");
    
    // Implementación futura:
    // 1. Listar dispositivos USB disponibles
    // 2. Permitir al usuario seleccionar dispositivo
    // 3. Verificar espacio suficiente
    // 4. Copiar carpetas directamente o crear ZIP según preferencia
    // 5. Verificar integridad de la copia
    // 6. Expulsar dispositivo de forma segura (opcional)
    
    return true;
}

std::string UsbStorage::getDescription() const {
    return "Almacenamiento USB";
}

// Implementación del método restore para UsbStorage
bool UsbStorage::restore() {
    show_message("La funcionalidad de restauración no está implementada para Almacenamiento USB.");
    // Aquí iría la lógica para restaurar desde un respaldo en USB.
    return false; // Retorna false porque la funcionalidad no está completa
}
