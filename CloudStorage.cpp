#include "CloudStorage.h"
#include "utils.h" // Para show_message, compress_folder, copy_directory, decompress_file, etc.
#include <filesystem>
#include <iostream>
#include <ctime>     // Para std::time
#include <fstream>   // Para std::ifstream (leer el archivo ZIP)
#include <string>    // Para std::string
#include <vector>    // Para std::vector
#include <nlohmann/json.hpp> // Para parsear la respuesta JSON de Flask

// Incluye la librería cURL para realizar peticiones HTTP
#include <curl/curl.h>

namespace fs = std::filesystem;
using json = nlohmann::json; // Alias para nlohmann::json

// Función de callback para cURL que se usa para escribir la respuesta del servidor
// en un std::string.
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Función de callback para cURL para escribir datos binarios directamente a un archivo
size_t WriteFileCallback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}



bool CloudStorage::validate() {
    show_message("Validando configuración para la subida/descarga a la Nube (via Flask API)...");
    return true;
}

// Implementación del método backup para CloudStorage.
// Este método se encarga de:
// 1. Copiar las carpetas seleccionadas a un directorio temporal.
// 2. Comprimir el directorio temporal en un archivo ZIP.
// 3. Enviar el archivo ZIP a la API de Flask usando una petición HTTP POST.
// 4. Limpiar los archivos temporales después de la subida.
bool CloudStorage::backup(const std::vector<std::string>& folders) {
    show_message("Iniciando subida a la Nube via Flask API...\n(Generando archivo local primero)");

    // Define la ruta del directorio temporal donde se preparará el respaldo.
    // Se utiliza std::filesystem::temp_directory_path() para obtener una ruta temporal segura.
    fs::path temp_backup_dir = fs::temp_directory_path() / "temp_cloud_backup";
    fs::path zip_file_path; // Ruta completa del archivo ZIP temporal.

    // Genera un nombre único para el archivo ZIP basado en la marca de tiempo actual.
    std::string backup_name = "respaldo_flask_" + std::to_string(std::time(nullptr));
    std::string file_to_upload_name = backup_name + ".zip"; // Nombre final del archivo ZIP.

    try {
        // Elimina el directorio temporal si ya existe para asegurar una limpieza.
        if (fs::exists(temp_backup_dir)) {
            fs::remove_all(temp_backup_dir);
        }
        // Crea el directorio temporal.
        fs::create_directories(temp_backup_dir);

        // Copia todas las carpetas seleccionadas por el usuario al directorio temporal.
        //'copy_directory' está definida en 'utils.h'.
        for (const std::string& folder : folders) {
            fs::path source_path(folder);
            fs::path dest_path = temp_backup_dir / source_path.filename();
            if (!copy_directory(source_path, dest_path)) {
                show_message("Error copiando una carpeta a la ubicación temporal: " + folder);
                fs::remove_all(temp_backup_dir); // Limpiar si hay un error
                return false;
            }
        }

        // Comprime el contenido del directorio temporal en un archivo ZIP.
        //'compress_folder' está definida en 'utils.h'.
        zip_file_path = temp_backup_dir.string() + ".zip";
        compress_folder(temp_backup_dir, temp_backup_dir); // compress_folder crea un ZIP junto a la carpeta

        // Verifica si el archivo ZIP se creó correctamente.
        if (!fs::exists(zip_file_path)) {
            show_message("Error: El archivo ZIP no se creó correctamente en la ruta temporal.");
            fs::remove_all(temp_backup_dir); // Limpiar si hay un error
            return false;
        }

    } catch (const std::exception& e) {
        // Captura cualquier excepción durante la preparación del archivo ZIP.
        show_message("Error preparando el archivo ZIP local: " + std::string(e.what()));
        // Intenta limpiar los archivos temporales en caso de error.
        if (fs::exists(temp_backup_dir)) {
            fs::remove_all(temp_backup_dir);
        }
        if (fs::exists(zip_file_path)) {
            fs::remove(zip_file_path);
        }
        return false;
    }

    // --- po aqui intentamos realizar la petición HTTP POST a la API de Flask ---
    CURL *curl; // Puntero a la sesión de cURL
    CURLcode res; // Código de resultado de las operaciones de cURL
    std::string readBuffer; // Buffer para almacenar la respuesta del servidor Flask

    // Inicializa la librería cURL. Esto debe hacerse una vez por aplicación.
    curl_global_init(CURL_GLOBAL_ALL);
    // Crea una nueva sesión de cURL.
    curl = curl_easy_init();

    bool upload_success = false; // Bandera para indicar el éxito de la subida.

    if (curl) {
        // Define la URL de tu aplicación Flask.
        // Reemplazariamos la dirección IP o nombre de host correcto si la app de Flask no está en el mismo pc.
        std::string flask_api_url = "http://127.0.0.1:5000/upload-backup"; // en este caso, la app Flask corre en el mismo pc.

        // Configura la URL a la que se enviará la petición.
        curl_easy_setopt(curl, CURLOPT_URL, flask_api_url.c_str());
        // Configura la función de callback para procesar la respuesta del servidor.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        // Pasa el buffer donde se almacenará la respuesta a la función de callback.
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Abre el archivo ZIP para lectura en modo binario.
        std::ifstream file_stream(zip_file_path, std::ios::binary | std::ios::ate);
        if (!file_stream.is_open()) {
            show_message("Error: No se pudo abrir el archivo ZIP para enviar.");
        } else {
            // Obtiene el tamaño del archivo para la petición.
            long file_size = file_stream.tellg();
            file_stream.seekg(0, std::ios::beg); // Vuelve al inicio del archivo.

            // Configura la petición para enviar el archivo como 'multipart/form-data'.
            // Esto es lo que Flask espera cuando accedes a 'request.files'.
            struct curl_httppost *formpost = NULL;
            struct curl_httppost *lastptr = NULL;

            // Añade el archivo ZIP al formulario HTTP.
            // "backup_file" es el nombre del campo que Flask buscará en 'request.files'.
            curl_formadd(&formpost,
                         &lastptr,
                         CURLFORM_COPYNAME, "backup_file",
                         CURLFORM_FILE, zip_file_path.string().c_str(), // Ruta del archivo local a enviar
                         CURLFORM_FILENAME, file_to_upload_name.c_str(), // Nombre del archivo que Flask verá
                         CURLFORM_CONTENTTYPE, "application/zip", // Tipo MIME del archivo
                         CURLFORM_END);

            // Establece el formulario HTTP para la petición POST.
            curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

            show_message("Enviando " + zip_file_path.filename().string() + " a la API Flask...");
            // Realiza la petición HTTP.
            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                // Si hay un error en la petición cURL, muestra el mensaje de error.
                std::string curl_error = curl_easy_strerror(res);
                show_message("Error al enviar el archivo via cURL: " + curl_error);
            } else {
                // Si la petición fue exitosa, procesa la respuesta de Flask.
                std::cout << "Respuesta de Flask:\n" << readBuffer << std::endl;
                // Aquí, se asume que Flask devolverá un JSON con "success": true
                // Para un análisis más robusto, usarías una librería JSON para C++.
                try {
                    auto response_json = json::parse(readBuffer);
                    if (response_json.contains("success") && response_json["success"].get<bool>()) {
                        show_message("Archivo enviado y procesado por Flask exitosamente.");
                        upload_success = true;
                    } else {
                        std::string error_msg = response_json.contains("message") ? response_json["message"].get<std::string>() : "Error desconocido.";
                        show_message("Flask API respondió con un error: " + error_msg);
                    }
                } catch (const json::parse_error& e) {
                    show_message("Error al parsear la respuesta JSON de Flask: " + std::string(e.what()));
                } catch (const std::exception& e) {
                    show_message("Error inesperado al procesar la respuesta de Flask: " + std::string(e.what()));
                }
            }

            // Limpia los recursos del formulario HTTP.
            curl_formfree(formpost);
            // Cierra el stream del archivo.
            file_stream.close();
        }
        // Limpia la sesión de cURL.
        curl_easy_cleanup(curl);
    } else {
        show_message("Error: No se pudo inicializar cURL.");
    }
    // Desinicializa la librería cURL. Esto debe hacerse una vez al final de la aplicación.
    curl_global_cleanup();

    // --- Sección para la limpieza de archivos temporales ---
    try {
        // Elimina el directorio temporal y el archivo ZIP generado.
        if (fs::exists(temp_backup_dir)) {
            fs::remove_all(temp_backup_dir);
        }
        if (fs::exists(zip_file_path)) {
            fs::remove(zip_file_path);
        }
    } catch (const std::exception& e) {
        std::cerr << "Advertencia: No se pudieron eliminar los archivos temporales: " << e.what() << std::endl;
    }

    return upload_success;
}

