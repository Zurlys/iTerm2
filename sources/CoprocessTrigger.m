//
//  InteractiveScriptTrigger.m
//  iTerm
//
//  Created by George Nachman on 9/24/11.
//  Copyright 2011 Georgetech. All rights reserved.
//

#import "CoprocessTrigger.h"
#import "iTermAnnouncementViewController.h"
#import "PTYSession.h"

static NSString *const kSuppressCoprocessTriggerWarning = @"NoSyncSuppressCoprocessTriggerWarning";

@implementation CoprocessTrigger

+ (NSString *)title {
    return @"Run Coprocess…";
}

- (BOOL)takesParameter {
    return YES;
}

- (NSString *)triggerOptionalParameterPlaceholderWithInterpolation:(BOOL)interpolation {
    return @"Enter coprocess command to run";
}

- (BOOL)performActionWithCapturedStrings:(NSString *const *)capturedStrings
                          capturedRanges:(const NSRange *)capturedRanges
                            captureCount:(NSInteger)captureCount
                               inSession:(id<iTermTriggerSession>)aSession
                                onString:(iTermStringLine *)stringLine
                    atAbsoluteLineNumber:(long long)lineNumber
                        useInterpolation:(BOOL)useInterpolation
                                    stop:(BOOL *)stop {
    // Need to stop the world to get scope, provided it is needed. Coprocesses are so slow & rare that this is ok.
    [self paramWithBackreferencesReplacedWithValues:capturedStrings
                                              count:captureCount
                                              scope:[aSession triggerSessionVariableScope:self]
                                              owner:aSession
                                   useInterpolation:useInterpolation
                                         completion:^(NSString *command) {
        [aSession triggerSession:self
      launchCoprocessWithCommand:command
                      identifier:kSuppressCoprocessTriggerWarning
                          silent:self.isSilent];
    }];
    return YES;
}

- (BOOL)isSilent {
    return NO;
}

@end

@implementation MuteCoprocessTrigger

+ (NSString *)title {
    return @"Run Silent Coprocess…";
}

- (BOOL)takesParameter {
    return YES;
}

- (NSString *)triggerOptionalParameterPlaceholderWithInterpolation:(BOOL)interpolation {
    return @"Enter coprocess command to run";
}

- (BOOL)isSilent {
    return YES;
}

@end
