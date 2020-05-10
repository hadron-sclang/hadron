#include "Lexer.hpp"

#include "spdlog/spdlog.h"

#include <cstdlib>

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

    // final states
    sInteger = 6,
    sHexInteger = 7, // done, this can only be an integer
    sFloat = 8, // done, we have enough to delegate to strtod
    sRadix = 9,  // can have decimal but all bets on speed are off.
    sZero = 10,
    sAdd = 11,
    sStringCat = 12, // ++
    sPathCat = 13,   // +/+
    sSubtract = 14,
    sMultiply = 15,
    sExponentiate = 16,
    sDivide = 17,
    sModulo = 18,

    sLexError = 19,

    // This has to stay the last state.
    sEndCode = 20,
};

constexpr uint8_t kFirstFinalState = 6;
constexpr uint8_t kNumStates = State::sEndCode + 1;

enum CharacterClass : uint8_t {
    cSpace = 0 * kNumStates,   // space, tab
    cNewline = 1 * kNumStates, // \n, \r
    cZero = 2 * kNumStates,    // 0
    cDigit = 3 * kNumStates,   // 1-9
    cPeriod = 4 * kNumStates,  // .
    cx = 5 * kNumStates,       // a lower-case x, possibly for hexadecimal
    cPlus = 6 * kNumStates,    // +
    cHyphen = 7 * kNumStates,  //
    cInvalid = 8 * kNumStates, // unsupported character
    // This has to stay the last character class.
    cEnd = 9 * kNumStates,     // EOF, \0
};

