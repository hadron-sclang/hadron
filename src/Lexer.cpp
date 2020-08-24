#include "Lexer.hpp"

#ifdef DEBUG_LEXER
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif
#include "spdlog/spdlog.h"

#include <array>
#include <cstdlib>

#ifdef DEBUG_LEXER
#include <fstream>
#endif

// Design following https://nothings.org/computer/lexing.html

namespace {

// Point of state table is to minimize branching. So we only select final states when either we're in a corner
// case that will require exceptional processing (radix numbers) or things are clear what we're parsing (floats)
enum State : uint8_t {
    // non-final states
    sSpace = 0,
    sLeadZero = 1,
    sNumber = 2,  // can still have radix or decimal TODO: can do flats/sharps as a separate token?
    sPlus = 3,   // could either be addition or spaceless string concatenation
    sAsterisk = 4, // either multiplication or exponentiation
    sForwardSlash = 5,  // either division or a comment
    sInString = 6,
    sStringEscape = 7,  // escape character in string parsing
    sInQuoteSymbol = 8,
    sSymbolEscape = 9,
    sInSlashSymbol = 10,

    // final states
    sInteger = 11,
    sHexInteger = 12, // done, this can only be an integer
    sFloat = 13, // done, we have enough to delegate to strtod
    sRadix = 14,  // can have decimal but all bets on speed are off.
    sZero = 15,
    sAdd = 16,
    sStringCat = 17, // ++
    sPathCat = 18,   // +/+
    sSubtract = 19,
    sMultiply = 20,
    sExponentiate = 21,
    sDivide = 22,
    sModulo = 23,
    sString = 24,
    sQuoteSymbol = 25,
    sSlashSymbol = 26,

    sLexError = 27,

