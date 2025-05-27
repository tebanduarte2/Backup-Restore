#include <iostream>   // Para entrada/salida de consola
#include <string>     // Para std::string
#include <filesystem> // Para std::filesystem (creación de directorios, rutas)
#include <fstream>    // Para std::ofstream (escritura de archivos)
#include <vector>     // Para std::vector (aunque no se usa directamente en este código, es común)

namespace fs = std::filesystem;

// Define el tamaño de cada archivo en bytes (100 MB)
const long long FILE_SIZE_BYTES = 100LL * 1024 * 1024; // 100 * 1024 * 1024 bytes

/**
 * @brief Crea un archivo de texto y lo llena con un carácter repetido hasta un tamaño específico.
 *
 * @param file_path La ruta completa del archivo a crear.
 * @param fill_char El carácter con el que se llenará el archivo.
 * @return true Si el archivo se creó y llenó exitosamente.
 * @return false Si ocurrió un error (ej. no se pudo abrir el archivo).
 */
bool create_and_fill_file(const fs::path& file_path, char fill_char) {
    std::ofstream outfile(file_path, std::ios::binary | std::ios::out);
    if (!outfile.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo para escritura: " << file_path << std::endl;
        return false;
    }

    // Crear un buffer de caracteres para escribir en bloques
    // Un tamaño de bloque de 1 MB es eficiente.
    const int CHUNK_SIZE = 1024 * 1024; // 1 MB
    std::string chunk(CHUNK_SIZE, fill_char); // Llenar el chunk con el carácter deseado

    long long bytes_written = 0;
    while (bytes_written < FILE_SIZE_BYTES) {
        long long bytes_to_write_this_chunk = std::min(static_cast<long long>(CHUNK_SIZE), FILE_SIZE_BYTES - bytes_written);
        outfile.write(chunk.data(), bytes_to_write_this_chunk);
        if (!outfile) {
            std::cerr << "Error: Falló la escritura en el archivo: " << file_path << std::endl;
            outfile.close();
            return false;
        }
        bytes_written += bytes_to_write_this_chunk;
    }

    outfile.close();
    std::cout << "Creado: " << file_path << " (" << bytes_written / (1024 * 1024) << " MB)" << std::endl;
    return true;
}

int main() {
    std::string base_path_str;
    std::cout << "Ingresa la ruta base donde deseas crear los archivos y carpetas (ej. /home/usuario/mis_archivos_grandes): ";
    std::getline(std::cin, base_path_str);

    fs::path base_path(base_path_str);

    // 1. Crear el directorio base si no existe
    try {
        if (!fs::exists(base_path)) {
            fs::create_directories(base_path);
            std::cout << "Directorio base creado: " << base_path << std::endl;
        } else if (!fs::is_directory(base_path)) {
            std::cerr << "Error: La ruta '" << base_path << "' existe pero no es un directorio." << std::endl;
            return 1;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creando el directorio base: " << e.what() << std::endl;
        return 1;
    }

    // 2. Crear 50 archivos (A1.txt a A50.txt) directamente en la ruta base
    std::cout << "\nCreando archivos A1.txt a A50.txt en " << base_path << "..." << std::endl;
    for (int i = 1; i <= 50; ++i) {
        std::string filename = "A" + std::to_string(i) + ".txt";
        fs::path file_path = base_path / filename;
        char fill_char = (i % 10) + '0'; // Usa el último dígito del número como carácter
        if (i % 10 == 0 && i != 0) { // Si el último dígito es 0 (ej. para A10, A20, A50)
            fill_char = '0';
        } else if (i % 10 == 0 && i == 0) { // Caso especial para A0 si existiera, aunque el rango es 1-50
            fill_char = '0';
        } else {
            fill_char = (i % 10) + '0';
        }

        // Para números de dos dígitos, el último dígito es el que se repite.
        // Ej: A10 -> '0', A11 -> '1', A25 -> '5'
        // Si quieres que A10 sea '1' y '0', la lógica sería más compleja,
        // pero la petición original "A1 lleno de numeros 1, A2 de numeros 2"
        // sugiere un solo dígito repetido. Usaremos el último dígito.
        if (i >= 10) {
            fill_char = (i % 10) + '0';
        } else {
            fill_char = i + '0';
        }

        if (!create_and_fill_file(file_path, fill_char)) {
            std::cerr << "Error al crear el archivo " << filename << ". Abortando." << std::endl;
            return 1;
        }
    }

    // 3. Crear la carpeta C1 dentro de la ruta base
    fs::path c1_path = base_path / "C1";
    try {
        if (!fs::exists(c1_path)) {
            fs::create_directories(c1_path);
            std::cout << "\nDirectorio C1 creado: " << c1_path << std::endl;
        } else if (!fs::is_directory(c1_path)) {
            std::cerr << "Error: La ruta '" << c1_path << "' existe pero no es un directorio." << std::endl;
            return 1;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creando el directorio C1: " << e.what() << std::endl;
        return 1;
    }

    // 4. Crear 50 archivos (A51.txt a A100.txt) dentro de la carpeta C1
    std::cout << "\nCreando archivos A51.txt a A100.txt en " << c1_path << "..." << std::endl;
    for (int i = 51; i <= 100; ++i) {
        std::string filename = "A" + std::to_string(i) + ".txt";
        fs::path file_path = c1_path / filename;
        char fill_char = (i % 10) + '0'; // Usa el último dígito del número como carácter
        if (i >= 10) {
            fill_char = (i % 10) + '0';
        } else {
            fill_char = i + '0'; // Esto solo sería relevante para i < 10, pero el rango es 51-100
        }

        if (!create_and_fill_file(file_path, fill_char)) {
            std::cerr << "Error al crear el archivo " << filename << ". Abortando." << std::endl;
            return 1;
        }
    }

    std::cout << "\n¡Todos los archivos y directorios han sido creados exitosamente!" << std::endl;

    return 0;
}
