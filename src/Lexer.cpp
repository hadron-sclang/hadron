#include "Lexer.hpp"

// Design following https://nothings.org/computer/lexing.html

namespace {

    // Point of state table is to minimize branching. So we only select final states when either we're in a corner
    // case that will require exceptional processing (radix numbers) or things are clear what we're parsing (floats)
enum State : uint8_t {
    // non-final states
    kInitialZero = 0,
    kNumber = 1,  // can still have radix or decimal TODO: can do flats/sharps as a separate token?

    kFirstFinalState = 2,

    // final states
    kInteger = 2,
    kHexInteger = 3, // done, this can only be an integer
    kFloat = 4, // done, we have enough to delegate to strtod
    kRadix = 5,  // can have decimal but all bets on speed are off.
    kLoneZero = 6,

    kLexError = 7,
};

// TODO: premultiply
enum Class : uint8_t {
    kWhitespace = 0,  // space, tab
    kNewline = 2,     // \n
    kReturn = 4,      // \r
    kZero = 6,       // 0
    kDigit = 8,      // 1-9
    kPeriod = 10,     // .
    kEndOfInput = 12  // EOF, \0
    kInvalid = 14,    // unsupported character
};

const State kStateTransitionTable[] = {
    // **** Class = kWhitespace
    /* kInitialZero => */ kLoneZero,
    /* kNumber      => */ kInteger,

    // Class = kNewline
    /* kInitialZero => */ kLoneZero,
    /* kNumber      => */ kInteger,

    // Class = kReturn
    /* kInitialZero => */ kLoneZero,
    /* kNumber      => */ kInteger,

    // Class = kZero
    /* kInitialZero => */ kInitialZero,  // many leading zeros treated as a single leading zero in sclang
    /* kNumber      => */ kNumber,

    // Class = kDigit
    /* kInitialZero => */ kNumber,
    /* kNumber      => */ kNumber,

    // Class = kPeriod
    /* kInitialZero => */ kFloat,
    /* kNumber      => */ kFloat,

    // Class = kEndOfInput
    /* kInitialZero => */ kLoneZero,
    /* kNumber      => */ kInteger,

    // Class = kInvalid
    /* kInitialZero => */ kLexError,
    /* kNumber      => */ kLexError,
};

const uint8_t kCharacterClasses[256] = {
    kEndOfInput /*   0 \0  */, kInvalid    /*   1 SOH */, kInvalid /*   2 STX */, kInvalid /*   3 ETX */,
    kInvalid    /*   4 EOT */, kEndOfInput /*   5 EOF */, kInvalid /*   6 ACK */, kInvalid /*   7 BEL */,
    kInvalid    /*   8 BS  */, kWhitespace /*   9 \t  */, kNewline /*  10 \n  */, kInvalid /*  11 VT  */,
    kInvalid    /*  12 FF  */, kReturn     /*  13 \r  */, kInvalid /*  14 SO  */, kInvalid /*  15 SI  */,
    kInvalid    /*  16 DLE */, kInvalid    /*  17 DC1 */, kInvalid /*  18 DC2 */, kInvalid /*  19 DC3 */,
    kInvalid    /*  20 DC4 */, kInvalid    /*  21 NAK */, kInvalid /*  22 SYN */, kInvalid /*  23 ETB */,
    kInvalid    /*  24 CAN */, kInvalid    /*  25 EM  */, kInvalid /*  26 SUB */, kInvalid /*  27 ESC */,
    kInvalid    /*  28 FS  */, kInvalid    /*  29 FS  */, kInvalid /*  30 RS  */, kInvalid /*  31 US  */,
    kWhitespace /*  32 SPC */, kInvalid    /*  33 !   */, kInvalid /*  34 "   */, kInvalid /*  35 #   */,
    kInvalid    /*  36 $   */, kInvalid    /*  37 %   */, kInvalid /*  38 &   */, kInvalid /*  39 '   */,
    kInvalid    /*  40 (   */, kInvalid    /*  41 )   */, kInvalid /*  42 *   */, kInvalid /*  43 +   */,
    kInvalid    /*  44 ,   */, kInvalid    /*  45 -   */, kInvalid /*  46 .   */, kInvalid /*  47 /   */,
    kZero       /*  48 0   */, kDigit      /*  49 1   */, kDigit   /*  50 2   */, kDigit   /*  51 3   */,
    kDigit      /*  52 4   */, kDigit      /*  53 5   */, kDigit   /*  54 6   */, kDigit   /*  55 7   */,
    kDigit      /*  56 8   */, kDigit      /*  57 9   */, kInvalid /*  58 :   */, kInvalid /*  59 ;   */,
    kInvalid    /*  60 <   */, kInvalid    /*  61 =   */, kInvalid /*  62 >   */, kInvalid /*  63 ?   */,
    kInvalid    /*  64 @   */, kInvalid    /*  65 A   */, kInvalid /*  66 B   */, kInvalid /*  67 C   */,
    kInvalid    /*  68 D   */, kInvalid    /*  69 E   */, kInvalid /*  70 F   */, kInvalid /*  71 G   */,
    kInvalid    /*  72 H   */, kInvalid    /*  73 I   */, kInvalid /*  74 J   */, kInvalid /*  75 K   */,
    kInvalid    /*  76 L   */, kInvalid    /*  77 M   */, kInvalid /*  78 N   */, kInvalid /*  79 O   */,
    kInvalid    /*  80 P   */, kInvalid    /*  81 Q   */, kInvalid /*  82 R   */, kInvalid /*  83 S   */,
    kInvalid    /*  84 T   */, kInvalid    /*  85 U   */, kInvalid /*  86 V   */, kInvalid /*  87 W   */,
    kInvalid    /*  88 X   */, kInvalid    /*  89 Y   */, kInvalid /*  90 Z   */, kInvalid /*  91 [   */,
    kInvalid    /*  92 \   */, kInvalid    /*  93 ]   */, kInvalid /*  94 ^   */, kInvalid /*  95 _   */,
    kInvalid    /*  96 `   */, kInvalid    /*  97 a   */, kInvalid /*  98 b   */, kInvalid /*  99 c   */,
    kInvalid    /* 100 d   */, kInvalid    /* 101 e   */, kInvalid /* 102 f   */, kInvalid /* 103 g   */,
    kInvalid    /* 104 h   */, kInvalid    /* 105 i   */, kInvalid /* 106 j   */, kInvalid /* 107 k   */,
    kInvalid    /* 108 l   */, kInvalid    /* 109 m   */, kInvalid /* 110 n   */, kInvalid /* 111 o   */,
    kInvalid    /* 112 p   */, kInvalid    /* 113 q   */, kInvalid /* 114 r   */, kInvalid /* 115 s   */,
    kInvalid    /* 116 t   */, kInvalid    /* 117 u   */, kInvalid /* 118 v   */, kInvalid /* 119 w   */,
    kInvalid    /* 120 x   */, kInvalid    /* 121 y   */, kInvalid /* 122 z   */, kInvalid /* 123 {   */,
    kInvalid    /* 124 |   */, kInvalid    /* 125 }   */, kInvalid /* 126 ~   */, kInvalid /* 127 DEL */,
    kInvalid    /* 128     */, kInvalid    /* 129     */, kInvalid /* 130     */, kInvalid /* 131     */,
    kInvalid    /* 132     */, kInvalid    /* 133     */, kInvalid /* 134     */, kInvalid /* 135     */,
    kInvalid    /* 136     */, kInvalid    /* 137     */, kInvalid /* 138     */, kInvalid /* 139     */,
    kInvalid    /* 140     */, kInvalid    /* 141     */, kInvalid /* 142     */, kInvalid /* 143     */,
    kInvalid    /* 144     */, kInvalid    /* 145     */, kInvalid /* 146     */, kInvalid /* 147     */,
    kInvalid    /* 148     */, kInvalid    /* 149     */, kInvalid /* 150     */, kInvalid /* 151     */,
    kInvalid    /* 152     */, kInvalid    /* 153     */, kInvalid /* 154     */, kInvalid /* 155     */,
    kInvalid    /* 156     */, kInvalid    /* 157     */, kInvalid /* 158     */, kInvalid /* 159     */,
    kInvalid    /* 160     */, kInvalid    /* 161     */, kInvalid /* 162     */, kInvalid /* 163     */,
    kInvalid    /* 164     */, kInvalid    /* 165     */, kInvalid /* 166     */, kInvalid /* 167     */,
    kInvalid    /* 168     */, kInvalid    /* 169     */, kInvalid /* 170     */, kInvalid /* 171     */,
    kInvalid    /* 172     */, kInvalid    /* 173     */, kInvalid /* 174     */, kInvalid /* 175     */,
    kInvalid    /* 176     */, kInvalid    /* 177     */, kInvalid /* 178     */, kInvalid /* 179     */,
    kInvalid    /* 180     */, kInvalid    /* 181     */, kInvalid /* 182     */, kInvalid /* 183     */,
    kInvalid    /* 184     */, kInvalid    /* 185     */, kInvalid /* 186     */, kInvalid /* 187     */,
    kInvalid    /* 188     */, kInvalid    /* 189     */, kInvalid /* 190     */, kInvalid /* 191     */,
    kInvalid    /* 192     */, kInvalid    /* 193     */, kInvalid /* 194     */, kInvalid /* 195     */,
    kInvalid    /* 196     */, kInvalid    /* 197     */, kInvalid /* 198     */, kInvalid /* 199     */,
    kInvalid    /* 200     */, kInvalid    /* 201     */, kInvalid /* 202     */, kInvalid /* 203     */,
    kInvalid    /* 204     */, kInvalid    /* 205     */, kInvalid /* 206     */, kInvalid /* 207     */,
    kInvalid    /* 208     */, kInvalid    /* 209     */, kInvalid /* 210     */, kInvalid /* 211     */,
    kInvalid    /* 212     */, kInvalid    /* 213     */, kInvalid /* 214     */, kInvalid /* 215     */,
    kInvalid    /* 216     */, kInvalid    /* 217     */, kInvalid /* 218     */, kInvalid /* 219     */,
    kInvalid    /* 220     */, kInvalid    /* 221     */, kInvalid /* 222     */, kInvalid /* 223     */,
    kInvalid    /* 224     */, kInvalid    /* 225     */, kInvalid /* 226     */, kInvalid /* 227     */,
    kInvalid    /* 228     */, kInvalid    /* 229     */, kInvalid /* 230     */, kInvalid /* 231     */,
    kInvalid    /* 232     */, kInvalid    /* 233     */, kInvalid /* 234     */, kInvalid /* 235     */,
    kInvalid    /* 236     */, kInvalid    /* 237     */, kInvalid /* 238     */, kInvalid /* 239     */,
    kInvalid    /* 240     */, kInvalid    /* 241     */, kInvalid /* 242     */, kInvalid /* 243     */,
    kInvalid    /* 244     */, kInvalid    /* 245     */, kInvalid /* 246     */, kInvalid /* 247     */,
    kInvalid    /* 248     */, kInvalid    /* 249     */, kInvalid /* 250     */, kInvalid /* 251     */,
    kInvalid    /* 252     */, kInvalid    /* 253     */, kInvalid /* 254     */, kInvalid /* 255     */,
};

}

namespace hadron {

Lexer::Lexer(std::string_view code): m_code(code) {}

bool Lexer::lex() {

/*
int token_len = 0;
   do {
      int ch     = *p_src++;
      int eq_cls = equivalence_class[ch];
      state      = transition[state][eq_cls];
      token_len += in_token[state];
   } while (state > LAST_FINAL_STATE);

   p_token_start = p_src - token_len;
   switch (token_for_state[state]) {
      ...
   }
*/

   return true;
}

}