// Implementación del método restore para CloudStorage.
// Este método se encarga de:
// 1. Obtener la lista de respaldos disponibles de la API de Flask.
// 2. Permitir al usuario seleccionar uno o más respaldos.
// 3. Solicitar al usuario un directorio de destino para la restauración.
// 4. Descargar cada respaldo seleccionado de la API de Flask.
// 5. Descomprimir cada respaldo descargado en el directorio de destino.
bool CloudStorage::restore() {
    show_message("Iniciando proceso de restauración desde la Nube (via Flask API)...");

    CURL *curl;
    CURLcode res;
    std::string readBuffer; // Para almacenar la respuesta JSON de listado
    std::string flask_api_base_url = "http://127.0.0.1:5000"; // URL base de tu API Flask

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (!curl) {
        show_message("Error: No se pudo inicializar cURL para la restauración.");
        curl_global_cleanup();
        return false;
    }

    // --- 1. Obtener la lista de respaldos disponibles ---
    std::vector<std::string> available_backups;
    curl_easy_setopt(curl, CURLOPT_URL, (flask_api_base_url + "/list-backups").c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    show_message("Obteniendo lista de respaldos disponibles de la Nube...");
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        show_message("Error al listar respaldos de la Nube: " + std::string(curl_easy_strerror(res)));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return false;
    }

    try {
        auto response_json = json::parse(readBuffer);
        if (response_json.contains("success") && response_json["success"].get<bool>() && response_json.contains("files")) {
            #pragma omp parallel for
            for (const auto& file : response_json["files"]) {
                available_backups.push_back(file.get<std::string>());
            }
        } else {
            std::string error_msg = response_json.contains("message") ? response_json["message"].get<std::string>() : "Error desconocido al listar.";
            show_message("Flask API respondió con un error al listar respaldos: " + error_msg);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return false;
        }
    } catch (const json::parse_error& e) {
        show_message("Error al parsear la respuesta JSON de listado: " + std::string(e.what()));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return false;
    } catch (const std::exception& e) {
        show_message("Error inesperado al procesar la lista de respaldos: " + std::string(e.what()));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return false;
    }

    if (available_backups.empty()) {
        show_message("No se encontraron respaldos en la Nube.");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return true; // No hay nada que restaurar, pero no es un error fatal.
    }

    // --- 2. Permitir al usuario seleccionar respaldos ---
    std::vector<std::string> selected_backups = select_files_from_list(available_backups);

    if (selected_backups.empty()) {
        show_message("No se seleccionaron archivos para restaurar.");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return true;
    }

    // --- 3. Solicitar al usuario un directorio de destino ---
    std::string destination_folder_str = ask_restore_destination_folder();
    if (destination_folder_str.empty()) {
        show_message("No se seleccionó una carpeta de destino para la restauración.");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return false;
    }
    fs::path destination_folder(destination_folder_str);

    // Asegurarse de que el directorio de destino exista
    try {
        fs::create_directories(destination_folder);
    } catch (const std::exception& e) {
        show_message("Error creando la carpeta de destino: " + std::string(e.what()));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return false;
    }

    // --- 4. y 5. Descargar y Descomprimir cada respaldo seleccionado ---
    bool all_restored_successfully = true;
    #pragma omp parallel for
    for (const auto& backup_filename : selected_backups) {
        show_message("Descargando respaldo: " + backup_filename + "...");
        std::string download_url = flask_api_base_url + "/download-backup/" + backup_filename;
        fs::path temp_download_path = fs::temp_directory_path() / backup_filename;

        // Configurar cURL para descargar el archivo directamente a un archivo local
        FILE* fp = fopen(temp_download_path.string().c_str(), "wb");
        if (!fp) {
            show_message("Error: No se pudo crear archivo temporal para descarga: " + temp_download_path.string());
            all_restored_successfully = false;
            continue;
        }

        curl_easy_setopt(curl, CURLOPT_URL, download_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback); // Usar el callback para escribir a archivo
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp); // Pasar el puntero al archivo

        res = curl_easy_perform(curl);
        fclose(fp); // Cerrar el archivo después de la descarga

        if (res != CURLE_OK) {
            show_message("Error al descargar " + backup_filename + ": " + std::string(curl_easy_strerror(res)));
            fs::remove(temp_download_path); // Limpiar archivo parcial
            all_restored_successfully = false;
            continue;
        }
        show_message("Descarga de " + backup_filename + " completada. Descomprimiendo...");
        if (decompress_file(temp_download_path, destination_folder)) {
            show_message("Restauración de " + backup_filename + " exitosa en " + destination_folder_str);
        } else {
            show_message("Error al descomprimir " + backup_filename + ".");
            all_restored_successfully = false;
        }

        // Limpiar el archivo ZIP descargado temporalmente
        try {
            fs::remove(temp_download_path);
        } catch (const std::exception& e) {
            std::cerr << "Advertencia: No se pudo eliminar el archivo temporal de descarga: " << e.what() << std::endl;
        }
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (all_restored_successfully) {
        show_message("Proceso de restauración completado exitosamente.");
    } else {
        show_message("Proceso de restauración completado con algunos errores.");
    }

    return all_restored_successfully;
}

std::string CloudStorage::getDescription() const {
    return "Almacenamiento en la Nube (via Flask API)";
}