const State kStateTransitionTable[] = {
    // CharacterClass = cSpace
    /* sSpace        => */ sSpace,
    /* sLeadZero     => */ sZero,
    /* sNumber       => */ sInteger,
    /* sPlus         => */ sAdd,
    /* sAsterisk     => */ sMultiply,
    /* sForwardSlash => */ sDivide,
    /* sInteger      => */ sSpace,
    /* sHexInteger   => */ sSpace,
    /* sFloat        => */ sSpace,
    /* sRadix        => */ sSpace,
    /* sZero         => */ sSpace,
    /* sAdd          => */ sSpace,
    /* sStringCat    => */ sSpace,
    /* sPathCat      => */ sSpace,
    /* sSubtract     => */ sSpace,
    /* sMultiply     => */ sSpace,
    /* sExponentiate => */ sSpace,
    /* sDivide       => */ sSpace,
    /* sModulo       => */ sSpace,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError,

    // Class = cNewline
    /* sSpace        => */ sSpace,
    /* sLeadZero     => */ sZero,
    /* sNumber       => */ sInteger,
    /* sPlus         => */ sAdd,
    /* sAsterisk     => */ sMultiply,
    /* sForwardSlash => */ sDivide,
    /* sInteger      => */ sSpace,
    /* sHexInteger   => */ sSpace,
    /* sFloat        => */ sSpace,
    /* sRadix        => */ sSpace,
    /* sZero         => */ sSpace,
    /* sAdd          => */ sSpace,
    /* sStringCat    => */ sSpace,
    /* sPathCat      => */ sSpace,
    /* sSubtract     => */ sSpace,
    /* sMultiply     => */ sSpace,
    /* sExponentiate => */ sSpace,
    /* sDivide       => */ sSpace,
    /* sModulo       => */ sSpace,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError,

    // Class = cZero
    /* sSpace        => */ sLeadZero,
    /* sLeadZero     => */ sLeadZero,  // many leading zeros treated as a single leading zero in sclang
    /* sNumber       => */ sNumber,
    /* sPlus         => */ sAdd,
    /* sAsterisk     => */ sLeadZero,
    /* sForwardSlash => */ sLeadZero,
    /* sInteger      => */ sLexError,
    /* sHexInteger   => */ sLexError,
    /* sFloat        => */ sLexError,
    /* sRadix        => */ sLexError,
    /* sZero         => */ sLexError,
    /* sAdd          => */ sLeadZero,
    /* sStringCat    => */ sLeadZero,
    /* sPathCat      => */ sLeadZero,
    /* sSubtract     => */ sLeadZero,
    /* sMultiply     => */ sLeadZero,
    /* sExponentiate => */ sLeadZero,
    /* sDivide       => */ sLeadZero,
    /* sModulo       => */ sLeadZero,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError,

    // Class = cDigit
    /* sSpace        => */ sNumber,
    /* sLeadZero     => */ sNumber,
    /* sNumber       => */ sNumber,
    /* sPlus         => */ sAdd,
    /* sAsterisk     => */ sMultiply,
    /* sForwardSlash => */ sDivide,
    /* sInteger      => */ sLexError,
    /* sHexInteger   => */ sLexError,
    /* sFloat        => */ sLexError,
    /* sRadix        => */ sLexError,
    /* sZero         => */ sLexError,
    /* sAdd          => */ sNumber,
    /* sStringCat    => */ sNumber,
    /* sPathCat      => */ sNumber,
    /* sSubtract     => */ sNumber,
    /* sMultiply     => */ sNumber,
    /* sExponentiate => */ sNumber,
    /* sDivide       => */ sNumber,
    /* sModulo       => */ sNumber,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError,

    // Class = cPeriod
    /* sSpace        => */ sLexError,
    /* sLeadZero     => */ sFloat,
    /* sNumber       => */ sFloat,
    /* sPlus         => */ sLexError,
    /* sAsterisk     => */ sLexError,
    /* sForwardSlash => */ sLexError,
    /* sInteger      => */ sLexError,
    /* sHexInteger   => */ sLexError,
    /* sFloat        => */ sLexError,
    /* sRadix        => */ sLexError,
    /* sZero         => */ sLexError,
    /* sAdd          => */ sLexError,
    /* sStringCat    => */ sLexError,
    /* sPathCat      => */ sLexError,
    /* sSubtract     => */ sLexError,
    /* sMultiply     => */ sLexError,
    /* sExponentiate => */ sLexError,
    /* sDivide       => */ sLexError,
    /* sModulo       => */ sLexError,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError,

    // Class = cx
    /* sSpace        => */ sLexError,
    /* sLeadZero     => */ sHexInteger,
    /* sNumber       => */ sLexError,
    /* sPlus         => */ sLexError,
    /* sAsterisk     => */ sLexError,
    /* sForwardSlash => */ sLexError,
    /* sInteger      => */ sLexError,
    /* sHexInteger   => */ sLexError,
    /* sFloat        => */ sLexError,
    /* sRadix        => */ sLexError,
    /* sZero         => */ sLexError,
    /* sAdd          => */ sLexError,
    /* sStringCat    => */ sLexError,
    /* sPathCat      => */ sLexError,
    /* sSubtract     => */ sLexError,
    /* sMultiply     => */ sLexError,
    /* sExponentiate => */ sLexError,
    /* sDivide       => */ sLexError,
    /* sModulo       => */ sLexError,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError,

    // Class = cPlus
    /* sSpace        => */ sPlus,
    /* sLeadZero     => */ sZero,
    /* sNumber       => */ sInteger,
    /* sPlus         => */ sStringCat,
    /* sAsterisk     => */ sLexError,
    /* sForwardSlash => */ sPathCat,
    /* sInteger      => */ sPlus,
    /* sHexInteger   => */ sPlus,
    /* sFloat        => */ sPlus,
    /* sRadix        => */ sPlus,
    /* sZero         => */ sPlus,
    /* sAdd          => */ sLexError,
    /* sStringCat    => */ sLexError,
    /* sPathCat      => */ sLexError,
    /* sSubtract     => */ sLexError,
    /* sMultiply     => */ sLexError,
    /* sExponentiate => */ sLexError,
    /* sDivide       => */ sLexError,
    /* sModulo       => */ sLexError,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError,

    // Class = cHyphen
    /* sSpace        => */ sSubtract,
    /* sLeadZero     => */ sZero,
    /* sNumber       => */ sInteger,
    /* sPlus         => */ sLexError,
    /* sAsterisk     => */ sLexError,
    /* sForwardSlash => */ sLexError,
    /* sInteger      => */ sSubtract,
    /* sHexInteger   => */ sSubtract,
    /* sFloat        => */ sSubtract,
    /* sRadix        => */ sSubtract,
    /* sZero         => */ sSubtract,
    /* sAdd          => */ sLexError,
    /* sStringCat    => */ sLexError,
    /* sPathCat      => */ sLexError,
    /* sSubtract     => */ sLexError,
    /* sMultiply     => */ sLexError,
    /* sExponentiate => */ sLexError,
    /* sDivide       => */ sLexError,
    /* sModulo       => */ sLexError,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError,

    // Class = cInvalid
    /* sSpace        => */ sEndCode,
    /* sLeadZero     => */ sLexError,
    /* sNumber       => */ sLexError,
    /* sPlus         => */ sLexError,
    /* sAsterisk     => */ sLexError,
    /* sForwardSlash => */ sLexError,
    /* sInteger      => */ sLexError,
    /* sHexInteger   => */ sLexError,
    /* sFloat        => */ sLexError,
    /* sRadix        => */ sLexError,
    /* sZero         => */ sLexError,
    /* sAdd          => */ sLexError,
    /* sStringCat    => */ sLexError,
    /* sPathCat      => */ sLexError,
    /* sSubtract     => */ sLexError,
    /* sMultiply     => */ sLexError,
    /* sExponentiate => */ sLexError,
    /* sDivide       => */ sLexError,
    /* sModulo       => */ sLexError,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError,

    // Class = cEnd
    /* sSpace        => */ sEndCode,
    /* sLeadZero     => */ sZero,
    /* sNumber       => */ sInteger,
    /* sPlus         => */ sAdd,
    /* sAsterisk     => */ sMultiply,
    /* sForwardSlash => */ sDivide,
    /* sInteger      => */ sEndCode,
    /* sHexInteger   => */ sEndCode,
    /* sFloat        => */ sEndCode,
    /* sRadix        => */ sEndCode,
    /* sZero         => */ sEndCode,
    /* sAdd          => */ sEndCode,
    /* sStringCat    => */ sEndCode,
    /* sPathCat      => */ sEndCode,
    /* sSubtract     => */ sEndCode,
    /* sMultiply     => */ sEndCode,
    /* sExponentiate => */ sEndCode,
    /* sDivide       => */ sEndCode,
    /* sModulo       => */ sEndCode,
    /* sLexError     => */ sLexError,
    /* sEndCode      => */ sLexError
};

