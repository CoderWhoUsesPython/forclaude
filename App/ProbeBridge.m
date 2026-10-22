#import "ProbeBridge.h"

#import <CoreFoundation/CoreFoundation.h>
#import <dlfcn.h>
#import <mach/mach.h>
#import <string.h>

// IOKit is loaded at runtime so the build does not depend on the iOS SDK shipping its headers.
typedef mach_port_t PObject;

typedef CFMutableDictionaryRef (*IOServiceMatching_t)(const char *);
typedef kern_return_t (*IOServiceGetMatchingServices_t)(mach_port_t, CFDictionaryRef, PObject *);
typedef PObject (*IOIteratorNext_t)(PObject);
typedef kern_return_t (*IOObjectRelease_t)(PObject);
typedef kern_return_t (*IOServiceOpen_t)(PObject, mach_port_t, uint32_t, PObject *);
typedef kern_return_t (*IOServiceClose_t)(PObject);
typedef kern_return_t (*IOConnectCallScalarMethod_t)(mach_port_t, uint32_t, const uint64_t *, uint32_t, uint64_t *, uint32_t *);

#define LOAD(name) \
    name##_t name = (name##_t)dlsym(handle, #name); \
    if (name == NULL) { return @"IOKit symbol missing: " #name; }

@implementation ProbeBridge

+ (NSString *)check {
    void *handle = dlopen("/System/Library/Frameworks/IOKit.framework/IOKit", RTLD_NOW);
    if (handle == NULL) {
        return [NSString stringWithFormat:@"Could not load IOKit: %s", dlerror()];
    }
    LOAD(IOServiceMatching)
    LOAD(IOServiceGetMatchingServices)
    LOAD(IOIteratorNext)
    LOAD(IOObjectRelease)
    LOAD(IOServiceOpen)
    LOAD(IOServiceClose)
    LOAD(IOConnectCallScalarMethod)

    NSMutableString *report = [NSMutableString string];

    // Match the dext's IOUserService node by its IOUserClass property.
    CFMutableDictionaryRef matching = IOServiceMatching("IOUserService");
    if (matching == NULL) {
        return @"IOServiceMatching returned NULL.";
    }
    CFMutableDictionaryRef propMatch = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(propMatch, CFSTR("IOUserClass"), CFSTR("UsbmuxProbe"));
    CFDictionarySetValue(matching, CFSTR("IOPropertyMatch"), propMatch);
    CFRelease(propMatch);

    PObject iterator = MACH_PORT_NULL;
    kern_return_t kr = IOServiceGetMatchingServices(MACH_PORT_NULL, matching, &iterator);
    if (kr != KERN_SUCCESS) {
        return [NSString stringWithFormat:@"IOServiceGetMatchingServices failed: 0x%x", kr];
    }

    PObject service = IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    if (service == MACH_PORT_NULL) {
        [report appendString:@"No running UsbmuxProbe driver found.\n\n"
                              "Likely causes, in order:\n"
                              "- No Apple USB device is attached (the driver only starts on match).\n"
                              "- The driver is not enabled under Settings > General > Drivers.\n"
                              "- The driver is not listed there at all: the entitlements were not "
                              "carried through signing, so iPadOS did not register it.\n"];
        return report;
    }
    [report appendString:@"Found UsbmuxProbe service.\n"];

    PObject connection = MACH_PORT_NULL;
    kr = IOServiceOpen(service, mach_task_self(), 0, &connection);
    IOObjectRelease(service);
    if (kr != KERN_SUCCESS) {
        [report appendFormat:@"IOServiceOpen failed: 0x%x\n"
                              "(an access error here usually means the user-client entitlements are missing)\n", kr];
        return report;
    }
    [report appendString:@"Opened user client.\n"];

    uint64_t out[4] = {0};
    uint32_t outCount = 1;
    kr = IOConnectCallScalarMethod(connection, 0, NULL, 0, out, &outCount);
    [report appendFormat:@"ping:   kr=0x%x value=0x%llx (expect 0x50524f4245)\n", kr, out[0]];

    memset(out, 0, sizeof(out));
    outCount = 4;
    kr = IOConnectCallScalarMethod(connection, 1, NULL, 0, out, &outCount);
    if (kr == KERN_SUCCESS) {
        [report appendFormat:@"status: vendor=0x%llx product=0x%llx device-open-result=0x%llx starts=%llu\n",
                             out[1], out[2], out[0], out[3]];
        if (out[0] == 0) {
            [report appendString:@"The driver opened the USB device. USB access works.\n"];
        } else {
            [report appendString:@"The driver started but Open() on the device failed.\n"];
        }
    } else {
        [report appendFormat:@"status: kr=0x%x\n", kr];
    }

    IOServiceClose(connection);
    return report;
}

@end
