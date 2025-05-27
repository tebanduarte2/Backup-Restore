#include "utils.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <zip.h> // Para compresión y descompresión ZIP
#include <algorithm>
#include <execution> // Para std::for_each con paralelismo
#include <mutex>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream> // Para std::ofstream en descompresión
#include <cstring> // ¡Añadido para strlen!
#include <omp.h>


namespace fs = std::filesystem;

void show_message(const std::string& message) {
    std::string cmd = "zenity --info --text=\"" + message + "\"";
    system(cmd.c_str());
}

std::vector<std::string> select_folders() {
    std::vector<std::string> folders;
    FILE* fp = popen("zenity --file-selection --directory --multiple --separator=\":\" --title=\"Selecciona carpetas a comprimir\"", "r");
    if (!fp) return folders;

    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof(buffer), fp)) {
        result += buffer;
    }
    pclose(fp);

    std::stringstream ss(result);
    std::string folder;
    while (std::getline(ss, folder, ':')) {
        if (!folder.empty()) {
            // Limpiar saltos de línea
            folder.erase(folder.find_last_not_of("\n\r") + 1);
            folders.push_back(folder);
        }
    }
    return folders;
}

std::string choose_destination_type() {
    FILE* fp = popen("zenity --list --radiolist --title=\"Tipo de almacenamiento\" \\\n                     --column=\"\" --column=\"Opcion\" \\\n                     TRUE Local \\\n                     FALSE Nube \\\n                     FALSE USB", "r");
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

bool copy_directory(const fs::path& source, const fs::path& destination) {
    try {
        if (!fs::exists(source) || !fs::is_directory(source)) {
            return false;
        }
        
        fs::copy(source, destination, 
                 fs::copy_options::recursive | 
                 fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error copiando directorio: " << e.what() << std::endl;
        return false;
    }
}

void compress_folder(const fs::path& folder, const fs::path& dest_path) {
    std::string zipname = dest_path.string() + ".zip";
    int err = 0;
    zip_t* archive = zip_open(zipname.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!archive) {
        std::cerr << "Error creando archivo ZIP: " << zipname << std::endl;
        return;
    }

    std::vector<fs::path> files;
    for (auto& entry : fs::recursive_directory_iterator(folder)) {
        if (fs::is_regular_file(entry)) {
            files.push_back(entry.path());
        }
    }

    std::mutex zip_mutex;

    std::for_each(std::execution::par_unseq, files.begin(), files.end(),
        [&](const fs::path& file_path) {
            std::string relative_path = fs::relative(file_path, folder).string();
            
            std::lock_guard<std::mutex> lock(zip_mutex);
            zip_source_t* source = zip_source_file(archive, file_path.c_str(), 0, 0);
            if (source != nullptr) {
                zip_file_add(archive, relative_path.c_str(), source, ZIP_FL_OVERWRITE);
            }
        });

    zip_close(archive);
}

// --- Implementaciones de las nuevas funciones para la restauración ---

std::string ask_restore_destination_folder() {
    FILE* fp = popen("zenity --file-selection --directory --title=\"Selecciona la carpeta de destino para la restauración\"", "r");
    if (!fp) return "";
    char buffer[1024];
    std::string path;
    if (fgets(buffer, sizeof(buffer), fp)) {
        path = buffer;
        path.erase(path.find_last_not_of("\n\r") + 1); // Eliminar saltos de línea
    }
    pclose(fp);
    return path;
}

std::vector<std::string> select_files_from_list(const std::vector<std::string>& file_list) {
    std::vector<std::string> selected_files;
    if (file_list.empty()) {
        show_message("No hay archivos disponibles para seleccionar.");
        return selected_files;
    }

    std::string cmd = "zenity --list --checklist --multiple --separator=\":\" --title=\"Selecciona archivos a restaurar\" --column=\"\" --column=\"Nombre del Archivo\"";
    for (const auto& file : file_list) {
        cmd += " FALSE \"" + file + "\""; // FALSE para que no estén seleccionados por defecto
    }

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) {
        show_message("Error al abrir Zenity para la selección de archivos.");
        return selected_files;
    }

    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof(buffer), fp)) {
        result += buffer;
    }
    pclose(fp);

    std::stringstream ss(result);
    std::string file;
    while (std::getline(ss, file, ':')) {
        if (!file.empty()) {
            file.erase(file.find_last_not_of("\n\r") + 1); // Limpiar saltos de línea
            selected_files.push_back(file);
        }
    }
    return selected_files;
}
std::string select_zip_file() {
    FILE* fp = popen("zenity --file-selection --file-filter=\"*.zip\" --title=\"Selecciona el archivo ZIP a restaurar\"", "r");
    if (!fp) return "";
    char buffer[1024];
    std::string path;
    if (fgets(buffer, sizeof(buffer), fp)) {
        path = buffer;
        path.erase(path.find_last_not_of("\n\r") + 1); // Eliminar saltos de línea
    }
    pclose(fp);

    // Validación adicional: Asegurarse de que el archivo seleccionado realmente termina en .zip
    // Zenity con --file-filter es bueno, pero el usuario podría escribir algo manualmente.
    if (!path.empty() && path.length() >= 4 && path.substr(path.length() - 4) == ".zip") {
        return path;
    } else if (!path.empty()) {
        show_message("Error: El archivo seleccionado no es un archivo ZIP válido.");
        return "";
    }
    return ""; // No se seleccionó ningún archivo o la selección fue inválida
}

