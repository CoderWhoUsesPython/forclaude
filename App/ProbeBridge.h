#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface ProbeBridge : NSObject

/// Looks for the running UsbmuxProbe driver, opens its user client and reads status.
/// Returns a human-readable report, including which step failed and why.
+ (NSString *)check;

@end

NS_ASSUME_NONNULL_END
