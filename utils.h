#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <filesystem> // Para std::filesystem::path

namespace fs = std::filesystem;

void show_message(const std::string& message);
std::vector<std::string> select_folders();
std::string choose_destination_type();
bool copy_directory(const fs::path& source, const fs::path& destination);
void compress_folder(const fs::path& folder, const fs::path& dest_path);

// --- Nuevas funciones para la restauración ---
std::string ask_restore_destination_folder();
std::vector<std::string> select_files_from_list(const std::vector<std::string>& file_list);
std::string select_zip_file();
bool decompress_file(const fs::path& zip_file_path, const fs::path& dest_path);

#endif // UTILS_H