    // This has to stay the last state for the counting and static asserts to work correctly.
    sEndCode = 28,
};

constexpr uint8_t kFirstFinalState = 11;
constexpr uint8_t kNumStates = State::sEndCode + 1;

#ifdef DEBUG_LEXER
std::array<const char*, kNumStates> kStateNames = {
    "sSpace",
    "sLeadZero",
    "sNumber",
    "sPlus",
    "sAsterisk",
    "sForwardSlash",
    "sInString",
    "sStringEscape",
    "sInQuoteSymbol",
    "sSymbolEscape",
    "sInSlashSymbol",
    "sInteger",
    "sHexInteger",
    "sFloat",
    "sRadix",
    "sZero",
    "sAdd",
    "sStringCat",
    "sPathCat",
    "sSubtract",
    "sMultiply",
    "sExponentiate",
    "sDivide",
    "sModulo",
    "sString",
    "sQuoteSymbol",
    "sSlashSymbol",
    "sLexError",
    "sEndCode"
};
#endif

enum CharacterClass : uint16_t {
    cSpace = 0 * kNumStates,        // space, tab
    cNewline = 1 * kNumStates,      // \n, \r
    cZero = 2 * kNumStates,         // 0
    cDigit = 3 * kNumStates,        // 1-9
    cPeriod = 4 * kNumStates,       // .
    cLowerX = 5 * kNumStates,       // a lower-case x, possibly for hexadecimal
    cPlus = 6 * kNumStates,         // +
    cHyphen = 7 * kNumStates,       // -
    cDoubleQuote = 8 * kNumStates,  // "
    cBackSlash = 9 * kNumStates,    // backslash
    cSingleQuote = 10 * kNumStates, // '
    cAlphaLower = 11 * kNumStates,  // a-z except for x
    cAlphaUpper = 12 * kNumStates,  // A-Z
    cUnderscore = 13 * kNumStates,  // _
    cOpenParen = 14 * kNumStates,   // (
    cCloseParen = 15 * kNumStates   // )
    cInvalid = 14 * kNumStates,     // unsupported character
    // This has to stay the last character class.
    cEnd = 15 * kNumStates,        // EOF, \0
};

#ifdef DEBUG_LEXER
std::array<const char*, cEnd / kNumStates + 1> kClassNames = {
    "\\\\w",    // cSpace
    "\\\\n",    // cNewline
    "0",        // cZero
    "1-9",      // cDigit
    ".",        // cPeriod
    "x",        // cLowerX
    "+",        // cPlus
    "-",        // cHyphen
    "\\\"",     // cDoubleQuote
    "\\\\",     // cBackSlash
    "'",        // cSingleQuote
    "a-z",      // cAlphaLower
    "A-Z",      // cAlphaUpper
    "_",        // cUnderscore
    "(",        // cOpenParen
    ")",        // cCloseParen
    "invalid",  // cInvalid
    "\\\\0"     // cEnd
};
#endif

std::array<State, CharacterClass::cEnd + kNumStates> kStateTransitionTable = {
    // CharacterClass = cSpace
    /* sSpace         => */ sSpace,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sMultiply,
    /* sForwardSlash  => */ sDivide,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sSlashSymbol,
    /* sInteger       => */ sSpace,
    /* sHexInteger    => */ sSpace,
    /* sFloat         => */ sSpace,
    /* sRadix         => */ sSpace,
    /* sZero          => */ sSpace,
    /* sAdd           => */ sSpace,
    /* sStringCat     => */ sSpace,
    /* sPathCat       => */ sSpace,
    /* sSubtract      => */ sSpace,
    /* sMultiply      => */ sSpace,
    /* sExponentiate  => */ sSpace,
    /* sDivide        => */ sSpace,
    /* sModulo        => */ sSpace,
    /* sString        => */ sSpace,
    /* sQuoteSymbol   => */ sSpace,
    /* sSlashSymbol   => */ sSpace,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cNewline
    /* sSpace         => */ sSpace,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sMultiply,
    /* sForwardSlash  => */ sDivide,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sSlashSymbol,
    /* sInteger       => */ sSpace,
    /* sHexInteger    => */ sSpace,
    /* sFloat         => */ sSpace,
    /* sRadix         => */ sSpace,
    /* sZero          => */ sSpace,
    /* sAdd           => */ sSpace,
    /* sStringCat     => */ sSpace,
    /* sPathCat       => */ sSpace,
    /* sSubtract      => */ sSpace,
    /* sMultiply      => */ sSpace,
    /* sExponentiate  => */ sSpace,
    /* sDivide        => */ sSpace,
    /* sModulo        => */ sSpace,
    /* sString        => */ sSpace,
    /* sQuoteSymbol   => */ sSpace,
    /* sSlashSymbol   => */ sSpace,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cZero
    /* sSpace         => */ sLeadZero,
    /* sLeadZero      => */ sLeadZero,  // many leading zeros treated as a single leading zero in sclang
    /* sNumber        => */ sNumber,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sLeadZero,
    /* sForwardSlash  => */ sLeadZero,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sInSlashSymbol,
    /* sInteger       => */ sLexError,
    /* sHexInteger    => */ sLexError,
    /* sFloat         => */ sLexError,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sLexError,
    /* sAdd           => */ sLeadZero,
    /* sStringCat     => */ sLeadZero,
    /* sPathCat       => */ sLeadZero,
    /* sSubtract      => */ sLeadZero,
    /* sMultiply      => */ sLeadZero,
    /* sExponentiate  => */ sLeadZero,
    /* sDivide        => */ sLeadZero,
    /* sModulo        => */ sLeadZero,
    /* sString        => */ sLeadZero,
    /* sQuoteSymbol   => */ sLeadZero,
    /* sSlashSymbol   => */ sLeadZero,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cDigit
    /* sSpace         => */ sNumber,
    /* sLeadZero      => */ sNumber,
    /* sNumber        => */ sNumber,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sMultiply,
    /* sForwardSlash  => */ sDivide,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sInSlashSymbol,
    /* sInteger       => */ sLexError,
    /* sHexInteger    => */ sLexError,
    /* sFloat         => */ sLexError,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sLexError,
    /* sAdd           => */ sNumber,
    /* sStringCat     => */ sNumber,
    /* sPathCat       => */ sNumber,
    /* sSubtract      => */ sNumber,
    /* sMultiply      => */ sNumber,
    /* sExponentiate  => */ sNumber,
    /* sDivide        => */ sNumber,
    /* sModulo        => */ sNumber,
    /* sString        => */ sNumber,
    /* sQuoteSymbol   => */ sNumber,
    /* sSlashSymbol   => */ sNumber,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cPeriod
    /* sSpace         => */ sLexError,
    /* sLeadZero      => */ sFloat,
    /* sNumber        => */ sFloat,
    /* sPlus          => */ sLexError,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sSlashSymbol,
    /* sInteger       => */ sLexError,
    /* sHexInteger    => */ sLexError,
    /* sFloat         => */ sLexError,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sLexError,
    /* sAdd           => */ sLexError,
    /* sStringCat     => */ sLexError,
    /* sPathCat       => */ sLexError,
    /* sSubtract      => */ sLexError,
    /* sMultiply      => */ sLexError,
    /* sExponentiate  => */ sLexError,
    /* sDivide        => */ sLexError,
    /* sModulo        => */ sLexError,
    /* sString        => */ sLexError,
    /* sQuoteSymbol   => */ sLexError,
    /* sSlashSymbol   => */ sLexError,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cLowerX
    /* sSpace         => */ sLexError,
    /* sLeadZero      => */ sHexInteger,
    /* sNumber        => */ sLexError,
    /* sPlus          => */ sLexError,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sInSlashSymbol,
    /* sInteger       => */ sLexError,
    /* sHexInteger    => */ sLexError,
    /* sFloat         => */ sLexError,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sLexError,
    /* sAdd           => */ sLexError,
    /* sStringCat     => */ sLexError,
    /* sPathCat       => */ sLexError,
    /* sSubtract      => */ sLexError,
    /* sMultiply      => */ sLexError,
    /* sExponentiate  => */ sLexError,
    /* sDivide        => */ sLexError,
    /* sModulo        => */ sLexError,
    /* sString        => */ sLexError,
    /* sQuoteSymbol   => */ sLexError,
    /* sSlashSymbol   => */ sLexError,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cPlus
    /* sSpace         => */ sPlus,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sStringCat,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sPathCat,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sSlashSymbol,
    /* sInteger       => */ sPlus,
    /* sHexInteger    => */ sPlus,
    /* sFloat         => */ sPlus,
    /* sRadix         => */ sPlus,
    /* sZero          => */ sPlus,
    /* sAdd           => */ sLexError,
    /* sStringCat     => */ sLexError,
    /* sPathCat       => */ sLexError,
    /* sSubtract      => */ sLexError,
    /* sMultiply      => */ sLexError,
    /* sExponentiate  => */ sLexError,
    /* sDivide        => */ sLexError,
    /* sModulo        => */ sLexError,
    /* sString        => */ sPlus,
    /* sQuoteSymbol   => */ sPlus,
    /* sSlashSymbol   => */ sPlus,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cHyphen
    /* sSpace         => */ sSubtract,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sLexError,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sSlashSymbol,
    /* sInteger       => */ sSubtract,
    /* sHexInteger    => */ sSubtract,
    /* sFloat         => */ sSubtract,
    /* sRadix         => */ sSubtract,
    /* sZero          => */ sSubtract,
    /* sAdd           => */ sLexError,
    /* sStringCat     => */ sLexError,
    /* sPathCat       => */ sLexError,
    /* sSubtract      => */ sLexError,
    /* sMultiply      => */ sLexError,
    /* sExponentiate  => */ sLexError,
    /* sDivide        => */ sLexError,
    /* sModulo        => */ sLexError,
    /* sString        => */ sSubtract,
    /* sQuoteSymbol   => */ sSubtract,
    /* sSlashSymbol   => */ sSubtract,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cDoubleQuote
    /* sSpace         => */ sInString,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sSlashSymbol,
    /* sInteger       => */ sInString,
    /* sHexInteger    => */ sInString,
    /* sFloat         => */ sInString,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sInString,
    /* sAdd           => */ sInString,
    /* sStringCat     => */ sInString,
    /* sPathCat       => */ sInString,
    /* sSubtract      => */ sInString,
    /* sMultiply      => */ sInString,
    /* sExponentiate  => */ sInString,
    /* sDivide        => */ sInString,
    /* sModulo        => */ sInString,
    /* sString        => */ sInString,
    /* sQuoteSymbol   => */ sInString,
    /* sSlashSymbol   => */ sInString,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cBackSlash
    /* sSpace         => */ sInSlashSymbol,
    /* sLeadZero      => */ sLexError,
    /* sNumber        => */ sLexError,
    /* sPlus          => */ sLexError,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sStringEscape,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sSymbolEscape,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sSlashSymbol,
    /* sInteger       => */ sLexError,
    /* sHexInteger    => */ sLexError,
    /* sFloat         => */ sLexError,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sLexError,
    /* sAdd           => */ sLexError,
    /* sStringCat     => */ sLexError,
    /* sPathCat       => */ sLexError,
    /* sSubtract      => */ sLexError,
    /* sMultiply      => */ sLexError,
    /* sExponentiate  => */ sLexError,
    /* sDivide        => */ sLexError,
    /* sModulo        => */ sLexError,
    /* sString        => */ sLexError,
    /* sQuoteSymbol   => */ sLexError,
    /* sSlashSymbol   => */ sInSlashSymbol,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cSingleQuote
    /* sSpace         => */ sInQuoteSymbol,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sSlashSymbol,
    /* sInteger       => */ sInQuoteSymbol,
    /* sHexInteger    => */ sInQuoteSymbol,
    /* sFloat         => */ sInQuoteSymbol,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sInQuoteSymbol,
    /* sAdd           => */ sInQuoteSymbol,
    /* sStringCat     => */ sInQuoteSymbol,
    /* sPathCat       => */ sInQuoteSymbol,
    /* sSubtract      => */ sInQuoteSymbol,
    /* sMultiply      => */ sInQuoteSymbol,
    /* sExponentiate  => */ sInQuoteSymbol,
    /* sDivide        => */ sInQuoteSymbol,
    /* sModulo        => */ sInQuoteSymbol,
    /* sString        => */ sInQuoteSymbol,
    /* sQuoteSymbol   => */ sInQuoteSymbol,
    /* sSlashSymbol   => */ sInQuoteSymbol,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class cAlphaLower
    /* sSpace         => */ sInQuoteSymbol,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sInSlashSymbol,
    /* sInteger       => */ sLexError,
    /* sHexInteger    => */ sLexError,
    /* sFloat         => */ sLexError,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sLexError,
    /* sAdd           => */ sLexError,
    /* sStringCat     => */ sLexError,
    /* sPathCat       => */ sLexError,
    /* sSubtract      => */ sLexError,
    /* sMultiply      => */ sLexError,
    /* sExponentiate  => */ sLexError,
    /* sDivide        => */ sLexError,
    /* sModulo        => */ sLexError,
    /* sString        => */ sLexError,
    /* sQuoteSymbol   => */ sLexError,
    /* sSlashSymbol   => */ sLexError,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class cAlphaUpper
    /* sSpace         => */ sInQuoteSymbol,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sInSlashSymbol,
    /* sInteger       => */ sLexError,
    /* sHexInteger    => */ sLexError,
    /* sFloat         => */ sLexError,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sLexError,
    /* sAdd           => */ sLexError,
    /* sStringCat     => */ sLexError,
    /* sPathCat       => */ sLexError,
    /* sSubtract      => */ sLexError,
    /* sMultiply      => */ sLexError,
    /* sExponentiate  => */ sLexError,
    /* sDivide        => */ sLexError,
    /* sModulo        => */ sLexError,
    /* sString        => */ sLexError,
    /* sQuoteSymbol   => */ sLexError,
    /* sSlashSymbol   => */ sLexError,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class cUnderscore
    /* sSpace         => */ sInQuoteSymbol,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sInString,
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sInSlashSymbol,
    /* sInteger       => */ sLexError,
    /* sHexInteger    => */ sLexError,
    /* sFloat         => */ sLexError,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sLexError,
    /* sAdd           => */ sLexError,
    /* sStringCat     => */ sLexError,
    /* sPathCat       => */ sLexError,
    /* sSubtract      => */ sLexError,
    /* sMultiply      => */ sLexError,
    /* sExponentiate  => */ sLexError,
    /* sDivide        => */ sLexError,
    /* sModulo        => */ sLexError,
    /* sString        => */ sLexError,
    /* sQuoteSymbol   => */ sLexError,
    /* sSlashSymbol   => */ sLexError,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cInvalid
    /* sSpace         => */ sLexError,
    /* sLeadZero      => */ sLexError,
    /* sNumber        => */ sLexError,
    /* sPlus          => */ sLexError,
    /* sAsterisk      => */ sLexError,
    /* sForwardSlash  => */ sLexError,
    /* sInString      => */ sInString, // <== for UTF-8 support
    /* sStringEscape  => */ sInString,
    /* sInQuoteSymbol => */ sInQuoteSymbol,
    /* sSymbolEscape  => */ sInQuoteSymbol,
    /* sInSlashSymbol => */ sLexError,
    /* sInteger       => */ sLexError,
    /* sHexInteger    => */ sLexError,
    /* sFloat         => */ sLexError,
    /* sRadix         => */ sLexError,
    /* sZero          => */ sLexError,
    /* sAdd           => */ sLexError,
    /* sStringCat     => */ sLexError,
    /* sPathCat       => */ sLexError,
    /* sSubtract      => */ sLexError,
    /* sMultiply      => */ sLexError,
    /* sExponentiate  => */ sLexError,
    /* sDivide        => */ sLexError,
    /* sModulo        => */ sLexError,
    /* sString        => */ sLexError,
    /* sQuoteSymbol   => */ sLexError,
    /* sSlashSymbol   => */ sLexError,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError,

    // Class = cEnd
    /* sSpace         => */ sEndCode,
    /* sLeadZero      => */ sZero,
    /* sNumber        => */ sInteger,
    /* sPlus          => */ sAdd,
    /* sAsterisk      => */ sMultiply,
    /* sForwardSlash  => */ sDivide,
    /* sInString      => */ sLexError,
    /* sStringEscape  => */ sLexError,
    /* sInQuoteSymbol => */ sLexError,
    /* sSymbolEscape  => */ sLexError,
    /* sInSlashSymbol => */ sSlashSymbol,
    /* sInteger       => */ sEndCode,
    /* sHexInteger    => */ sEndCode,
    /* sFloat         => */ sEndCode,
    /* sRadix         => */ sEndCode,
    /* sZero          => */ sEndCode,
    /* sAdd           => */ sEndCode,
    /* sStringCat     => */ sEndCode,
    /* sPathCat       => */ sEndCode,
    /* sSubtract      => */ sEndCode,
    /* sMultiply      => */ sEndCode,
    /* sExponentiate  => */ sEndCode,
    /* sDivide        => */ sEndCode,
    /* sModulo        => */ sEndCode,
    /* sString        => */ sEndCode,
    /* sQuoteSymbol   => */ sEndCode,
    /* sSlashSymbol   => */ sEndCode,
    /* sLexError      => */ sLexError,
    /* sEndCode       => */ sLexError
};

std::array<CharacterClass, 256> kCharacterClasses = {
    cEnd        /*   0 \0  */, cInvalid    /*   1 SOH */, cInvalid     /*   2 STX */, cInvalid     /*   3 ETX */,
    cInvalid    /*   4 EOT */, cEnd        /*   5 EOF */, cInvalid     /*   6 ACK */, cInvalid     /*   7 BEL */,
    cInvalid    /*   8 BS  */, cSpace      /*   9 \t  */, cNewline     /*  10 \n  */, cInvalid     /*  11 VT  */,
    cInvalid    /*  12 FF  */, cNewline    /*  13 \r  */, cInvalid     /*  14 SO  */, cInvalid     /*  15 SI  */,
    cInvalid    /*  16 DLE */, cInvalid    /*  17 DC1 */, cInvalid     /*  18 DC2 */, cInvalid     /*  19 DC3 */,
    cInvalid    /*  20 DC4 */, cInvalid    /*  21 NAK */, cInvalid     /*  22 SYN */, cInvalid     /*  23 ETB */,
    cInvalid    /*  24 CAN */, cInvalid    /*  25 EM  */, cInvalid     /*  26 SUB */, cInvalid     /*  27 ESC */,
    cInvalid    /*  28 FS  */, cInvalid    /*  29 FS  */, cInvalid     /*  30 RS  */, cInvalid     /*  31 US  */,
    cSpace      /*  32 SPC */, cInvalid    /*  33 !   */, cDoubleQuote /*  34 "   */, cInvalid     /*  35 #   */,
    cInvalid    /*  36 $   */, cInvalid    /*  37 %   */, cInvalid     /*  38 &   */, cSingleQuote /*  39 '   */,
    cInvalid    /*  40 (   */, cInvalid    /*  41 )   */, cInvalid     /*  42 *   */, cPlus        /*  43 +   */,
    cInvalid    /*  44 ,   */, cHyphen     /*  45 -   */, cInvalid     /*  46 .   */, cInvalid     /*  47 /   */,
    cZero       /*  48 0   */, cDigit      /*  49 1   */, cDigit       /*  50 2   */, cDigit       /*  51 3   */,
    cDigit      /*  52 4   */, cDigit      /*  53 5   */, cDigit       /*  54 6   */, cDigit       /*  55 7   */,
    cDigit      /*  56 8   */, cDigit      /*  57 9   */, cInvalid     /*  58 :   */, cInvalid     /*  59 ;   */,
    cInvalid    /*  60 <   */, cInvalid    /*  61 =   */, cInvalid     /*  62 >   */, cInvalid     /*  63 ?   */,
    cInvalid    /*  64 @   */, cAlphaUpper /*  65 A   */, cAlphaUpper  /*  66 B   */, cAlphaUpper  /*  67 C   */,
    cAlphaUpper /*  68 D   */, cAlphaUpper /*  69 E   */, cAlphaUpper  /*  70 F   */, cAlphaUpper  /*  71 G   */,
    cAlphaUpper /*  72 H   */, cAlphaUpper /*  73 I   */, cAlphaUpper  /*  74 J   */, cAlphaUpper  /*  75 K   */,
    cAlphaUpper /*  76 L   */, cAlphaUpper /*  77 M   */, cAlphaUpper  /*  78 N   */, cAlphaUpper  /*  79 O   */,
    cAlphaUpper /*  80 P   */, cAlphaUpper /*  81 Q   */, cAlphaUpper  /*  82 R   */, cAlphaUpper  /*  83 S   */,
    cAlphaUpper /*  84 T   */, cAlphaUpper /*  85 U   */, cAlphaUpper  /*  86 V   */, cAlphaUpper  /*  87 W   */,
    cAlphaUpper /*  88 X   */, cAlphaUpper /*  89 Y   */, cAlphaUpper  /*  90 Z   */, cInvalid     /*  91 [   */,
    cBackSlash  /*  92 \   */, cInvalid    /*  93 ]   */, cInvalid     /*  94 ^   */, cUnderscore  /*  95 _   */,
    cInvalid    /*  96 `   */, cAlphaLower /*  97 a   */, cAlphaLower  /*  98 b   */, cAlphaLower  /*  99 c   */,
    cAlphaLower /* 100 d   */, cAlphaLower /* 101 e   */, cAlphaLower  /* 102 f   */, cAlphaLower  /* 103 g   */,
    cAlphaLower /* 104 h   */, cAlphaLower /* 105 i   */, cAlphaLower  /* 106 j   */, cAlphaLower  /* 107 k   */,
    cAlphaLower /* 108 l   */, cAlphaLower /* 109 m   */, cAlphaLower  /* 110 n   */, cAlphaLower  /* 111 o   */,
    cAlphaLower /* 112 p   */, cAlphaLower /* 113 q   */, cAlphaLower  /* 114 r   */, cAlphaLower  /* 115 s   */,
    cAlphaLower /* 116 t   */, cAlphaLower /* 117 u   */, cAlphaLower  /* 118 v   */, cAlphaLower  /* 119 w   */,
    cLowerX     /* 120 x   */, cAlphaLower /* 121 y   */, cAlphaLower  /* 122 z   */, cInvalid     /* 123 {   */,
    cInvalid    /* 124 |   */, cInvalid    /* 125 }   */, cInvalid     /* 126 ~   */, cInvalid     /* 127 DEL */,
    cInvalid    /* 128     */, cInvalid    /* 129     */, cInvalid     /* 130     */, cInvalid     /* 131     */,
    cInvalid    /* 132     */, cInvalid    /* 133     */, cInvalid     /* 134     */, cInvalid     /* 135     */,
    cInvalid    /* 136     */, cInvalid    /* 137     */, cInvalid     /* 138     */, cInvalid     /* 139     */,
    cInvalid    /* 140     */, cInvalid    /* 141     */, cInvalid     /* 142     */, cInvalid     /* 143     */,
    cInvalid    /* 144     */, cInvalid    /* 145     */, cInvalid     /* 146     */, cInvalid     /* 147     */,
    cInvalid    /* 148     */, cInvalid    /* 149     */, cInvalid     /* 150     */, cInvalid     /* 151     */,
    cInvalid    /* 152     */, cInvalid    /* 153     */, cInvalid     /* 154     */, cInvalid     /* 155     */,
    cInvalid    /* 156     */, cInvalid    /* 157     */, cInvalid     /* 158     */, cInvalid     /* 159     */,
    cInvalid    /* 160     */, cInvalid    /* 161     */, cInvalid     /* 162     */, cInvalid     /* 163     */,
    cInvalid    /* 164     */, cInvalid    /* 165     */, cInvalid     /* 166     */, cInvalid     /* 167     */,
    cInvalid    /* 168     */, cInvalid    /* 169     */, cInvalid     /* 170     */, cInvalid     /* 171     */,
    cInvalid    /* 172     */, cInvalid    /* 173     */, cInvalid     /* 174     */, cInvalid     /* 175     */,
    cInvalid    /* 176     */, cInvalid    /* 177     */, cInvalid     /* 178     */, cInvalid     /* 179     */,
    cInvalid    /* 180     */, cInvalid    /* 181     */, cInvalid     /* 182     */, cInvalid     /* 183     */,
    cInvalid    /* 184     */, cInvalid    /* 185     */, cInvalid     /* 186     */, cInvalid     /* 187     */,
    cInvalid    /* 188     */, cInvalid    /* 189     */, cInvalid     /* 190     */, cInvalid     /* 191     */,
    cInvalid    /* 192     */, cInvalid    /* 193     */, cInvalid     /* 194     */, cInvalid     /* 195     */,
    cInvalid    /* 196     */, cInvalid    /* 197     */, cInvalid     /* 198     */, cInvalid     /* 199     */,
    cInvalid    /* 200     */, cInvalid    /* 201     */, cInvalid     /* 202     */, cInvalid     /* 203     */,
    cInvalid    /* 204     */, cInvalid    /* 205     */, cInvalid     /* 206     */, cInvalid     /* 207     */,
    cInvalid    /* 208     */, cInvalid    /* 209     */, cInvalid     /* 210     */, cInvalid     /* 211     */,
    cInvalid    /* 212     */, cInvalid    /* 213     */, cInvalid     /* 214     */, cInvalid     /* 215     */,
    cInvalid    /* 216     */, cInvalid    /* 217     */, cInvalid     /* 218     */, cInvalid     /* 219     */,
    cInvalid    /* 220     */, cInvalid    /* 221     */, cInvalid     /* 222     */, cInvalid     /* 223     */,
    cInvalid    /* 224     */, cInvalid    /* 225     */, cInvalid     /* 226     */, cInvalid     /* 227     */,
    cInvalid    /* 228     */, cInvalid    /* 229     */, cInvalid     /* 230     */, cInvalid     /* 231     */,
    cInvalid    /* 232     */, cInvalid    /* 233     */, cInvalid     /* 234     */, cInvalid     /* 235     */,
    cInvalid    /* 236     */, cInvalid    /* 237     */, cInvalid     /* 238     */, cInvalid     /* 239     */,
    cInvalid    /* 240     */, cInvalid    /* 241     */, cInvalid     /* 242     */, cInvalid     /* 243     */,
    cInvalid    /* 244     */, cInvalid    /* 245     */, cInvalid     /* 246     */, cInvalid     /* 247     */,
    cInvalid    /* 248     */, cInvalid    /* 249     */, cInvalid     /* 250     */, cInvalid     /* 251     */,
    cInvalid    /* 252     */, cInvalid    /* 253     */, cInvalid     /* 254     */, cInvalid     /* 255     */,
};

#ifdef DEBUG_LEXER
std::array<const char*, 128> kCharacterNames = {
    "\\0"    /*   0 \0  */, "1:SOH"  /*   1 SOH */, "2:STX"  /*   2 STX */, "3:ETX"   /*   3 ETX */,
    "4:EOT"  /*   4 EOT */, "5:EOF"  /*   5 EOF */, "6:ACK"  /*   6 ACK */, "7:BEL"   /*   7 BEL */,
    "8:BS"   /*   8 BS  */, "\\t"    /*   9 \t  */, "\\n"    /*  10 \n  */, "11:VT"   /*  11 VT  */,
    "12:FF"  /*  12 FF  */, "\\r"    /*  13 \r  */, "14:SO"  /*  14 SO  */, "15:SI"   /*  15 SI  */,
    "16:DLE" /*  16 DLE */, "17:DC1" /*  17 DC1 */, "18:DC2" /*  18 DC2 */, "19:DC3"  /*  19 DC3 */,
    "20:DC4" /*  20 DC4 */, "21:MAK" /*  21 NAK */, "22:SYN" /*  22 SYN */, "23:STB"  /*  23 ETB */,
    "24:CAN" /*  24 CAN */, "25:EM"  /*  25 EM  */, "26:SUB" /*  26 SUB */, "27:ESC"  /*  27 ESC */,
    "28:FS"  /*  28 FS  */, "29:FS"  /*  29 FS  */, "30:RS"  /*  30 RS  */, "31:US"   /*  31 US  */,
    " "      /*  32 SPC */, "!"      /*  33 !   */, "\""     /*  34 "   */, "#"       /*  35 #   */,
    "$"      /*  36 $   */, "%"      /*  37 %   */, "&"      /*  38 &   */, "'"       /*  39 '   */,
    "("      /*  40 (   */, ")"      /*  41 )   */, "*"      /*  42 *   */, "+"       /*  43 +   */,
    ","      /*  44 ,   */, "-"      /*  45 -   */, "."      /*  46 .   */, "/"       /*  47 /   */,
    "0"      /*  48 0   */, "1"      /*  49 1   */, "2"      /*  50 2   */, "3"       /*  51 3   */,
    "4"      /*  52 4   */, "5"      /*  53 5   */, "6"      /*  54 6   */, "7"       /*  55 7   */,
    "8"      /*  56 8   */, "9"      /*  57 9   */, ":"      /*  58 :   */, ";"       /*  59 ;   */,
    "<"      /*  60 <   */, "="      /*  61 =   */, ">"      /*  62 >   */, "?"       /*  63 ?   */,
    "@"      /*  64 @   */, "A"      /*  65 A   */, "B"      /*  66 B   */, "C"       /*  67 C   */,
    "D"      /*  68 D   */, "E"      /*  69 E   */, "F"      /*  70 F   */, "G"       /*  71 G   */,
    "H"      /*  72 H   */, "I"      /*  73 I   */, "J"      /*  74 J   */, "K"       /*  75 K   */,
    "L"      /*  76 L   */, "M"      /*  77 M   */, "N"      /*  78 N   */, "O"       /*  79 O   */,
    "P"      /*  80 P   */, "Q"      /*  81 Q   */, "R"      /*  82 R   */, "S"       /*  83 S   */,
    "T"      /*  84 T   */, "U"      /*  85 U   */, "V"      /*  86 V   */, "W"       /*  87 W   */,
    "X"      /*  88 X   */, "Y"      /*  89 Y   */, "Z"      /*  90 Z   */, "["       /*  91 [   */,
    "\\"     /*  92 \   */, "]"      /*  93 ]   */, "^"      /*  94 ^   */, "_"       /*  95 _   */,
    "`"      /*  96 `   */, "a"      /*  97 a   */, "b"      /*  98 b   */, "c"       /*  99 c   */,
    "d"      /* 100 d   */, "e"      /* 101 e   */, "f"      /* 102 f   */, "g"       /* 103 g   */,
    "h"      /* 104 h   */, "i"      /* 105 i   */, "j"      /* 106 j   */, "k"       /* 107 k   */,
    "l"      /* 108 l   */, "m"      /* 109 m   */, "n"      /* 110 n   */, "o"       /* 111 o   */,
    "p"      /* 112 p   */, "q"      /* 113 q   */, "r"      /* 114 r   */, "s"       /* 115 s   */,
    "t"      /* 116 t   */, "u"      /* 117 u   */, "v"      /* 118 v   */, "w"       /* 119 w   */,
    "x"      /* 120 x   */, "y"      /* 121 y   */, "z"      /* 122 z   */, "{"       /* 123 {   */,
    "|"      /* 124 |   */, "}"      /* 125 }   */, "~"      /* 126 ~   */, "127:DEL" /* 127 DEL */
};
#endif

std::array<int8_t, kNumStates> kStateLengths = {
    0, // sSpace
    1, // sLeadZero
    1, // sNumber
    1, // sPlus
    1, // sAsterisk
    1, // sForwardSlash
    1, // sInString
    1, // sStringEscape
    1, // sInQuoteSymbol
    1, // sSymbolEscape
    1, // sInSlashSymbol
    0, // sInteger
    0, // sHexInteger
    0, // sFloat
    0, // sRadix
    0, // sZero
    0, // sAdd
    0, // sStringCat
    0, // sPathCat
    0, // sSubtract
    0, // sMultiply
    0, // sExponentiate
    0, // sDivide
    0, // sModulo
    1, // sString - a 1 in a final state here will include the current character in the length.
    1, // sQuoteSymbol
    0, // sSlashSymbol
    0, // sLexError
    0, // sEndCode
};

#ifdef DEBUG_LEXER
void saveStatesLeadingTo(State targetState, std::array<bool, kStateTransitionTable.size()>& visited,
        std::array<bool, kNumStates>& knownStates, const char* suffix, std::ofstream& outFile) {
    // Scan through state transition table looking for targetState, if hasn't been visited yet add to file and
    // recurse back until all entry states have been exhausted
    for (size_t i = 0; i < kStateTransitionTable.size(); ++i) {
        if (kStateTransitionTable[i] == targetState && !visited[i]) {
            State entryState = static_cast<State>(i % kNumStates);
            if (entryState >= sInteger) {
                continue;
            }
            if (!knownStates[entryState]) {
                knownStates[entryState] = true;
                outFile << "    " << kStateNames[entryState] << suffix << " [label=" << kStateNames[entryState] << "]"
                    << std::endl;
            }
            CharacterClass characterClass = static_cast<CharacterClass>(i / kNumStates);
            outFile << "    " << kStateNames[entryState] << suffix << " -> " << kStateNames[targetState] << suffix
                    << "  [label=\"" << kClassNames[characterClass] << "\"]" << std::endl;
            visited[i] = true;
            saveStatesLeadingTo(entryState, visited, knownStates, suffix, outFile);
        }
    }
}
#endif
}