// Ensure a full state table or subtle out-of-bounds access errors become a nightmare to debug.
static_assert(sizeof(kStateTransitionTable) == (CharacterClass::cEnd + kNumStates));

const CharacterClass kCharacterClasses[256] = {
    cEnd     /*   0 \0  */, cInvalid /*   1 SOH */, cInvalid /*   2 STX */, cInvalid /*   3 ETX */,
    cInvalid /*   4 EOT */, cEnd     /*   5 EOF */, cInvalid /*   6 ACK */, cInvalid /*   7 BEL */,
    cInvalid /*   8 BS  */, cSpace   /*   9 \t  */, cNewline /*  10 \n  */, cInvalid /*  11 VT  */,
    cInvalid /*  12 FF  */, cNewline /*  13 \r  */, cInvalid /*  14 SO  */, cInvalid /*  15 SI  */,
    cInvalid /*  16 DLE */, cInvalid /*  17 DC1 */, cInvalid /*  18 DC2 */, cInvalid /*  19 DC3 */,
    cInvalid /*  20 DC4 */, cInvalid /*  21 NAK */, cInvalid /*  22 SYN */, cInvalid /*  23 ETB */,
    cInvalid /*  24 CAN */, cInvalid /*  25 EM  */, cInvalid /*  26 SUB */, cInvalid /*  27 ESC */,
    cInvalid /*  28 FS  */, cInvalid /*  29 FS  */, cInvalid /*  30 RS  */, cInvalid /*  31 US  */,
    cSpace   /*  32 SPC */, cInvalid /*  33 !   */, cInvalid /*  34 "   */, cInvalid /*  35 #   */,
    cInvalid /*  36 $   */, cInvalid /*  37 %   */, cInvalid /*  38 &   */, cInvalid /*  39 '   */,
    cInvalid /*  40 (   */, cInvalid /*  41 )   */, cInvalid /*  42 *   */, cPlus    /*  43 +   */,
    cInvalid /*  44 ,   */, cHyphen  /*  45 -   */, cInvalid /*  46 .   */, cInvalid /*  47 /   */,
    cZero    /*  48 0   */, cDigit   /*  49 1   */, cDigit   /*  50 2   */, cDigit   /*  51 3   */,
    cDigit   /*  52 4   */, cDigit   /*  53 5   */, cDigit   /*  54 6   */, cDigit   /*  55 7   */,
    cDigit   /*  56 8   */, cDigit   /*  57 9   */, cInvalid /*  58 :   */, cInvalid /*  59 ;   */,
    cInvalid /*  60 <   */, cInvalid /*  61 =   */, cInvalid /*  62 >   */, cInvalid /*  63 ?   */,
    cInvalid /*  64 @   */, cInvalid /*  65 A   */, cInvalid /*  66 B   */, cInvalid /*  67 C   */,
    cInvalid /*  68 D   */, cInvalid /*  69 E   */, cInvalid /*  70 F   */, cInvalid /*  71 G   */,
    cInvalid /*  72 H   */, cInvalid /*  73 I   */, cInvalid /*  74 J   */, cInvalid /*  75 K   */,
    cInvalid /*  76 L   */, cInvalid /*  77 M   */, cInvalid /*  78 N   */, cInvalid /*  79 O   */,
    cInvalid /*  80 P   */, cInvalid /*  81 Q   */, cInvalid /*  82 R   */, cInvalid /*  83 S   */,
    cInvalid /*  84 T   */, cInvalid /*  85 U   */, cInvalid /*  86 V   */, cInvalid /*  87 W   */,
    cInvalid /*  88 X   */, cInvalid /*  89 Y   */, cInvalid /*  90 Z   */, cInvalid /*  91 [   */,
    cInvalid /*  92 \   */, cInvalid /*  93 ]   */, cInvalid /*  94 ^   */, cInvalid /*  95 _   */,
    cInvalid /*  96 `   */, cInvalid /*  97 a   */, cInvalid /*  98 b   */, cInvalid /*  99 c   */,
    cInvalid /* 100 d   */, cInvalid /* 101 e   */, cInvalid /* 102 f   */, cInvalid /* 103 g   */,
    cInvalid /* 104 h   */, cInvalid /* 105 i   */, cInvalid /* 106 j   */, cInvalid /* 107 k   */,
    cInvalid /* 108 l   */, cInvalid /* 109 m   */, cInvalid /* 110 n   */, cInvalid /* 111 o   */,
    cInvalid /* 112 p   */, cInvalid /* 113 q   */, cInvalid /* 114 r   */, cInvalid /* 115 s   */,
    cInvalid /* 116 t   */, cInvalid /* 117 u   */, cInvalid /* 118 v   */, cInvalid /* 119 w   */,
    cx       /* 120 x   */, cInvalid /* 121 y   */, cInvalid /* 122 z   */, cInvalid /* 123 {   */,
    cInvalid /* 124 |   */, cInvalid /* 125 }   */, cInvalid /* 126 ~   */, cInvalid /* 127 DEL */,
    cInvalid /* 128     */, cInvalid /* 129     */, cInvalid /* 130     */, cInvalid /* 131     */,
    cInvalid /* 132     */, cInvalid /* 133     */, cInvalid /* 134     */, cInvalid /* 135     */,
    cInvalid /* 136     */, cInvalid /* 137     */, cInvalid /* 138     */, cInvalid /* 139     */,
    cInvalid /* 140     */, cInvalid /* 141     */, cInvalid /* 142     */, cInvalid /* 143     */,
    cInvalid /* 144     */, cInvalid /* 145     */, cInvalid /* 146     */, cInvalid /* 147     */,
    cInvalid /* 148     */, cInvalid /* 149     */, cInvalid /* 150     */, cInvalid /* 151     */,
    cInvalid /* 152     */, cInvalid /* 153     */, cInvalid /* 154     */, cInvalid /* 155     */,
    cInvalid /* 156     */, cInvalid /* 157     */, cInvalid /* 158     */, cInvalid /* 159     */,
    cInvalid /* 160     */, cInvalid /* 161     */, cInvalid /* 162     */, cInvalid /* 163     */,
    cInvalid /* 164     */, cInvalid /* 165     */, cInvalid /* 166     */, cInvalid /* 167     */,
    cInvalid /* 168     */, cInvalid /* 169     */, cInvalid /* 170     */, cInvalid /* 171     */,
    cInvalid /* 172     */, cInvalid /* 173     */, cInvalid /* 174     */, cInvalid /* 175     */,
    cInvalid /* 176     */, cInvalid /* 177     */, cInvalid /* 178     */, cInvalid /* 179     */,
    cInvalid /* 180     */, cInvalid /* 181     */, cInvalid /* 182     */, cInvalid /* 183     */,
    cInvalid /* 184     */, cInvalid /* 185     */, cInvalid /* 186     */, cInvalid /* 187     */,
    cInvalid /* 188     */, cInvalid /* 189     */, cInvalid /* 190     */, cInvalid /* 191     */,
    cInvalid /* 192     */, cInvalid /* 193     */, cInvalid /* 194     */, cInvalid /* 195     */,
    cInvalid /* 196     */, cInvalid /* 197     */, cInvalid /* 198     */, cInvalid /* 199     */,
    cInvalid /* 200     */, cInvalid /* 201     */, cInvalid /* 202     */, cInvalid /* 203     */,
    cInvalid /* 204     */, cInvalid /* 205     */, cInvalid /* 206     */, cInvalid /* 207     */,
    cInvalid /* 208     */, cInvalid /* 209     */, cInvalid /* 210     */, cInvalid /* 211     */,
    cInvalid /* 212     */, cInvalid /* 213     */, cInvalid /* 214     */, cInvalid /* 215     */,
    cInvalid /* 216     */, cInvalid /* 217     */, cInvalid /* 218     */, cInvalid /* 219     */,
    cInvalid /* 220     */, cInvalid /* 221     */, cInvalid /* 222     */, cInvalid /* 223     */,
    cInvalid /* 224     */, cInvalid /* 225     */, cInvalid /* 226     */, cInvalid /* 227     */,
    cInvalid /* 228     */, cInvalid /* 229     */, cInvalid /* 230     */, cInvalid /* 231     */,
    cInvalid /* 232     */, cInvalid /* 233     */, cInvalid /* 234     */, cInvalid /* 235     */,
    cInvalid /* 236     */, cInvalid /* 237     */, cInvalid /* 238     */, cInvalid /* 239     */,
    cInvalid /* 240     */, cInvalid /* 241     */, cInvalid /* 242     */, cInvalid /* 243     */,
    cInvalid /* 244     */, cInvalid /* 245     */, cInvalid /* 246     */, cInvalid /* 247     */,
    cInvalid /* 248     */, cInvalid /* 249     */, cInvalid /* 250     */, cInvalid /* 251     */,
    cInvalid /* 252     */, cInvalid /* 253     */, cInvalid /* 254     */, cInvalid /* 255     */,
};

