#include <os/log.h>

#include <DriverKit/DriverKit.h>

#include "UsbmuxProbe.h"
#include "UsbmuxProbeUserClient.h"

struct UsbmuxProbeUserClient_IVars
{
    UsbmuxProbe* driver;
};

bool UsbmuxProbeUserClient::init()
{
    if (!super::init()) {
        return false;
    }
    ivars = IONewZero(UsbmuxProbeUserClient_IVars, 1);
    return ivars != nullptr;
}

void UsbmuxProbeUserClient::free()
{
    IOSafeDeleteNULL(ivars, UsbmuxProbeUserClient_IVars, 1);
    super::free();
}

kern_return_t IMPL(UsbmuxProbeUserClient, Start)
{
    kern_return_t ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        return ret;
    }
    ivars->driver = OSDynamicCast(UsbmuxProbe, provider);
    if (ivars->driver == nullptr) {
        return kIOReturnBadArgument;
    }
    ivars->driver->retain();
    return kIOReturnSuccess;
}

kern_return_t IMPL(UsbmuxProbeUserClient, Stop)
{
    OSSafeReleaseNULL(ivars->driver);
    return Stop(provider, SUPERDISPATCH);
}

// Selector 0: ping    -> 1 scalar out (magic value)
// Selector 1: status  -> 4 scalars out (open result, vendor id, product id, start count)
kern_return_t UsbmuxProbeUserClient::ExternalMethod(uint64_t selector,
                                                    IOUserClientMethodArguments* arguments,
                                                    const IOUserClientMethodDispatch* dispatch,
                                                    OSObject* target,
                                                    void* reference)
{
    if (arguments == nullptr) {
        return kIOReturnBadArgument;
    }

    switch (selector) {
    case 0:
        if (arguments->scalarOutput == nullptr || arguments->scalarOutputCount < 1) {
            return kIOReturnBadArgument;
        }
        arguments->scalarOutput[0] = 0x50524F4245ULL; // "PROBE"
        arguments->scalarOutputCount = 1;
        return kIOReturnSuccess;

    case 1: {
        if (ivars->driver == nullptr ||
            arguments->scalarOutput == nullptr ||
            arguments->scalarOutputCount < 4) {
            return kIOReturnBadArgument;
        }
        uint64_t openResult = 0, vendorId = 0, productId = 0, startCount = 0;
        kern_return_t ret = ivars->driver->GetStatus(&openResult, &vendorId, &productId, &startCount);
        if (ret != kIOReturnSuccess) {
            return ret;
        }
        arguments->scalarOutput[0] = openResult;
        arguments->scalarOutput[1] = vendorId;
        arguments->scalarOutput[2] = productId;
        arguments->scalarOutput[3] = startCount;
        arguments->scalarOutputCount = 4;
        return kIOReturnSuccess;
    }

    default:
        return kIOReturnUnsupported;
    }
}
