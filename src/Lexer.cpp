#include "Lexer.hpp"

#ifdef DEBUG_LEXER
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif
#include "spdlog/spdlog.h"

#include <array>
#include <cstdlib>
#include <regex>

#ifdef DEBUG_LEXER
#include <fstream>
#endif

// Design following https://nothings.org/computer/lexing.html

namespace {

// Point of state table is to minimize branching. So we only select final states when either we're in a corner
// case that will require exceptional processing (radix numbers) or things are clear what we're parsing (floats)
enum State : uint8_t {
    // non-final states
    sSpace =           0,
    sForwardSlash =    1,  // either division or a comment
    sInBlockComment =  2,
    sBlockAsterisk =   3,
    sInLineComment  =  4,

    // final states
    sNumber =          5,
    sString =          6,
    sQuoteSymbol =     7,
    sSlashSymbol =     8,
    sDelimiter =       9, // single character separators with no padding requirements, like (;:{[^~
    sIdentifier =     10, // words starting with lower alpha, could also be keywords
    sClassName =      11,
    sOperator =       12,
    sDot =            13,

    sLexError =       14,
    // This has to stay the last state for the counting and static asserts to work correctly.
    sEndCode =        15
};

constexpr uint8_t kFirstFinalState = 5;
constexpr uint8_t kNumStates = State::sEndCode + 1;

#ifdef DEBUG_LEXER
std::array<const char*, kNumStates> kStateNames = {
    "sSpace",
    "sForwardSlash",
    "sInBlockComment",
    "sBlockAsterisk",
    "sInLineComment",
    "sNumber",
    "sString",
    "sQuoteSymbol",
    "sSlashSymbol",
    "sDelimiter",
    "sIdentifier",
    "sClassName",
    "sOperator",
    "sLexError",
    "sEndCode"
};
#endif

enum CharacterClass : uint8_t {
    cSpace =        0 * kNumStates, // space, tab
    cNewline =      1 * kNumStates, // \n
    cDigit =        2 * kNumStates, // 0-9
    cAsterisk =     3 * kNumStates, // *
    cForwardSlash = 4 * kNumStates, // /
    cDoubleQuote =  5 * kNumStates, // "
    cBackSlash =    6 * kNumStates, // backslash
    cSingleQuote =  7 * kNumStates, // '
    cAlphaLower =   8 * kNumStates, // a-z
    cAlphaUpper =   9 * kNumStates, // A-Z
    cDelimiter =   10 * kNumStates, // {}()[]|;:,^~-%#` always single-character
    cOperator =    11 * kNumStates, // +-|&=!<> can be multi-character
    cDot =         12 * kNumStates, // .
    cInvalid =     13 * kNumStates, // unsupported character
    // This has to stay the last character class.
    cEnd =         14 * kNumStates, // EOF, \0
};

#ifdef DEBUG_LEXER
std::array<const char*, cEnd / kNumStates + 1> kClassNames = {
    "\\\\w",    // cSpace
    "\\\\n",    // cNewline
    "0-9",      // cDigit
    "*",        // cAsterisk
    "/",        // cForwardSlash
    "\\\"",     // cDoubleQuote
    "\\\\",     // cBackSlash
    "'",        // cSingleQuote
    "a-z",      // cAlphaLower
    "A-Z",      // cAlphaUpper
    "delim",    // cDelimiter
    "op",       // cOperator
    ".",        // cDot
    "invalid",  // cInvalid
    "\\\\0"     // cEnd
};
#endif

std::array<State, CharacterClass::cEnd + kNumStates> kStateTransitionTable = {
    /* ====== CharacterClass = cSpace ====== */
    /* sSpace          => */ sSpace,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sSpace,
    /* sString         => */ sSpace,
    /* sQuoteSymbol    => */ sSpace,
    /* sSlashSymbol    => */ sSpace,
    /* sDelimiter      => */ sSpace,
    /* sIdentifier     => */ sSpace,
    /* sClassName      => */ sSpace,
    /* sOperator       => */ sSpace,
    /* sDot            => */ sSpace,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cNewLine ====== */
    /* sSpace          => */ sSpace,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sSpace,
    /* ---- Final States  */
    /* sNumber         => */ sSpace,
    /* sString         => */ sSpace,
    /* sQuoteSymbol    => */ sSpace,
    /* sSlashSymbol    => */ sSpace,
    /* sDelimiter      => */ sSpace,
    /* sIdentifier     => */ sSpace,
    /* sClassName      => */ sSpace,
    /* sOperator       => */ sSpace,
    /* sDot            => */ sSpace,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cDigit ====== */
    /* sSpace          => */ sNumber,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sLexError,
    /* sString         => */ sNumber,
    /* sQuoteSymbol    => */ sNumber,
    /* sSlashSymbol    => */ sLexError,
    /* sDelimiter      => */ sNumber,
    /* sIdentifier     => */ sLexError,
    /* sClassName      => */ sLexError,
    /* sOperator       => */ sNumber,
    /* sDot            => */ sNumber,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cAsterisk ====== */
    /* sSpace          => */ sOperator,
    /* sForwardSlash   => */ sInBlockComment,
    /* sInBlockComment => */ sBlockAsterisk,
    /* sBlockAsterisk  => */ sBlockAsterisk,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sOperator,
    /* sString         => */ sOperator,
    /* sQuoteSymbol    => */ sOperator,
    /* sSlashSymbol    => */ sOperator,
    /* sDelimiter      => */ sOperator,
    /* sIdentifier     => */ sOperator,
    /* sClassName      => */ sOperator,
    /* sOperator       => */ sLexError,
    /* sDot            => */ sOperator,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cForwardSlash ====== */
    /* sSpace          => */ sForwardSlash,
    /* sForwardSlash   => */ sInLineComment,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sSpace,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sForwardSlash,
    /* sString         => */ sForwardSlash,
    /* sQuoteSymbol    => */ sForwardSlash,
    /* sSlashSymbol    => */ sForwardSlash,
    /* sDelimiter      => */ sForwardSlash,
    /* sIdentifier     => */ sForwardSlash,
    /* sClassName      => */ sForwardSlash,
    /* sOperator       => */ sForwardSlash,
    /* sDot            => */ sForwardSlash,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cDoubleQuote ====== */
    /* sSpace          => */ sString,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sString,
    /* sString         => */ sString,
    /* sQuoteSymbol    => */ sString,
    /* sSlashSymbol    => */ sString,
    /* sDelimiter      => */ sString,
    /* sIdentifier     => */ sString,
    /* sClassName      => */ sString,
    /* sOperator       => */ sString,
    /* sDot            => */ sString,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cBackSlash ====== */
    /* sSpace          => */ sSlashSymbol,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sSlashSymbol,
    /* sString         => */ sSlashSymbol,
    /* sQuoteSymbol    => */ sSlashSymbol,
    /* sSlashSymbol    => */ sSlashSymbol,
    /* sDelimiter      => */ sSlashSymbol,
    /* sIdentifier     => */ sSlashSymbol,
    /* sClassName      => */ sSlashSymbol,
    /* sOperator       => */ sSlashSymbol,
    /* sDot            => */ sSlashSymbol,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cSingleQuote ====== */
    /* sSpace          => */ sQuoteSymbol,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sQuoteSymbol,
    /* sString         => */ sQuoteSymbol,
    /* sQuoteSymbol    => */ sQuoteSymbol,
    /* sSlashSymbol    => */ sQuoteSymbol,
    /* sDelimiter      => */ sQuoteSymbol,
    /* sIdentifier     => */ sLexError,
    /* sClassName      => */ sQuoteSymbol,
    /* sOperator       => */ sQuoteSymbol,
    /* sDot            => */ sQuoteSymbol,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cAlphaLower ====== */
    /* sSpace          => */ sIdentifier,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sIdentifier,
    /* sString         => */ sIdentifier,
    /* sQuoteSymbol    => */ sIdentifier,
    /* sSlashSymbol    => */ sIdentifier,
    /* sDelimiter      => */ sIdentifier,
    /* sIdentifier     => */ sLexError,
    /* sClassName      => */ sLexError,
    /* sOperator       => */ sIdentifier,
    /* sDot            => */ sIdentifier,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cAlphaUpper ====== */
    /* sSpace          => */ sClassName,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sClassName,
    /* sString         => */ sClassName,
    /* sQuoteSymbol    => */ sClassName,
    /* sSlashSymbol    => */ sClassName,
    /* sDelimiter      => */ sClassName,
    /* sIdentifier     => */ sLexError,
    /* sClassName      => */ sLexError,
    /* sOperator       => */ sClassName,
    /* sDot            => */ sClassName,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cDelimiter ====== */
    /* sSpace          => */ sDelimiter,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sDelimiter,
    /* sString         => */ sDelimiter,
    /* sQuoteSymbol    => */ sDelimiter,
    /* sSlashSymbol    => */ sDelimiter,
    /* sDelimiter      => */ sDelimiter,
    /* sIdentifier     => */ sDelimiter,
    /* sClassName      => */ sDelimiter,
    /* sOperator       => */ sDelimiter,
    /* sDot            => */ sDelimiter,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cOperator ====== */
    /* sSpace          => */ sOperator,
    /* sForwardSlash   => */ sOperator,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sOperator,
    /* sString         => */ sOperator,
    /* sQuoteSymbol    => */ sOperator,
    /* sSlashSymbol    => */ sOperator,
    /* sDelimiter      => */ sOperator,
    /* sIdentifier     => */ sOperator,
    /* sClassName      => */ sOperator,
    /* sOperator       => */ sOperator,
    /* sDot            => */ sOperator,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cDot ====== */
    /* sSpace          => */ sDot,
    /* sForwardSlash   => */ sLexError,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sDot,
    /* sString         => */ sDot,
    /* sQuoteSymbol    => */ sDot,
    /* sSlashSymbol    => */ sDot,
    /* sDelimiter      => */ sDot,
    /* sIdentifier     => */ sDot,
    /* sClassName      => */ sDot,
    /* sOperator       => */ sDot,
    /* sDot            => */ sLexError,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cInvalid ====== */
    /* sSpace          => */ sLexError,
    /* sForwardSlash   => */ sLexError,
    /* sInBlockComment => */ sInBlockComment,
    /* sBlockAsterisk  => */ sInBlockComment,
    /* sInLineComment  => */ sInLineComment,
    /* ---- Final States  */
    /* sNumber         => */ sLexError,
    /* sString         => */ sLexError,
    /* sQuoteSymbol    => */ sLexError,
    /* sSlashSymbol    => */ sLexError,
    /* sDelimiter      => */ sLexError,
    /* sIdentifier     => */ sLexError,
    /* sClassName      => */ sLexError,
    /* sOperator       => */ sLexError,
    /* sDot            => */ sLexError,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,

    /* ====== CharacterClass = cEnd ====== */
    /* sSpace          => */ sEndCode,
    /* sForwardSlash   => */ sLexError,
    /* sInBlockComment => */ sLexError,
    /* sBlockAsterisk  => */ sLexError,
    /* sInLineComment  => */ sEndCode,
    /* ---- Final States  */
    /* sNumber         => */ sEndCode,
    /* sString         => */ sEndCode,
    /* sQuoteSymbol    => */ sEndCode,
    /* sSlashSymbol    => */ sEndCode,
    /* sDelimiter      => */ sEndCode,
    /* sIdentifier     => */ sEndCode,
    /* sClassName      => */ sEndCode,
    /* sOperator       => */ sEndCode,
    /* sDot            => */ sEndCode,
    /* sLexError       => */ sLexError,
    /* sEndCode        => */ sEndCode,
};

std::array<CharacterClass, 256> kCharacterClasses = {
    cEnd        /*   0 \0  */, cInvalid    /*   1 SOH */, cInvalid     /*   2 STX */, cInvalid      /*   3 ETX */,
    cInvalid    /*   4 EOT */, cEnd        /*   5 EOF */, cInvalid     /*   6 ACK */, cInvalid      /*   7 BEL */,
    cInvalid    /*   8 BS  */, cSpace      /*   9 \t  */, cNewline     /*  10 \n  */, cInvalid      /*  11 VT  */,
    cInvalid    /*  12 FF  */, cNewline    /*  13 \r  */, cInvalid     /*  14 SO  */, cInvalid      /*  15 SI  */,
    cInvalid    /*  16 DLE */, cInvalid    /*  17 DC1 */, cInvalid     /*  18 DC2 */, cInvalid      /*  19 DC3 */,
    cInvalid    /*  20 DC4 */, cInvalid    /*  21 NAK */, cInvalid     /*  22 SYN */, cInvalid      /*  23 ETB */,
    cInvalid    /*  24 CAN */, cInvalid    /*  25 EM  */, cInvalid     /*  26 SUB */, cInvalid      /*  27 ESC */,
    cInvalid    /*  28 FS  */, cInvalid    /*  29 FS  */, cInvalid     /*  30 RS  */, cInvalid      /*  31 US  */,
    cSpace      /*  32 SPC */, cOperator   /*  33 !   */, cDoubleQuote /*  34 "   */, cDelimiter    /*  35 #   */,
    cInvalid    /*  36 $   */, cOperator   /*  37 %   */, cOperator    /*  38 &   */, cSingleQuote  /*  39 '   */,
    cDelimiter  /*  40 (   */, cDelimiter  /*  41 )   */, cAsterisk    /*  42 *   */, cOperator     /*  43 +   */,
    cDelimiter  /*  44 ,   */, cOperator   /*  45 -   */, cDot         /*  46 .   */, cForwardSlash /*  47 /   */,
    cDigit      /*  48 0   */, cDigit      /*  49 1   */, cDigit       /*  50 2   */, cDigit        /*  51 3   */,
    cDigit      /*  52 4   */, cDigit      /*  53 5   */, cDigit       /*  54 6   */, cDigit        /*  55 7   */,
    cDigit      /*  56 8   */, cDigit      /*  57 9   */, cDelimiter   /*  58 :   */, cDelimiter    /*  59 ;   */,
    cOperator   /*  60 <   */, cOperator   /*  61 =   */, cOperator    /*  62 >   */, cOperator     /*  63 ?   */,
    cOperator   /*  64 @   */, cAlphaUpper /*  65 A   */, cAlphaUpper  /*  66 B   */, cAlphaUpper   /*  67 C   */,
    cAlphaUpper /*  68 D   */, cAlphaUpper /*  69 E   */, cAlphaUpper  /*  70 F   */, cAlphaUpper   /*  71 G   */,
    cAlphaUpper /*  72 H   */, cAlphaUpper /*  73 I   */, cAlphaUpper  /*  74 J   */, cAlphaUpper   /*  75 K   */,
    cAlphaUpper /*  76 L   */, cAlphaUpper /*  77 M   */, cAlphaUpper  /*  78 N   */, cAlphaUpper   /*  79 O   */,
    cAlphaUpper /*  80 P   */, cAlphaUpper /*  81 Q   */, cAlphaUpper  /*  82 R   */, cAlphaUpper   /*  83 S   */,
    cAlphaUpper /*  84 T   */, cAlphaUpper /*  85 U   */, cAlphaUpper  /*  86 V   */, cAlphaUpper   /*  87 W   */,
    cAlphaUpper /*  88 X   */, cAlphaUpper /*  89 Y   */, cAlphaUpper  /*  90 Z   */, cDelimiter    /*  91 [   */,
    cBackSlash  /*  92 \   */, cDelimiter  /*  93 ]   */, cDelimiter   /*  94 ^   */, cInvalid      /*  95 _   */,
    cInvalid    /*  96 `   */, cAlphaLower /*  97 a   */, cAlphaLower  /*  98 b   */, cAlphaLower   /*  99 c   */,
    cAlphaLower /* 100 d   */, cAlphaLower /* 101 e   */, cAlphaLower  /* 102 f   */, cAlphaLower   /* 103 g   */,
    cAlphaLower /* 104 h   */, cAlphaLower /* 105 i   */, cAlphaLower  /* 106 j   */, cAlphaLower   /* 107 k   */,
    cAlphaLower /* 108 l   */, cAlphaLower /* 109 m   */, cAlphaLower  /* 110 n   */, cAlphaLower   /* 111 o   */,
    cAlphaLower /* 112 p   */, cAlphaLower /* 113 q   */, cAlphaLower  /* 114 r   */, cAlphaLower   /* 115 s   */,
    cAlphaLower /* 116 t   */, cAlphaLower /* 117 u   */, cAlphaLower  /* 118 v   */, cAlphaLower   /* 119 w   */,
    cAlphaLower /* 120 x   */, cAlphaLower /* 121 y   */, cAlphaLower  /* 122 z   */, cDelimiter    /* 123 {   */,
    cDelimiter  /* 124 |   */, cDelimiter  /* 125 }   */, cDelimiter   /* 126 ~   */, cInvalid      /* 127 DEL */,
    cInvalid    /* 128     */, cInvalid    /* 129     */, cInvalid     /* 130     */, cInvalid      /* 131     */,
    cInvalid    /* 132     */, cInvalid    /* 133     */, cInvalid     /* 134     */, cInvalid      /* 135     */,
    cInvalid    /* 136     */, cInvalid    /* 137     */, cInvalid     /* 138     */, cInvalid      /* 139     */,
    cInvalid    /* 140     */, cInvalid    /* 141     */, cInvalid     /* 142     */, cInvalid      /* 143     */,
    cInvalid    /* 144     */, cInvalid    /* 145     */, cInvalid     /* 146     */, cInvalid      /* 147     */,
    cInvalid    /* 148     */, cInvalid    /* 149     */, cInvalid     /* 150     */, cInvalid      /* 151     */,
    cInvalid    /* 152     */, cInvalid    /* 153     */, cInvalid     /* 154     */, cInvalid      /* 155     */,
    cInvalid    /* 156     */, cInvalid    /* 157     */, cInvalid     /* 158     */, cInvalid      /* 159     */,
    cInvalid    /* 160     */, cInvalid    /* 161     */, cInvalid     /* 162     */, cInvalid      /* 163     */,
    cInvalid    /* 164     */, cInvalid    /* 165     */, cInvalid     /* 166     */, cInvalid      /* 167     */,
    cInvalid    /* 168     */, cInvalid    /* 169     */, cInvalid     /* 170     */, cInvalid      /* 171     */,
    cInvalid    /* 172     */, cInvalid    /* 173     */, cInvalid     /* 174     */, cInvalid      /* 175     */,
    cInvalid    /* 176     */, cInvalid    /* 177     */, cInvalid     /* 178     */, cInvalid      /* 179     */,
    cInvalid    /* 180     */, cInvalid    /* 181     */, cInvalid     /* 182     */, cInvalid      /* 183     */,
    cInvalid    /* 184     */, cInvalid    /* 185     */, cInvalid     /* 186     */, cInvalid      /* 187     */,
    cInvalid    /* 188     */, cInvalid    /* 189     */, cInvalid     /* 190     */, cInvalid      /* 191     */,
    cInvalid    /* 192     */, cInvalid    /* 193     */, cInvalid     /* 194     */, cInvalid      /* 195     */,
    cInvalid    /* 196     */, cInvalid    /* 197     */, cInvalid     /* 198     */, cInvalid      /* 199     */,
    cInvalid    /* 200     */, cInvalid    /* 201     */, cInvalid     /* 202     */, cInvalid      /* 203     */,
    cInvalid    /* 204     */, cInvalid    /* 205     */, cInvalid     /* 206     */, cInvalid      /* 207     */,
    cInvalid    /* 208     */, cInvalid    /* 209     */, cInvalid     /* 210     */, cInvalid      /* 211     */,
    cInvalid    /* 212     */, cInvalid    /* 213     */, cInvalid     /* 214     */, cInvalid      /* 215     */,
    cInvalid    /* 216     */, cInvalid    /* 217     */, cInvalid     /* 218     */, cInvalid      /* 219     */,
    cInvalid    /* 220     */, cInvalid    /* 221     */, cInvalid     /* 222     */, cInvalid      /* 223     */,
    cInvalid    /* 224     */, cInvalid    /* 225     */, cInvalid     /* 226     */, cInvalid      /* 227     */,
    cInvalid    /* 228     */, cInvalid    /* 229     */, cInvalid     /* 230     */, cInvalid      /* 231     */,
    cInvalid    /* 232     */, cInvalid    /* 233     */, cInvalid     /* 234     */, cInvalid      /* 235     */,
    cInvalid    /* 236     */, cInvalid    /* 237     */, cInvalid     /* 238     */, cInvalid      /* 239     */,
    cInvalid    /* 240     */, cInvalid    /* 241     */, cInvalid     /* 242     */, cInvalid      /* 243     */,
    cInvalid    /* 244     */, cInvalid    /* 245     */, cInvalid     /* 246     */, cInvalid      /* 247     */,
    cInvalid    /* 248     */, cInvalid    /* 249     */, cInvalid     /* 250     */, cInvalid      /* 251     */,
    cInvalid    /* 252     */, cInvalid    /* 253     */, cInvalid     /* 254     */, cInvalid      /* 255     */,
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
    1, // sForwardSlash
    1, // sInBlockComment
    1, // sBlockAsterisk
    1, // sInlineComment
    0, // sNumber
    0, // sString       // 1?
    0, // sQuoteSymbol  // 1?
    0, // sSlashSymbol
    0, // sDelimiter
    0, // sIdentifier
    0, // sClassName
    0, // sOperator
    0, // sLexError
    0, // sEndCode
};

std::array<int8_t, 26> kKeywordStart = {
    0,  // a - "arg"
    -1, // b
    -1, // c
    -1, // d
    -1, // e
    1,  // f - "false"
    -1, // g
    -1, // h
    -1, // i
    -1, // j
    -1, // k
    -1, // l
    -1, // m
    2,  // n - "nil"
    -1, // o
    -1, // p
    -1, // q
    -1, // r
    -1, // s
    3,  // t - "true"
    -1, // u
    4,  // v - "var"
    -1, // w
    -1, // x
    -1, // y
    -1, // z
};

// Note deliberate elision of first character, which is captured by the kKeywordStart array.
std::array<const char*, 5> kKeywords = {
/* a */ "rg",
/* f */ "alse",
/* n */ "il",
/* t */ "rue",
/* v */ "ar"
};

std::array<hadron::Lexer::Token::Type, 5> kKeywordTokens = {
    hadron::Lexer::Token::Type::kArg,
    hadron::Lexer::Token::Type::kFalse,
    hadron::Lexer::Token::Type::kNil,
    hadron::Lexer::Token::Type::kTrue,
    hadron::Lexer::Token::Type::kVar
};

#ifdef DEBUG_LEXER
void saveStatesLeadingTo(State targetState, std::array<bool, kStateTransitionTable.size()>& visited,
        std::array<bool, kNumStates>& knownStates, const char* suffix, std::ofstream& outFile) {
    // Scan through state transition table looking for targetState, if hasn't been visited yet add to file and
    // recurse back until all entry states have been exhausted
    for (size_t i = 0; i < kStateTransitionTable.size(); ++i) {
        if (kStateTransitionTable[i] == targetState && !visited[i]) {
            State entryState = static_cast<State>(i % kNumStates);
            if (entryState >= kFirstFinalState) {
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

Lexer::Lexer(const char* code):
    m_code(code),
    m_state(sSpace),
    m_characterClass(static_cast<int>(CharacterClass::cSpace)) {
    SPDLOG_DEBUG("lex input string: '{}'", m_code);
}

bool Lexer::next() {
    if (m_state < sLexError) {
        int tokenLength = 0;
        do {
            int character = static_cast<int>(*m_code++);
            m_characterClass = static_cast<int>(kCharacterClasses[character]);
            SPDLOG_DEBUG("  character: '{}', class: {}, state: {}, length: {}", kCharacterNames[character & 0x7f],
                    kClassNames[m_characterClass / kNumStates], kStateNames[m_state], tokenLength);
            m_state = kStateTransitionTable[m_characterClass + m_state];
            tokenLength += kStateLengths[m_state];
        } while (m_state < kFirstFinalState);

        SPDLOG_DEBUG("final state: {}", kStateNames[m_state]);

        char* tokenEnd = nullptr;
        const char* tokenStart;
        switch(m_state) {
        case sNumber: {
            tokenStart = m_code - tokenLength - 1;
            std::regex radixPattern("^([0-9]+)([xr])");
            std::cmatch radixMatch;
            if (std::regex_search(tokenStart, radixMatch, radixPattern)) {
                if (*radixMatch[2].first == 'x') {
                    std::regex hexNumberPattern("^([0-9a-fA-F])");
                    std::cmatch hexMatch;
                    if (!std::regex_search(radixMatch[2].first + 1, hexMatch, hexNumberPattern)) {
                        m_state = sLexError;
                        return false;
                    }
                    int64_t value = std::strtoll(hexMatch[1].first, &tokenEnd, 16);
                    m_token = Token(tokenStart, tokenEnd - tokenStart, value);
                    m_code = tokenEnd;
                } else {
                    // Since r was the other allowable value in the regex it is assumed it is present.
                    //int radix = std::strtol(radixMatch[1].first, nullptr, 10);
                    //std::regex alphaNumberPattern("^([0-9a-zA-Z]+)|([0-9a-zA-Z]+[.][0-9a-zA-Z]+).*");
                    //std::cmatch alphaMatch;
                    //if (!std:regex_match(radixMatch[2].first + 1, alphaMatch, alphaNumberPattern)) {
                        m_state = sLexError;
                        return false;
                    //}
                }
            } else {
                std::regex numberPattern("^([0-9]+\\.[0-9]+)|([0-9]+)");
                std::cmatch numberMatch;
                if (std::regex_search(tokenStart, numberMatch, numberPattern)) {
                    if (numberMatch[1].matched) {
                        double value = std::strtod(tokenStart, &tokenEnd);
                        m_token = Token(tokenStart, tokenEnd - tokenStart, value);
                        m_code = tokenEnd;
                    } else {
                        int64_t value = std::strtoll(tokenStart, &tokenEnd, 10);
                        m_token = Token(tokenStart, tokenEnd - tokenStart, value);
                        m_code = tokenEnd;
                    }
                } else {
                    m_state = sLexError;
                    return false;
                }
            }
        } break;

        case sString: {
            tokenStart = m_code - tokenLength - 1;
            std::regex stringPattern("^\"(([^\"\\\\]|\\\\[^])*)\"");
            std::cmatch stringMatch;
            if (std::regex_search(tokenStart, stringMatch, stringPattern)) {
                m_token = Token(Token::Type::kString, stringMatch[1].first, stringMatch[1].str().size());
                m_code = tokenStart + stringMatch[1].str().size() + 2;
            } else {
                m_state = sLexError;
                return false;
            }
        } break;

        case sQuoteSymbol: {
            tokenStart = m_code - tokenLength - 1;
            std::regex symbolPattern("^'(([^'\\\\]|\\\\[^])*)'");
            std::cmatch symbolMatch;
            if (std::regex_search(tokenStart, symbolMatch, symbolPattern)) {
                m_token = Token(Token::Type::kSymbol, symbolMatch[1].first, symbolMatch[1].str().size());
                m_code = tokenStart + symbolMatch[1].str().size() + 2;
            } else {
                m_state = sLexError;
                return false;
            }
        } break;

        case sSlashSymbol: {
            tokenStart = m_code - tokenLength - 1;
            std::regex symbolPattern("^\\\\([a-zA-Z0-9_]*)");
            std::cmatch symbolMatch;
            if (std::regex_search(tokenStart, symbolMatch, symbolPattern)) {
                m_token = Token(Token::Type::kSymbol, symbolMatch[1].first, symbolMatch[1].str().size());
                m_code = tokenStart + symbolMatch[1].str().size() + 1;
            } else {
                m_state = sLexError;
                return false;
            }
        } break;

        case sDelimiter: {
            tokenStart = m_code - 1;
            switch (*tokenStart) {
            case '(':
                m_token = Token(Token::Type::kOpenParen, tokenStart, 1);
                break;

            case ')':
                m_token = Token(Token::Type::kCloseParen, tokenStart, 1);
                break;

            case '{':
                m_token = Token(Token::Type::kOpenCurly, tokenStart, 1);
                break;

            case '}':
                m_token = Token(Token::Type::kCloseCurly, tokenStart, 1);
                break;

            case '[':
                m_token = Token(Token::Type::kOpenSquare, tokenStart, 1);
                break;

            case ']':
                m_token = Token(Token::Type::kCloseSquare, tokenStart, 1);
                break;

            case ',':
                m_token = Token(Token::Type::kComma, tokenStart, 1);
                break;

            case ';':
                m_token = Token(Token::Type::kSemicolon, tokenStart, 1);
                break;

            case ':':
                m_token = Token(Token::Type::kColon, tokenStart, 1);
                break;

            case '^':
                m_token = Token(Token::Type::kCaret, tokenStart, 1);
                break;

            case '~':
                m_token = Token(Token::Type::kTilde, tokenStart, 1);
                break;

            default:
                SPDLOG_CRITICAL("got missing delimiter case");
                m_state = sLexError;
                break;
            }
            } break;

        case sIdentifier: {
            tokenStart = m_code - tokenLength - 1;
            std::regex identifierPattern("^([a-z][a-zA-Z0-9_]*)");
            std::cmatch identifierMatch;
            if (std::regex_search(tokenStart, identifierMatch, identifierPattern)) {
                tokenLength = identifierMatch[1].str().size();
                int8_t keywordIndex = kKeywordStart[*tokenStart - 'a'];
                if (keywordIndex >= 0 && tokenLength > 2 && tokenLength < 6 &&
                        strncmp(kKeywords[keywordIndex], tokenStart + 1, tokenLength - 1) == 0) {
                    m_token = Token(kKeywordTokens[keywordIndex], tokenStart, tokenLength);
                    SPDLOG_DEBUG("keyword, tokenStart: {}, length: {}", *tokenStart, tokenLength);
                } else {
                    m_token = Token(Token::Type::kIdentifier, tokenStart, tokenLength);
                    SPDLOG_DEBUG("identifier, tokenStart: {}, length: {}", *tokenStart, tokenLength);
                }
                m_code = tokenStart + tokenLength;
            } else {
                m_state = sLexError;
                return false;
            }
        } break;

        case sClassName: {
            tokenStart = m_code - tokenLength - 1;
            std::regex classPattern("^([A-Z][a-zA-Z0-9_]*)");
            std::cmatch classMatch;
            if (std::regex_search(tokenStart, classMatch, classPattern)) {
                m_token = Token(Token::Type::kClassName, classMatch[1].first, classMatch[1].str().size());
                m_code = tokenStart + classMatch[1].str().size();
            } else {
                m_state = sLexError;
                return false;
            }
        } break;

        case sOperator: {
            tokenStart = m_code - tokenLength - 1;
            std::regex operatorPattern("^([!@%&*-+=|<>?/]+)");
            std::cmatch operatorMatch;
            if (std::regex_search(tokenStart, operatorMatch, operatorPattern)) {
                if (operatorMatch[1].str().size() == 1) {
                    switch (*tokenStart) {
                    case '+':
                        m_token = Token(Token::Type::kPlus, tokenStart, 1);
                        break;

                    case '*':
                        m_token = Token(Token::Type::kAsterisk, tokenStart, 1);
                        break;

                    case '=':
                        m_token = Token(Token::Type::kAssign, tokenStart, 1);
                        break;

                    case '<':
                        m_token = Token(Token::Type::kLessThan, tokenStart, 1);
                        break;

                    case '>':
                        m_token = Token(Token::Type::kGreaterThan, tokenStart, 1);
                        break;

                    case '|':
                        m_token = Token(Token::Type::kPipe, tokenStart, 1);
                        break;

                    default:
                        m_token = Token(Token::Type::kBinop, tokenStart, 1);
                        break;
                    }
                } else if (operatorMatch[1].str().size() == 2) {
                    // Things we may care about: '<>', '<-'
                    if (*tokenStart == '<') {
                        if (tokenStart[1] == '>') {
                            m_token = Token(Token::Type::kReadWriteVar, tokenStart, 2);
                        } else if (tokenStart[1] == '-') {
                            m_token = Token(Token::Type::kLeftArrow, tokenStart, 2);
                        } else {
                            m_token = Token(Token::Type::kBinop, tokenStart, 2);
                        }
                    }
                } else {
                    m_token = Token(Token::Type::kBinop, tokenStart, operatorMatch[1].str().size());
                }

                m_code = tokenStart + operatorMatch[1].str().size();
            } else {
                m_state = sLexError;
                return false;
            }
        } break;

        case sDot: {
            tokenStart = m_code - tokenLength - 1;
            if (tokenStart[1] == '.') {
                if (tokenStart[2] == '.') {
                    if (tokenStart[3] == '.') {
                        m_state = sLexError;
                    } else {
                        m_token = Token(Token::Type::kEllipses, tokenStart, 3);
                        m_code = tokenStart + 3;
                    }
                } else {
                    m_token = Token(Token::Type::kDotDot, tokenStart, 2);
                    m_code = tokenStart + 2;
                }
            } else {
                m_token = Token(Token::Type::kDot, tokenStart, 1);
                m_code = tokenStart + 1;
            }
        } break;

        case sEndCode:
        case sLexError:
            m_token = Token(Token::Type::kEmpty, nullptr, 0);
            return false;

        default:
            m_state = sLexError;
            m_token = Token(Token::Type::kEmpty, nullptr, 0);
            return false;

        }
    } else {
        m_token = Token(Token::Type::kEmpty, nullptr, 0);
        return false;
    }

    return m_state < sLexError;
}

bool Lexer::isError() {
    return m_state == sLexError;
}

bool Lexer::isEOF() {
    return m_state == sEndCode;
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

    State finalState = static_cast<State>(kFirstFinalState);
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
