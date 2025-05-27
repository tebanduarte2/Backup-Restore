#include "LocalStorage.h"
#include "utils.h"
#include <filesystem>
#include <algorithm>
#include <execution>
#include <mutex>
#include <iostream>
#include <cstdlib>

namespace fs = std::filesystem;
bool LocalStorage::validate() {
    destination_folder = ask_destination_folder();
    if (destination_folder.empty()) {
        show_message("No se seleccionó una carpeta de destino.");
        return false;
    }

    backup_name = ask_backup_name();
    if (backup_name.empty()) {
        show_message("No se ingresó un nombre para el respaldo.");
        return false;
    }
    return true;
}

bool LocalStorage::backup(const std::vector<std::string>& folders) {
    // Crear la carpeta de respaldo
    fs::path backup_folder = fs::path(destination_folder) / backup_name;
    try {
        if (fs::exists(backup_folder)) {
            show_message("La carpeta de respaldo ya existe. Se sobrescribirá.");
            fs::remove_all(backup_folder);
        }
        fs::create_directories(backup_folder);
    } catch (const std::exception& e) {
        show_message("Error creando la carpeta de respaldo: " + std::string(e.what()));
        return false;
    }

    // Copiar todas las carpetas seleccionadas en paralelo
    std::vector<bool> copy_results(folders.size());
    std::mutex error_mutex;
    std::vector<std::string> error_messages;

    std::transform(std::execution::par_unseq, folders.begin(), folders.end(), copy_results.begin(),
        [&](const std::string& folder) -> bool {
            fs::path source_path(folder);
            if (fs::is_directory(source_path)) {
                fs::path dest_path = backup_folder / source_path.filename();
                try {
                    if (!copy_directory(source_path, dest_path)) {
                        std::lock_guard<std::mutex> lock(error_mutex);
                        error_messages.push_back("Error copiando la carpeta: " + folder);
                        return false;
                    }
                    return true;
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(error_mutex);
                    error_messages.push_back("Excepción copiando " + folder + ": " + e.what());
                    return false;
                }
            } else {
                std::lock_guard<std::mutex> lock(error_mutex);
                error_messages.push_back("Carpeta no válida: " + folder);
                return false;
            }
        });

    bool all_copied = std::all_of(copy_results.begin(), copy_results.end(), [](bool result) { return result; });
    
    if (!all_copied) {
        std::string combined_errors;
        for (const auto& error : error_messages) {
            combined_errors += error + "\n";
        }
        show_message("Hubo errores al copiar algunas carpetas:\n" + combined_errors);
        return false;
    }

    // Comprimir la carpeta de respaldo completa
    compress_folder(backup_folder, backup_folder);
    
    // Eliminar la carpeta temporal después de crear el ZIP
    try {
        fs::remove_all(backup_folder);
    } catch (const std::exception& e) {
        std::cerr << "Advertencia: No se pudo eliminar la carpeta temporal: " << e.what() << std::endl;
    }

    show_message("Respaldo local creado exitosamente: " + backup_name + ".zip");
    return true;
}

std::string LocalStorage::getDescription() const {
    return "Almacenamiento Local";
}

std::string LocalStorage::ask_destination_folder() {
    FILE* fp = popen("zenity --file-selection --directory --title=\"Selecciona carpeta de destino para el respaldo\"", "r");
    if (!fp) return "";
    char buffer[1024];
    std::string path;
    if (fgets(buffer, sizeof(buffer), fp)) {
        path = buffer;
        path.erase(path.find_last_not_of("\n\r") + 1);
    }
    pclose(fp);
    return path;
}

std::string LocalStorage::ask_backup_name() {
    FILE* fp = popen("zenity --entry --title=\"Nombre del respaldo\" --text=\"Ingresa el nombre para la carpeta de respaldo:\"", "r");
    if (!fp) return "";
    char buffer[256];
    std::string name;
    if (fgets(buffer, sizeof(buffer), fp)) {
        name = buffer;
        name.erase(name.find_last_not_of("\n\r") + 1);
    }
    pclose(fp);
    return name;
}

bool LocalStorage::restore() {
    show_message("Iniciando restauración desde Almacenamiento Local...");

    // 1. Pedir al usuario que seleccione un archivo ZIP
    std::string zip_file_str = select_zip_file();
    if (zip_file_str.empty()) {
        show_message("No se seleccionó ningún archivo ZIP para restaurar o la selección fue inválida.");
        return false;
    }
    fs::path zip_file_path(zip_file_str);

    // 2. Pedir al usuario dónde quiere que se descomprima
    std::string destination_folder_str = ask_restore_destination_folder();
    if (destination_folder_str.empty()) {
        show_message("No se seleccionó una carpeta de destino para la restauración.");
        return false;
    }
    fs::path destination_folder(destination_folder_str);

    // Asegurarse de que el directorio de destino exista
    try {
        fs::create_directories(destination_folder);
    } catch (const std::exception& e) {
        show_message("Error creando la carpeta de destino: " + std::string(e.what()));
        return false;
    }

    // 3. Descomprimir el archivo ZIP en el directorio elegido
    show_message("Descomprimiendo " + zip_file_path.filename().string() + " en " + destination_folder_str + "...");
    if (decompress_file(zip_file_path, destination_folder)) {
        show_message("Restauración local completada exitosamente.");
        return true;
    } else {
        show_message("Error durante la descompresión del archivo ZIP.");
        return false;
    }
}