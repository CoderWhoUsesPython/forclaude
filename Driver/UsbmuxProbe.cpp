#include <os/log.h>

#include <DriverKit/DriverKit.h>
#include <USBDriverKit/USBDriverKit.h>

#include "UsbmuxProbe.h"

struct UsbmuxProbe_IVars
{
    IOUSBHostDevice* device;
    uint64_t openResult;
    uint64_t vendorId;
    uint64_t productId;
    uint64_t startCount;
};

bool UsbmuxProbe::init()
{
    if (!super::init()) {
        return false;
    }
    ivars = IONewZero(UsbmuxProbe_IVars, 1);
    if (ivars == nullptr) {
        return false;
    }
    return true;
}

void UsbmuxProbe::free()
{
    IOSafeDeleteNULL(ivars, UsbmuxProbe_IVars, 1);
    super::free();
}

kern_return_t IMPL(UsbmuxProbe, Start)
{
    kern_return_t ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        return ret;
    }

    ivars->device = OSDynamicCast(IOUSBHostDevice, provider);
    if (ivars->device == nullptr) {
        os_log(OS_LOG_DEFAULT, "UsbmuxProbe: provider is not an IOUSBHostDevice");
        return kIOReturnBadArgument;
    }
    ivars->device->retain();
    ivars->startCount++;

    // Read vendor/product from the provider's registry properties.
    OSDictionary* props = nullptr;
    if (provider->CopyProperties(&props) == kIOReturnSuccess && props != nullptr) {
        OSNumber* vid = OSDynamicCast(OSNumber, props->getObject("idVendor"));
        OSNumber* pid = OSDynamicCast(OSNumber, props->getObject("idProduct"));
        if (vid != nullptr) {
            ivars->vendorId = vid->unsigned64BitValue();
        }
        if (pid != nullptr) {
            ivars->productId = pid->unsigned64BitValue();
        }
        props->release();
    }

    // Opening the device is the first real privilege this driver needs.
    ivars->openResult = (uint64_t)ivars->device->Open(this, 0, 0);

    os_log(OS_LOG_DEFAULT,
           "UsbmuxProbe: started vid=0x%llx pid=0x%llx open=0x%llx",
           ivars->vendorId, ivars->productId, ivars->openResult);

    return RegisterService();
}

kern_return_t IMPL(UsbmuxProbe, Stop)
{
    if (ivars->device != nullptr) {
        ivars->device->Close(this, 0);
        ivars->device->release();
        ivars->device = nullptr;
    }
    return Stop(provider, SUPERDISPATCH);
}

kern_return_t IMPL(UsbmuxProbe, NewUserClient)
{
    IOService* client = nullptr;
    kern_return_t ret = Create(this, "UserClientProperties", &client);
    if (ret != kIOReturnSuccess) {
        return ret;
    }
    *userClient = OSDynamicCast(IOUserClient, client);
    if (*userClient == nullptr) {
        client->release();
        return kIOReturnError;
    }
    return kIOReturnSuccess;
}

kern_return_t UsbmuxProbe::GetStatus(uint64_t* openResult,
                                     uint64_t* vendorId,
                                     uint64_t* productId,
                                     uint64_t* startCount)
{
    *openResult = ivars->openResult;
    *vendorId = ivars->vendorId;
    *productId = ivars->productId;
    *startCount = ivars->startCount;
    return kIOReturnSuccess;
}
