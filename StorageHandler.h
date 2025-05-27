#ifndef STORAGE_HANDLER_H
#define STORAGE_HANDLER_H

#include <vector>
#include <string>
#include <memory>

// Clase abstracta para tipos de almacenamiento donde tienen sus metodos virtuales para manejar respaldos y restauraciones de forma polimorfica.
class StorageHandler {
public:
    virtual ~StorageHandler() = default;
    virtual bool backup(const std::vector<std::string>& folders) = 0;
    virtual bool validate() = 0;
    virtual std::string getDescription() const = 0;
    virtual bool restore() = 0;
};

// factory para crear instancias de StorageHandler dependiendo del tipo de almacenamiento.
std::unique_ptr<StorageHandler> createStorageHandler(const std::string& type);

#endif // STORAGE_HANDLER_H