int8_t kStateLengths[] = {
    0, // sSpace
    1, // sLeadZero
    1, // sNumber
    1, // sPlus
    1, // sAsterisk
    1, // sForwardSlash
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
    0, // sLexError
    0, // sEndCode
};

static_assert(sizeof(kStateLengths) == kNumStates);

}

namespace hadron {

Lexer::Lexer(const char* code): m_code(code) {}

bool Lexer::lex() {
    const char* code = m_code;
    bool lexContinues = true;
    State state = sSpace;
    while (lexContinues) {
        int tokenLength = 0;
        do {
            int character = static_cast<int>(*code++);
            int characterClass = static_cast<int>(kCharacterClasses[character]);
            state = kStateTransitionTable[characterClass + state];
            tokenLength += kStateLengths[state];
            SPDLOG_DEBUG("lexer character: '{}', class: {}, state: {}, tokenLength: {}",
                static_cast<char>(character), characterClass, state, tokenLength);

        } while (state < kFirstFinalState);

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
                lexContinues = false;
                break;

            case sInteger: {
                // Exit state machine pointing just past the end of the integer.
                tokenStart = code - tokenLength - 1;
                int64_t intValue = std::strtoll(tokenStart, &tokenEnd, 10);
                if (tokenStart < tokenEnd) {
                    m_tokens.emplace_back(Token(tokenStart, tokenEnd - tokenStart, intValue));
                } else {
                    lexContinues = false;
                }
            } break;

            case sHexInteger: {
                // Exit state machine pointing just past the "0x", length can be > 2 because we support multiple leading
                // zeros, such as "0000x3" needs to lex to 3.
                tokenStart = code - tokenLength - 1;
                int64_t intValue = std::strtoll(code, &tokenEnd, 16);
                if (code < tokenEnd) {
                    m_tokens.emplace_back(Token(tokenStart, tokenEnd - tokenStart, intValue));
                    code = tokenEnd + 1;
                } else {
                    lexContinues = false;
                }
            } break;

            case sFloat:
            case sRadix:
                lexContinues = false;
                break;

            case sZero:
                tokenStart = code - tokenLength - 1;
                m_tokens.emplace_back(Token(tokenStart, tokenLength, 0LL));
                break;

            case sAdd:
                tokenStart = code - tokenLength - 1;
                m_tokens.emplace_back(Token(Token::Type::kAddition, tokenStart, tokenLength));
                break;

            case sStringCat:
            case sPathCat:
            case sSubtract:
            case sMultiply:
            case sExponentiate:
            case sDivide:
            case sModulo:
                lexContinues = false;
                break;

            case sLexError:
            case sEndCode:
                lexContinues = false;
                break;
        }
    }

    return state == sEndCode;
}

}
