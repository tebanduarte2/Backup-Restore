#include "StorageHandler.h"
#include "utils.h"
#include <iostream>
#include <curl/curl.h>

// Función para elegir la acción (Respaldo o Restauración)
std::string choose_action() {
    FILE* fp = popen("zenity --list --radiolist --title=\"Selecciona una acción\" \\\n                     --column=\"\" --column=\"Acción\" \\\n                     TRUE Respaldo \\\n                     FALSE Restaurar", "r");
    if (!fp) return "";
    char buffer[256];
    std::string choice;
    if (fgets(buffer, sizeof(buffer), fp)) {
        choice = buffer;
        choice.erase(choice.find_last_not_of("\n\r") + 1);
    }
    pclose(fp);
    return choice;
}


int main() {
    
    curl_global_init(CURL_GLOBAL_ALL);

    std::string action = choose_action();
    if (action.empty()) {
        show_message("No se seleccionó ninguna acción.");
        curl_global_cleanup(); // Limpia cURL si se inicializó globalmente
        return 1;
    }

    std::unique_ptr<StorageHandler> storage_handler; // Declarar aquí para scope

    if (action == "Respaldo") {
        auto folders = select_folders();
        if (folders.empty()) {
            show_message("No se seleccionaron carpetas.");
            curl_global_cleanup();
            return 1;
        }

        std::string dest_type = choose_destination_type();
        if (dest_type.empty()) {
            show_message("No se seleccionó el tipo de destino.");
            curl_global_cleanup();
            return 1;
        }

        storage_handler = createStorageHandler(dest_type);
        if (!storage_handler) {
            show_message("Tipo de almacenamiento no válido.");
            curl_global_cleanup();
            return 1;
        }

        if (!storage_handler->validate()) {
             curl_global_cleanup();
            return 1;
        }

        if (!storage_handler->backup(folders)) {
            show_message("Error durante el proceso de respaldo.");
            curl_global_cleanup();
            return 1;
        }
    } else if (action == "Restaurar") {
        std::string restore_type = choose_destination_type(); // Pregunta al usuario el tipo de almacenamiento para restaurar
        if (restore_type.empty()) {
            show_message("No se seleccionó el tipo de almacenamiento para restaurar.");
            curl_global_cleanup();
            return 1;
        }

        storage_handler = createStorageHandler(restore_type);
        if (!storage_handler) {
            show_message("Tipo de almacenamiento no válido para restauración.");
            curl_global_cleanup();
            return 1;
        }

        if (!storage_handler->validate()) {
            // La validación para restauración podría ser diferente poor ej. verificar si el USB está conectado o algo asi
            curl_global_cleanup();
            return 1;
        }

        if (!storage_handler->restore()) {
            show_message("Error durante el proceso de restauración.");
            curl_global_cleanup();
            return 1;
        }
    } else {
        show_message("Acción no reconocida.");
        curl_global_cleanup();
        return 1;
    }
    show_message("Operación completada exitosamente.");
    curl_global_cleanup();

    return 0;
}