//
//  LineBlock.h
//  iTerm
//
//  Created by George Nachman on 11/21/13.
//
//

#import <Foundation/Foundation.h>
#import "FindContext.h"
#import "ScreenCharArray.h"
#import "iTermEncoderAdapter.h"
#import "iTermFindViewController.h"
#import "iTermMetadata.h"

typedef struct {
    iTermMetadata lineMetadata;
    screen_char_t continuation;
    int number_of_wrapped_lines;
    int width_for_number_of_wrapped_lines;

    // Remembers the offsets at which double-width characters that are wrapped
    // to the next line occur for a pane of width
    // width_for_double_width_characters_cache.
    NSMutableIndexSet *double_width_characters;
    int width_for_double_width_characters_cache;
} LineBlockMetadata;

@class LineBlock;

@protocol iTermLineBlockObserver<NSObject>
- (void)lineBlockDidChange:(LineBlock *)lineBlock;
@end

// LineBlock represents an ordered collection of lines of text. It stores them contiguously
// in a buffer.
@interface LineBlock : NSObject <NSCopying, iTermUniquelyIdentifiable>

// Once this is set to true, it stays true. If double width characters are
// possibly present then a slower algorithm is used to count the number of
// lines. The default (fast) algorithm would give incorrect results for DWCs
// that get wrapped to the next line.
@property(nonatomic, assign) BOOL mayHaveDoubleWidthCharacter;
@property(nonatomic, readonly) int numberOfCharacters;
@property(nonatomic, readonly) NSInteger generation;

+ (instancetype)blockWithDictionary:(NSDictionary *)dictionary;

- (instancetype)initWithRawBufferSize:(int)size;

// Try to append a line to the end of the buffer. Returns false if it does not fit. If length > buffer_size it will never succeed.
// Callers should split such lines into multiple pieces.
- (BOOL)appendLine:(const screen_char_t*)buffer
            length:(int)length
           partial:(BOOL)partial
             width:(int)width
          metadata:(iTermImmutableMetadata)metadata
      continuation:(screen_char_t)continuation;

// Try to get a line that is lineNum after the first line in this block after wrapping them to a given width.
// If the line is present, return a pointer to its start and fill in *lineLength with the number of bytes in the line.
// If the line is not present, decrement *lineNum by the number of lines in this block and return NULL.
- (const screen_char_t *)getWrappedLineWithWrapWidth:(int)width
                                      lineNum:(int*)lineNum
                                   lineLength:(int*)lineLength
                            includesEndOfLine:(int*)includesEndOfLine
                                 continuation:(screen_char_t *)continuationPtr;

// Sets *yOffsetPtr (if not null) to the number of consecutive empty lines just before |lineNum| because
// there's no way for the returned pointer to indicate this.
- (const screen_char_t *)getWrappedLineWithWrapWidth:(int)width
                                             lineNum:(int*)lineNum
                                          lineLength:(int*)lineLength
                                   includesEndOfLine:(int*)includesEndOfLine
                                             yOffset:(int*)yOffsetPtr
                                        continuation:(screen_char_t *)continuationPtr
                                isStartOfWrappedLine:(BOOL *)isStartOfWrappedLine
                                            metadata:(out iTermImmutableMetadata *)metadataPtr;

- (ScreenCharArray *)rawLineAtWrappedLineOffset:(int)lineNum width:(int)width;

// Get the number of lines in this block at a given screen width.
- (int)getNumLinesWithWrapWidth:(int)width;

// Returns whether getNumLinesWithWrapWidth will be fast.
- (BOOL)hasCachedNumLinesForWidth:(int)width;

// Returns true if the last line is incomplete.
- (BOOL)hasPartial;

// Remove the last line. Returns false if there was none.
- (BOOL)popLastLineInto:(screen_char_t const **)ptr
             withLength:(int*)length
              upToWidth:(int)width
               metadata:(out iTermImmutableMetadata *)metadataPtr
           continuation:(screen_char_t *)continuationPtr;

- (void)removeLastWrappedLines:(int)numberOfLinesToRemove
                         width:(int)width;
- (void)removeLastRawLine;
- (int)lengthOfLastLine;

// Drop lines from the start of the buffer. Returns the number of lines actually dropped
// (either n or the number of lines in the block).
- (int)dropLines:(int)n withWidth:(int)width chars:(int *)charsDropped;

// Returns true if there are no lines in the block
- (BOOL)isEmpty;

// Grow the buffer.
- (void)changeBufferSize:(int)capacity;

// Get the size of the raw buffer.
- (int)rawBufferSize;

// Return the number of raw (unwrapped) lines
- (int)numRawLines;

