#ifndef LOCAL_STORAGE_H
#define LOCAL_STORAGE_H

#include "StorageHandler.h"
#include <string>
//Clase que hereda de StorageHandler y maneja respaldos y restauraciones en almacenamiento local sobrescribiendo la¿os metodos virtuales.
class LocalStorage : public StorageHandler {
private:
    std::string destination_folder;
    std::string backup_name;

public:
    bool validate() override;
    bool backup(const std::vector<std::string>& folders) override;
    std::string getDescription() const override;
    bool restore() override;

private:
    // Métodos privados para manejar la interacción con el usuario se les pide que seleccionen la carpeta de destino y el nombre del respaldo.
    std::string ask_destination_folder();
    std::string ask_backup_name();
};

#endif // LOCAL_STORAGE_H