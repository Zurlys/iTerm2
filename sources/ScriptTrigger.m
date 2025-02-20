//
//  ScriptTrigger.m
//  iTerm
//
//  Created by George Nachman on 9/23/11.
//

#import "ScriptTrigger.h"
#import "DebugLogging.h"
#import "iTermAdvancedSettingsModel.h"
#import "iTermBackgroundCommandRunner.h"
#import "iTermCommandRunnerPool.h"
#import "RegexKitLite.h"
#import "NSStringITerm.h"
#include <sys/types.h>
#include <pwd.h>

@implementation ScriptTrigger

+ (iTermBackgroundCommandRunnerPool *)commandRunnerPool {
    static iTermBackgroundCommandRunnerPool *pool;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        pool = [[iTermBackgroundCommandRunnerPool alloc] initWithCapacity:[iTermAdvancedSettingsModel maximumNumberOfTriggerCommands]];
    });
    return pool;
}

+ (NSString *)title
{
    return @"Run Command…";
}

- (BOOL)takesParameter
{
    return YES;
}

- (NSString *)triggerOptionalParameterPlaceholderWithInterpolation:(BOOL)interpolation {
    return @"Enter command to run";
}


- (BOOL)performActionWithCapturedStrings:(NSString *const *)capturedStrings
                          capturedRanges:(const NSRange *)capturedRanges
                            captureCount:(NSInteger)captureCount
                               inSession:(id<iTermTriggerSession>)aSession
                                onString:(iTermStringLine *)stringLine
                    atAbsoluteLineNumber:(long long)lineNumber
                        useInterpolation:(BOOL)useInterpolation
                                    stop:(BOOL *)stop {
    // Need to stop the world to get scope, provided it is needed. Running a command is so slow & rare that this is ok.
    [self paramWithBackreferencesReplacedWithValues:capturedStrings
                                              count:captureCount
                                              scope:[aSession triggerSessionVariableScope:self]
                                              owner:aSession
                                   useInterpolation:useInterpolation
                                         completion:^(NSString *command) {
        if (!command) {
            return;
        }
        [self runCommand:command session:aSession];
    }];
    return YES;
}

- (void)runCommand:(NSString *)command session:(id<iTermTriggerSession>)session {
    DLog(@"Invoking command %@", command);
    iTermBackgroundCommandRunner *runner = [[ScriptTrigger commandRunnerPool] requestBackgroundCommandRunnerWithTerminationBlock:nil];
    runner.command = command;

    [session triggerSession:self runCommandWithRunner:runner];
}

@end