// Return the position of the first used character in the raw buffer. Only valid if not empty.
- (int)startOffset;

// Return the length of a raw (unwrapped) line
- (int)getRawLineLength:(int)linenum;

// Remove extra space from the end of the buffer. Future appends will fail.
- (void)shrinkToFit;

// Return a raw line
- (const screen_char_t *)rawLine:(int)linenum;

// NSLog the contents of the block. For debugging.
- (void)dump:(int)rawOffset toDebugLog:(BOOL)toDebugLog;

// Returns the metadata associated with a line when wrapped to the specified width.
- (iTermImmutableMetadata)metadataForLineNumber:(int)lineNum width:(int)width;
- (iTermImmutableMetadata)metadataForRawLineAtWrappedLineOffset:(int)lineNum width:(int)width;

// Appends the contents of the block to |s|.
- (void)appendToDebugString:(NSMutableString *)s;

// Returns the total number of bytes used, including dropped chars.
- (int)rawSpaceUsed;

// Returns the total number of lines, including dropped lines.
- (int)numEntries;

// Searches for a substring, populating results with ResultRange objects.
- (void)findSubstring:(NSString*)substring
              options:(FindOptions)options
                 mode:(iTermFindMode)mode
             atOffset:(int)offset
              results:(NSMutableArray*)results
      multipleResults:(BOOL)multipleResults
includesPartialLastLine:(BOOL *)includesPartialLastLine;

// Tries to convert a byte offset into the block to an x,y coordinate relative to the first char
// in the block. Returns YES on success, NO if the position is out of range.
//
// If the position is after the last character on a line, wrapEOL determines if it will return the
// coordinate of the first null on that line of the first character on the next line.
- (BOOL)convertPosition:(int)position
              withWidth:(int)width
              wrapOnEOL:(BOOL)wrapOnEOL
                    toX:(int*)x
                    toY:(int*)y;

// Returns the position of a char at (x, lineNum). Fills in yOffsetPtr with number of blank lines
// before that cell, and sets *extendsPtr if x is at the right margin (after nulls).
- (int)getPositionOfLine:(int*)lineNum
                     atX:(int)x
               withWidth:(int)width
                 yOffset:(int *)yOffsetPtr
                 extends:(BOOL *)extendsPtr;

// Count the number of "full lines" in buffer up to position 'length'. A full
// line is one that, after wrapping, goes all the way to the edge of the screen
// and has at least one character wrap around. It is equal to the number of
// lines after wrapping minus one. Examples:
//
// 2 Full Lines:    0 Full Lines:   0 Full Lines:    1 Full Line:
// |xxxxx|          |x     |        |xxxxxx|         |xxxxxx|
// |xxxxx|                                           |x     |
// |x    |
- (int)numberOfFullLinesFromOffset:(int)offset
                            length:(int)length
                             width:(int)width;

- (int)numberOfFullLinesFromBuffer:(const screen_char_t *)buffer
                            length:(int)length
                             width:(int)width;
#if BETA
int iTermLineBlockNumberOfFullLinesImpl(const screen_char_t *buffer,
                                        int length,
                                        int width,
                                        BOOL mayHaveDoubleWidthCharacter);
#endif  // BETA

// Finds a where the nth line begins after wrapping and returns its offset from the start of the
// buffer.
//
// In the following example, this would return:
// pointer to a if n==0, pointer to g if n==1, asserts if n > 1
// |abcdef|
// |ghi   |
//
// It's more complex with double-width characters.
// In this example, suppose XX is a double-width character.
//
// Returns a pointer to a if n==0, pointer XX if n==1, asserts if n > 1:
// |abcde|   <- line is short after wrapping
// |XXzzzz|
// The slow code for dealing with DWCs is run only if mayHaveDwc is YES.
int OffsetOfWrappedLine(const screen_char_t* p, int n, int length, int width, BOOL mayHaveDwc);

// Returns a dictionary with the contents of this block. The data is a weak reference and will be
// invalid if the block is changed.
- (NSDictionary *)dictionary;

// Number of empty lines at the end of the block.
- (int)numberOfTrailingEmptyLines;

// Call this only before a line block has been created.
void EnableDoubleWidthCharacterLineCache(void);

- (void)addObserver:(id<iTermLineBlockObserver>)observer;
- (void)removeObserver:(id<iTermLineBlockObserver>)observer;
- (BOOL)hasObserver:(id<iTermLineBlockObserver>)observer;

- (void)setPartial:(BOOL)partial;
- (LineBlock *)cowCopy;

// For tests only
- (LineBlockMetadata)internalMetadataForLine:(int)line;
- (NSInteger)numberOfClients;
- (BOOL)hasOwner;

@end