// Función para descomprimir un archivo ZIP
bool decompress_file(const fs::path& zip_file_path, const fs::path& dest_path) {
    int err = 0;
    zip_t* archive = zip_open(zip_file_path.string().c_str(), 0, &err);
    if (!archive) {
        std::cerr << "Error abriendo archivo ZIP para descompresión: " << zip_file_path << " (Error: " << err << ")" << std::endl;
        show_message("Error: No se pudo abrir el archivo ZIP para descompresión.");
        return false;
    }

    bool success = true;
    #pragma omp parallel for
    for (int i = 0; i < zip_get_num_entries(archive, 0); ++i) {
        zip_stat_t zs;
        if (zip_stat_index(archive, i, 0, &zs) < 0) {
            std::cerr << "Error obteniendo estadísticas del archivo en ZIP." << std::endl;
            success = false;
            continue;
        }

        fs::path entry_path = dest_path / zs.name;

        // Si es un directorio, crearlo
        if (zs.name[strlen(zs.name) - 1] == '/') { // strlen requiere <cstring>
            try {
                fs::create_directories(entry_path);
            } catch (const std::exception& e) {
                std::cerr << "Error creando directorio: " << entry_path << " - " << e.what() << std::endl;
                success = false;
            }
            continue;
        }

        // Si es un archivo, extraerlo
        zip_file_t* zf = zip_fopen_index(archive, i, 0);
        if (!zf) {
            std::cerr << "Error abriendo archivo dentro del ZIP: " << zs.name << std::endl;
            success = false;
            continue;
        }

        // Asegurarse de que el directorio padre exista para el archivo
        try {
            fs::create_directories(entry_path.parent_path());
        } catch (const std::exception& e) {
            std::cerr << "Error creando directorio padre para " << entry_path << ": " << e.what() << std::endl;
            success = false;
            zip_fclose(zf);
            continue;
        }

        std::ofstream outfile(entry_path, std::ios::binary);
        if (!outfile.is_open()) {
            std::cerr << "Error creando archivo de salida: " << entry_path << std::endl;
            success = false;
            zip_fclose(zf);
            continue;
        }

        char buffer[4096];
        zip_int64_t read_bytes; // Cambiado de zip_int66_t a zip_int64_t
        while ((read_bytes = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
            outfile.write(buffer, read_bytes);
        }

        outfile.close();
        zip_fclose(zf);
    }

    zip_close(archive);
    return success;
}
