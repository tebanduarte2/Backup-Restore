#include "StorageHandler.h"
#include "LocalStorage.h"
#include "CloudStorage.h"
#include "UsbStorage.h"

// Deficinicion de la factory para crear cada tipo de almacenamiento
std::unique_ptr<StorageHandler> createStorageHandler(const std::string& type) {
    if (type == "Local") {
        return std::make_unique<LocalStorage>();
    } else if (type == "Nube") {
        return std::make_unique<CloudStorage>();
    } else if (type == "USB") {
        return std::make_unique<UsbStorage>();
    }
    return nullptr;
}