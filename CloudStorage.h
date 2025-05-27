#ifndef CLOUD_STORAGE_H
#define CLOUD_STORAGE_H

#include "StorageHandler.h"
#include <string>
// clase que hereda de StorageHandler y maneja respaldos y restauraciones en almacenamiento en la nube sobrescribiendo los metodos virtuales.
class CloudStorage : public StorageHandler {
public:
    bool validate() override;
    bool backup(const std::vector<std::string>& folders) override;
    std::string getDescription() const override;
    bool restore() override;
};

#endif // CLOUD_STORAGE_H