namespace hadron {

Lexer::Lexer(const char* code): m_code(code) { m_length = std::strlen(code); }
Lexer::Lexer(const char* code, size_t length): m_code(code), m_length(length) {}

bool Lexer::lex() {
    SPDLOG_DEBUG("** start of Lex on string \"{}\"", m_code);
    const char* code = m_code;
    const char* endCode = m_code + m_length;
    State state = sSpace;
    int characterClass = static_cast<int>(CharacterClass::cSpace);
    while (code < endCode) {
        int tokenLength = 0;
        do {
            int character = static_cast<int>(*code++);
            characterClass = static_cast<int>(kCharacterClasses[character]);
            SPDLOG_DEBUG("  character: '{}', class: {}, state: {}, length: {}", kCharacterNames[character & 0x7f],
                    kClassNames[characterClass / kNumStates], kStateNames[state], tokenLength);
            state = kStateTransitionTable[characterClass + state];
            tokenLength += kStateLengths[state];
        } while (state < kFirstFinalState);

        SPDLOG_DEBUG("final state: {}", kStateNames[state]);

        char* tokenEnd = nullptr;
        const char* tokenStart;
        switch(state) {
            // Non-final states go here and are a fatal lexing error, indicating a broken state table. There is no
            // default: case, so that the compiler can catch absent states with a warning.
            case sSpace:
            case sLeadZero:
            case sNumber:
            case sPlus:
            case sAsterisk:
            case sForwardSlash:
            case sInString:
            case sStringEscape:
            case sInQuoteSymbol:
            case sSymbolEscape:
            case sInSlashSymbol:
                state = sLexError;
                code = endCode;
                break;

            case sInteger: {
                // Exit state machine pointing just past the end of the integer.
                tokenStart = code - tokenLength - 1;
                int64_t intValue = std::strtoll(tokenStart, &tokenEnd, 10);
                if (tokenStart < tokenEnd) {
                    m_tokens.emplace_back(Token(tokenStart, tokenEnd - tokenStart, intValue));
                    // Reset pointer to the end of the integer.
                    code = tokenEnd;
                } else {
                    state = sLexError;
                    code = endCode;
                }
            } break;

            case sHexInteger: {
                // Exit state machine pointing just past the "0x", length can be > 2 because we support multiple leading
                // zeros, such as "0000x3" needs to lex to 3.
                tokenStart = code - tokenLength - 1;
                int64_t intValue = std::strtoll(code, &tokenEnd, 16);
                if (code < tokenEnd) {
                    m_tokens.emplace_back(Token(tokenStart, tokenEnd - tokenStart, intValue));
                    code = tokenEnd;
                } else {
                    state = sLexError;
                    code = endCode;
                }
            } break;

            case sFloat:
            case sRadix:
                state = sLexError;
                code = endCode;
                break;

            case sZero:
                tokenStart = code - tokenLength - 1;
                m_tokens.emplace_back(Token(tokenStart, tokenLength, 0LL));
                code = tokenStart + tokenLength;
                break;

            case sAdd:
                tokenStart = code - tokenLength - 1;
                m_tokens.emplace_back(Token(Token::Type::kAddition, tokenStart, tokenLength));
                // Add supports other symbols following without whitespace, so back up to re-evaluate next symbol.
                // (assumes tokenLength == 1)
                --code;
                break;

            case sStringCat:
            case sPathCat:
            case sSubtract:
            case sMultiply:
            case sExponentiate:
            case sDivide:
            case sModulo:
                state = sLexError;
                code = endCode;
                break;

            case sString:
                tokenStart = code - tokenLength;
                m_tokens.emplace_back(Token(Token::Type::kString, tokenStart, tokenLength));
                break;

            case sQuoteSymbol:
                tokenStart = code - tokenLength;
                m_tokens.emplace_back(Token(Token::Type::kSymbol, tokenStart, tokenLength));
                break;

            case sSlashSymbol:
                tokenStart = code - tokenLength - 1;
                m_tokens.emplace_back(Token(Token::Type::kSymbol, tokenStart, tokenLength));
                break;

            case sLexError:
                code = endCode;
                spdlog::error("got lex error");
                break;

            case sEndCode:
                code = endCode;
                break;
        }
    }

    return state != sLexError;
}

#ifdef DEBUG_LEXER
// static
void Lexer::saveLexerStateMachineGraph(const char* fileName) {
    std::ofstream outFile(fileName);
    if (!outFile) {
        SPDLOG_ERROR("unable to open lexer state machine graph output file {} for writing", fileName);
        return;
    }

    outFile << "digraph G {" << std::endl;
    outFile << "  ratio=0.125" << std::endl;

    State finalState = sInteger;
    while (finalState < sLexError) {
        outFile << "  subgraph " << kStateNames[finalState] << " {";
        outFile << "    " << kStateNames[finalState] << kStateNames[finalState] << "[peripheries=2,label="
                << kStateNames[finalState] << "]" << std::endl;
        std::array<bool, kStateTransitionTable.size()> visited{ false };
        std::array<bool, kNumStates> knownStates{ false };
        knownStates[finalState] = true;
        saveStatesLeadingTo(finalState, visited, knownStates, kStateNames[finalState], outFile);
        outFile << "  }" << std::endl;
        finalState = static_cast<State>(static_cast<uint8_t>(finalState) + 1);
    }

    outFile << "}" << std::endl;
}
#endif
}
