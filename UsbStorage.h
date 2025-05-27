#ifndef USB_STORAGE_H
#define USB_STORAGE_H

#include "StorageHandler.h"

class UsbStorage : public StorageHandler {
public:
    bool validate() override;
    bool backup(const std::vector<std::string>& folders) override;
    std::string getDescription() const override;
    bool restore() override;
};

#endif // USB_STORAGE_H