#ifndef INCLUDE_GUARD_LS_STRING
#define INCLUDE_GUARD_LS_STRING

#ifndef ls_string_allocator_
enum ls_string_allocator_ {};
#endif

#define LS_STRING_ASSERT(Expression) { if (!(Expression)) {*(int*)0 = 0;} }
#define LS_STRING_DEFAULT_SIZE 256

inline int ls_string_Max(int A, int B) { return A >= B ? A : B; }
int ls_string_Strlen(char *S) { int Len=0; LS_STRING_ASSERT(S); while (*S++) { ++Len; } return Len; }
void ls_string_Memcpy(void *To, void *From, int Size) {
    u8 *T = (u8 *)To;
    u8 *F = (u8 *)From;

    LS_STRING_ASSERT(T);
    LS_STRING_ASSERT(F);

    for (int i=0; i<Size; ++i) {
        *T++ = *F++;
    }
}

typedef char * ls_string_alloc(void *Data, u32 Size);
typedef char * ls_string_realloc(void *Data, void *String, u32 Size);

struct ls_string_allocator {
    void *Data;
    ls_string_alloc *Alloc;
    ls_string_realloc *Realloc;
};

struct ls_string {
    char *Data;
    u32 Size;

    ls_string(char *Data, u32 Size) { this->Data = Data; this->Size = Size; }
    ls_string(char *Data) { this->Data = Data; this->Size = ls_string_Strlen(Data); }
    ls_string() { this->Data = 0; this->Size = 0; }

    bool EqualTo(char *String, u32 N);
    bool EqualTo(char *String);
    bool EqualTo(ls_string String);
    bool StartsWith(char *String, u32 Len);
    bool StartsWith(char *String);
    bool operator==(char *String);
    bool operator==(ls_string String);
    bool operator!=(char *String);
    bool operator!=(ls_string String);
    char operator[](u32 i) { Assert(this->Size >= i); return this->Data[i]; }
    // operator char*() { return this->Data; }

    u32 Utf8Length();
    static u32 Utf8Size(utf8 *S, u32 Count);
    static u32 UnicodeCodepointToUtf8(u32 Codepoint, char *Result);

    static void Concat(ls_string A, ls_string B);
    static void Concat(ls_string A, char *B);
    static void Concat(char *A, ls_string B);
    static void Concat(char *A, char *B);

    static void Equal(ls_string A, ls_string B);
    static void Equal(char *A, char *B);
    static void Equal(char *A, ls_string B);
    static void Equal(ls_string A, char *B);

    static void ConvertSlashesToUnix(ls_string S);
    static ls_string ExpandRelativePath(ls_string Path, ls_string CurrentDir);
};

struct ls_mutable_string : ls_string {
    u32 Cap;

    virtual void FitSize(u32 Size);
    void AppendChar(char C);
    void AppendF(const char *Format, ...);
    void AppendStringN(ls_string From, u32 N);
    void AppendString(ls_string From);
    void AppendCStringN(char *String, u32 Size);
    void AppendCString(char *String);
    void Terminate();
};

template <int Capacity>
struct ls_static_string : ls_mutable_string {
    char *DebugData;
    char Storage[Capacity];

    ls_static_string() : ls_mutable_string() {
        this->Data = this->Storage;
        this->Cap = Capacity;
        this->Size = 0;
    }
    ls_static_string(ls_string String) {
        this->Cap = Capacity;
        Assert(String.Size <= Capacity);
        this->Data = this->Storage;
        ls_string_Memcpy(this->Data, String.Data, String.Size);
        this->Size = String.Size;

        this->Data[this->Size] = 0;
    }

    ls_static_string(char *String) {
        this->Data = this->Storage;
        this->Cap = Capacity;

        u32 Size = ls_string_Strlen(String);

        // Current capacity is not enough to store the 0-terminated string
        Assert(Size + 1 <= Capacity);

        ls_string_Memcpy(this->Data, String, Size);
        this->Size = Size;

        this->Data[this->Size] = 0;
    }

    ls_static_string(const ls_static_string &String) {
        this->Data = this->Storage;
        this->Cap = Capacity;
        ls_string_Memcpy(this->Storage, String.Storage, Capacity);
        this->Size = String.Size;
        this->Data[this->Size] = 0;
    }

    ls_static_string & operator=(const ls_static_string &String) {
        this->Data = this->Storage;
        this->Cap = Capacity;
        ls_string_Memcpy(this->Storage, String.Storage, Capacity);
        this->Size = String.Size;
        this->Data[this->Size] = 0;

        u8 *Vptr = *(u8 **)&String;
        *(u8 **)this = Vptr;

        return *this;
    }
    void operator=(char *String) {
        if (String) {
            this->Data = this->Storage;
            this->Cap = Capacity;

            u32 Size = ls_string_Strlen(String);
            Assert(Size + 1 <= Capacity);

            ls_string_Memcpy(this->Data, String, Size);
            this->Size = Size;

            this->Data[this->Size] = 0;
        } else {
            this->Size = 0;
        }
    }

    void FitSize(u32 Size);
};

struct ls_stringbuf: public ls_mutable_string {
    char *D; // debug data pointer, because unwrapping hierarchy in Visual Studio watch window is annoying
    static ls_string_allocator *AllocatorTable;

    ls_stringbuf(ls_string_allocator A) {
        this->Allocator = A;
        this->Data = 0;
        this->Cap = 0;
        this->Size = 0;
    }

    ls_stringbuf(ls_string_allocator_ A) {
        this->Allocator = this->AllocatorTable[A];
        this->Data = 0;
        this->Cap = 0;
        this->Size = 0;
    }

    ls_stringbuf() {
        this->Allocator = {};
        this->Data = 0;
        this->Cap = 0;
        this->Size = 0;
    }

    ls_stringbuf(ls_string String) {
        this->Data = 0;
        this->Cap = 0;
        this->Size = 0;
        this->FitSize(String.Size + 1);
        ls_string_Memcpy(this->Data, String.Data, String.Size);
        this->Size = String.Size;
        this->D = this->Data;
        this->Data[this->Size] = 0;
    }

    ls_stringbuf(char *String) {
        u32 Size = ls_string_Strlen(String);
        this->Data = 0;
        this->Cap = 0;
        this->Size = 0;
        this->FitSize(Size + 1);
        ls_string_Memcpy(this->Data,String, Size);
        this->Size = Size;
        this->D = this->Data;
        this->Data[this->Size] = 0;
    }

    ls_stringbuf & operator=(const ls_stringbuf &String) {
        this->Allocator = String.Allocator;
        this->Data = String.Data;
        this->D = String.Data;
        this->Cap = String.Cap;
        this->Size = String.Size;

        u8 *Vptr = *(u8 **)&String;
        *(u8 **)this = Vptr;

        return *this;
    }

    void operator=(char *String) {
        if (String) {
            u32 Size = ls_string_Strlen(String);
            this->FitSize(Size + 1);
            ls_string_Memcpy(this->Data, String, Size);
            this->Size = Size;
            this->Data[this->Size] = 0;
        } else {
            this->Data = 0;
            this->Size = 0;
        }
        this->D = this->Data;
    }

    void operator=(ls_string String) {
        this->FitSize(String.Size + 1);
        ls_string_Memcpy(this->Data, String.Data, String.Size);
        this->Size = String.Size;
        this->Data[this->Size] = 0;
        this->D = this->Data;
    }

    void FitSize(u32 Size);
    ls_string_allocator Allocator;
};

enum token_ {
    Token_Unknown,

    Token_OpenParen,
    Token_CloseParen,
    Token_Colon,
    Token_Semicolon,
    Token_Asterisk,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_LessThan,
    Token_GreaterThan,
    Token_Equals,
    Token_Minus,
    Token_Plus,
    Token_Dot,
    Token_Comma,
    Token_ForwardSlash,
    Token_BackSlash,
    Token_Percent,
    Token_Pound,
    Token_Tildae,
    Token_Circumflex,
    Token_Ampersand,
    Token_Asperand,
    Token_Dollar,
    Token_Pipe,
    Token_Exclamation,
    Token_At,
    Token_Underscore,

    Token_String,
    Token_Identifier,
    Token_Integer,
    Token_Real,

    Token_EndOfLine,
    Token_EndOfStream,
};

enum StringWhitespace_ {
    StringWhitespace_Nothing,
    StringWhitespace_NewLine = 1 << 1,
    StringWhitespace_NewWord = 1 << 2,
};

struct token {
    token_ Type;
    ls_string Text;

    u32 White;
    bool NewWord;
    bool NewLine;
    u32 NewLines;

    union {
        r32 Real;
        s32 Integer;
    };

    r32 GetReal() {
        assert(this->Type == Token_Real || this->Type == Token_Integer);
        return this->Type == Token_Integer ? (r32)this->Integer : this->Real;
    }
    s32 GetInteger() {
        assert(this->Type == Token_Real || this->Type == Token_Integer);
        return this->Type == Token_Real ? (s32)this->Real : this->Integer;
    }
};

struct ls_parser: ls_string {
    char *At;

    ls_parser() {
        this->Data = 0;
        this->At = 0;
        this->Size = 0;
    }
    ls_parser(char *Data, u32 Size) {
        this->Data = Data;
        this->At = Data;
        this->Size = Size;
    }

    ls_parser(char *Data) {
        this->Data = Data;
        this->At = Data;
        this->Size = ls_string_Strlen(Data);
    }

    ls_parser(ls_string String) {
        this->Data = String.Data;
        this->At = String.Data;
        this->Size = String.Size;
    }

    static r32 Power(u32 number, u32 power);
    static u32 CharToDigit(char C) { return (u32)(C) - 48; }
    static bool Whitespace(char c) { return ((c == ' ') || (c == '\r') || (c == '\t')); }
    static bool Digit(char c) { return (c >= '0' && c <= '9'); }
    static bool Alpha(char C) { return ((C >= 'a') && (C <= 'z')) || ((C >= 'A') && (C <= 'Z')); }
    static bool Hex(char C) { return (C >= '0' && C <= '9') || (C >= 'a' && C <= 'f') || (C >= 'A' && C <= 'F'); }
    static char Lowercase(char C);
    static u32 HexStringToU32(char *String);
    static b32 IsControlSymbol(unsigned char C);
    static r32 TokenToReal32(token Token);
    static s32 TokenToInt32(token Token);


    char **Split(char *Delimiters, u32 *LineCount);

    b32 EqualTo(char *String, u32 Len);
    b32 EqualTo(char *String);
    b32 StartsWith(char *String, u32 Len);

    u32 Pos() { return (this->At - this->Data); };
    u32 RemainingBytes() { return (u32)ls_string_Max((s32)this->Size - (s32)(this->At - this->Data), (s32)0); };
    u32 SafeToSubtract() { return (this->At > this->Data); };
    u32 Location() { return ((u32)(this->At - this->Data)); };

    bool GetLine(ls_parser *Result);

    u32 TrimLeft();
    u32 TrimRight();

    token GetToken();
    token PeekToken();
    bool RequireToken(token_ Type);

    bool RequireChar(char C);
    bool MaybeEatChar(char C);
    bool MaybeEatIdentifier(char *String);

    u32 FFToString(char *String, u32 N);
    u32 FFToString(char *String);
    u32 FFToStringEnd(char *String, u32 N);
    u32 FFToStringEnd(char *String);

    bool FFToChar(char C);
    bool FFToAfterChar(char C);
    void FFToToken(token_ Type);
    void FFToTokenEnd(token_ Type);
    bool MaybeEatToken(token_ Type);

    // Filesystem paths
    u32 ReadDotDotSlashes();

    // UTF8
    u32 ReadUtf8Codepoint();
    u32 ReadUtf8CodepointBackwards();
    static u32 ReadUtf8Codepoint(utf8 *String, u32 *Size);

    void SkipUtf8CharsN(u32 N);
};

#ifdef LS_STRING_IMPLEMENTATION

bool ls_string::
EqualTo(char *String, u32 Len)
{
    bool Result = true;

    if (!String && !this->Data) {
        return true;
    } else if (!String || !this->Data) {
        return false;
    } else if (this->Size != Len) {
        return false;
    } else if (!*String && (this->Size == 0)) {
        // empty string are equal
        return true;
    }

    for (int i=0; i<Len; ++i) {
        if (this->Data[i] != String[i]) {
            Result = false;
            break;
        }
    }

    return Result;
}

bool ls_string::
operator==(char *String)
{
    if (!String) {
        LS_STRING_ASSERT(!"Trying to compare to a null string");
    }
    return EqualTo(String, ls_string_Strlen(String));
}

bool ls_string::
operator==(ls_string String)
{
    return EqualTo(String.Data, String.Size);
}

bool ls_string::
operator!=(char *String)
{
    if (!String) {
        LS_STRING_ASSERT(!"Trying to compare to a null string");
    }
    return !EqualTo(String, ls_string_Strlen(String));
}

bool ls_string::
operator!=(ls_string String)
{
    return !EqualTo(String.Data, String.Size);
}

bool ls_string::
StartsWith(char *String, u32 Len)
{
    bool Result = true;

    if (!String || !this->Data) {
        LS_STRING_ASSERT(!"Trying to compare invalid strings");
    } else if (this->Size < Len) {
        return false;
    } else if (!*String && (this->Size == 0) && Len == 0) {
        // empty string are equal
        return true;
    }

    for (int i=0; i<Len; ++i) {
        if (this->Data[i] != String[i]) {
            Result = false;
            break;
        }
    }

    return Result;
}

u32 ls_string::
Utf8Size(utf8 *S, u32 Count)
{
    u32 Result = 0;
    while (Count) {
        u32 Size = 0;
        ls_parser::ReadUtf8Codepoint(S, &Size);
        Result += Size;
        --Count;
    }

    return Result;
};

u32 ls_string::
Utf8Length()
{
    u32 Result = 0;
    ls_parser String = *this;

    while (String.RemainingBytes()) {
        String.SkipUtf8CharsN(1);
        ++Result;
    }

    return Result;
}

u32 ls_string::
UnicodeCodepointToUtf8(u32 Codepoint, char *Result)
{
    u32 ByteCount = 0;

    if (Codepoint <= 0x007f) {
        Result[0] = Codepoint;
        ByteCount = 1;
    } else if (Codepoint >= 0x0080 && Codepoint <= 0x07ff) {
        u32 Byte1 = 0b10000000 | (Codepoint & 0b00111111);
        u32 Byte0 = 0b11000000 | ((Codepoint >> 6) & 0b00011111);

        Result[0] = Byte0;
        Result[1] = Byte1;
        ByteCount = 2;
    } else if (Codepoint >= 0x0800 && Codepoint <= 0xffff) {
        u32 Byte2 = 0b10000000 | (Codepoint & 0b00111111);
        u32 Byte1 = 0b10000000 | ((Codepoint >> 6) & 0b00111111);
        u32 Byte0 = 0b11100000 | ((Codepoint >> 12) & 0b00001111);

        Result[0] = Byte0;
        Result[1] = Byte1;
        Result[2] = Byte2;
        ByteCount = 3;
    } else if (Codepoint >= 0x0800 && Codepoint <= 0xffff) {
        u32 Byte3 = 0b10000000 | (Codepoint & 0b00111111);
        u32 Byte2 = 0b10000000 | ((Codepoint >> 6) & 0b00111111);
        u32 Byte1 = 0b10000000 | ((Codepoint >> 12) & 0b00111111);
        u32 Byte0 = 0b11110000 | ((Codepoint >> 18) & 0b00000111);

        Result[0] = Byte0;
        Result[1] = Byte1;
        Result[2] = Byte2;
        Result[3] = Byte3;
        ByteCount = 4;
    }

    return ByteCount;
}

/*
▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

MUTABLE STRING */

void ls_mutable_string::
FitSize(u32 Size)
{
    LS_STRING_ASSERT(this->Size + Size < this->Cap);
}
void ls_mutable_string::
Terminate()
{
    if (this->Size) {
        this->Data[this->Size] = 0;
    }
}

void ls_mutable_string::
AppendChar(char C)
{
    this->FitSize(1+1);
    *(this->Data + this->Size++) = C;
    this->Data[this->Size] = 0;
}

void ls_mutable_string::
AppendF(const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    int len = vsnprintf(0, 0, Format, Args);
    va_end(Args);

    this->FitSize(len + 1);

    u32 AvailableBytes = this->Cap - this->Size;
    char *Buffer = (char *)(this->Data + this->Size);

    va_start(Args, Format);
    this->Size += vsnprintf(Buffer, AvailableBytes, Format, Args);
    this->Data[this->Size] = 0;
    va_end(Args);
}

void ls_mutable_string::
AppendStringN(ls_string From, u32 N)
{
    FitSize(N + 1);

    char *ToChar = this->Data + this->Size;
    char *FromChar = From.Data;
    for (u32 i=0; i<N; ++i) {
        *ToChar++ = *FromChar++;
        ++this->Size;
    }
    this->Data[this->Size] = 0;
}

void ls_mutable_string::
AppendString(ls_string From)
{
    FitSize(From.Size + 1);

    char *ToChar = this->Data + this->Size;
    char *FromChar = From.Data;
    for (u32 i=0; i<From.Size; ++i) {
        *ToChar++ = *FromChar++;
        ++this->Size;
    }
    this->Data[this->Size] = 0;
}

void ls_mutable_string::
AppendCStringN(char *String, u32 N)
{
    FitSize(N + 1);

    char *ToChar = this->Data + this->Size;
    char *FromChar = String;
    for (u32 i=0; i<N; ++i) {
        *ToChar++ = *FromChar++;
        ++this->Size;
    }
    this->Data[this->Size] = 0;
}

void ls_mutable_string::
AppendCString(char *String)
{
    AppendCStringN(String, ls_string_Strlen(String));
}

/*
▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

STATIC MUTABLE STRING */

template <int Capacity>
void ls_static_string<Capacity>::
FitSize(u32 Size)
{
    Assert(this->Size + Size <= Capacity);
}


/*
▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

GROWABLE MUTABLE STRING BUFFER */

void ls_stringbuf::
FitSize(u32 Size)
{
    /*  note: I always add 1 to the actual size to account for null-termination of
              vsnprintf-style stuff, that has no option of not writing the 0. */
    Size += 1;

    int NewCap = this->Cap;

#ifdef LS_STRING_USE_DEFAULT_ALLOCATOR
    if (!this->Allocator.Alloc || !this->Allocator.Realloc) {
        this->Allocator.Alloc = LsStringDefaultAlloc;
        this->Allocator.Realloc = LsStringDefaultRealloc;
        this->Allocator.Data = &LsStringDefaultAllocator;
    }
#endif

    if (this->Cap == 0) {
        NewCap = ls_string_Max(LS_STRING_DEFAULT_SIZE, Size);

        if (this->Allocator.Alloc) {
            this->Data = (char *)this->Allocator.Alloc(this->Allocator.Data, NewCap);
        } else {
            this->Data = (char *)malloc(NewCap);
        }

        this->Cap = NewCap;
    } else if (this->Cap < this->Size + Size) {
        NewCap = ls_string_Max(this->Size * 2, this->Size + Size);

        if (this->Allocator.Realloc) {
            this->Data = (char *)this->Allocator.Realloc(this->Allocator.Data, this, NewCap);
        } else {
            this->Data = (char *)realloc(this->Data, NewCap);
        }

        this->Cap = NewCap;
    }
}

/*
▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

PARSER */

// char ** ls_parser::
// SplitLines(u32 *out_LineCount)
// {
//     u32 LineCount;

//     while (this->RemainingBytes()) {
//         char C = this->At;
//         if (C == '\n') {
//             ++LineCount;
//         }
//     }

//     ls_string *Lines = (ls_string *)malloc(sizeof(ls_string) * LineCount);
//     return Lines;

//     ls_string Line;
//     this->At = this->Data;

//     u32 Index = 0;
//     while (this->GetLine(&Line)) {
//         Lines[Index] = Line;
//     }

//     if (out_LineCount) {
//         *out_LineCount = LineCount;
//     }

//     return Lines;
// }

b32
ls_parser::EqualTo(char *String, u32 Len)
{
    b32 Result = true;

    if (!String && !this->Data) {
        LS_STRING_ASSERT(!"Trying to compare invalid strings");
    } else if (this->RemainingBytes() != Len) {
        return false;
    } else if (!*String && (this->Size == 0)) {
        // empty string are equal
        return true;
    }

    for (int i=0; i<Len; ++i) {
        if (this->At[i] != String[i]) {
            Result = false;
            break;
        }
    }

    return Result;
}

b32
ls_parser::StartsWith(char *String, u32 Len)
{
    b32 Result = true;

    if (!String && !this->Data) {
        LS_STRING_ASSERT(!"Trying to compare invalid strings");
    } else if (this->RemainingBytes() < Len) {
        return false;
    } else if (!*String && (this->Size == 0)) {
        // empty string are equal
        return true;
    }

    for (int i=0; i<Len; ++i) {
        if (this->At[i] != String[i]) {
            Result = false;
            break;
        }
    }

    return Result;
}

r32
ls_parser::Power(u32 Number, u32 Power)
{
    u32 Result = 1;

    while (Power > 0) {
        Result *= Number;
        --Power;
    }

    return Result;
}

char
ls_parser::Lowercase(char C)
{
    char Result = C;
    if ((C >= 65) && (C <= 90)) {
        Result = C + 32;
    }
    return Result;
}


bool
ls_parser::RequireChar(char C)
{
    bool Result = false;

    Result = (this->At[0] == C);

    if (Result) {
        ++this->At;
    }

    return Result;
}

bool
ls_parser::MaybeEatChar(char C)
{
    b32 Result = false;
    if (this->At[0] == C) { ++this->At; Result = true; }
    return Result;
}

r32
ls_parser::TokenToReal32(token Token)
{
    float Result = 0;

    int Sign = 1.0f;

    char *At = Token.Text.Data;

    if (*At == '-') {
        Sign = -1.0f;
        ++At;
    }

    u32 Numbers[10];
    u32 NumberCounter = 0;
    int IntegerCount = 0;

    b32 FoundADot = false;

    while (At < (Token.Text.Data + Token.Text.Size)) {
        if (ls_parser::Digit(*At)) {
            Numbers[NumberCounter++] = ls_parser::CharToDigit(*At);
        } else if (*At == '.' && !FoundADot) {
            FoundADot = true;
            IntegerCount = NumberCounter;
        } else {
            // Assert(!"Numbers should be prepared");
        }
        ++At;
    }

    float Multiplier = 0.1f;
    if (IntegerCount) {
        Multiplier = (float)ls_parser::Power(10, IntegerCount - 1);
    }

    for (u32 i=0; i < NumberCounter; ++i) {
        Result += Multiplier * Numbers[i];
        Multiplier /= 10.0f;
    }

    Result *= Sign;

    return Result;
}

s32
ls_parser::TokenToInt32(token Token)
{
    s32 Result = 0;

    int Sign = 1;

    char *At = Token.Text.Data;

    if (*At == '-') {
        Sign = -1;
        ++At;
    }

    u32 Numbers[10];
    u32 NumberCounter = 0;

   while (At < (Token.Text.Data + Token.Text.Size)) {
        if (ls_parser::Digit(*At)) {
            Numbers[NumberCounter++] = ls_parser::CharToDigit(*At);
        } else {
            // Assert(!"Numbers should be prepared");
        }
        ++At;
    }

    int Multiplier = 1;
    for (s32 i = NumberCounter - 1; i >= 0; --i) {
        Result += Multiplier * Numbers[i];
        Multiplier *= 10;
    }

    Result *= Sign;

    return Result;
}

token
ls_parser::GetToken()
{
    token Token = {};

    u32 White = this->TrimLeft();
    Token.NewWord = White & StringWhitespace_NewWord;
    Token.NewLine = White & StringWhitespace_NewLine;
    Token.Text.Data = this->At;

    char *C = this->At;
    u32 RemainingSize = this->RemainingBytes();

    switch(*C) {
        case '\0': {Token.Type = Token_EndOfStream;} break;
        case '(':  {Token.Type = Token_OpenParen;} break;
        case ')':  {Token.Type = Token_CloseParen;} break;
        case ':':  {Token.Type = Token_Colon;} break;
        case ';':  {Token.Type = Token_Semicolon;} break;
        case '*':  {Token.Type = Token_Asterisk;} break;
        case '[':  {Token.Type = Token_OpenBracket;} break;
        case ']':  {Token.Type = Token_CloseBracket;} break;
        case '{':  {Token.Type = Token_OpenBrace;} break;
        case '}':  {Token.Type = Token_CloseBrace;} break;
        case '<':  {Token.Type = Token_LessThan;} break;
        case '>':  {Token.Type = Token_GreaterThan;} break;
        case '=':  {Token.Type = Token_Equals;} break;
        case '+':  {Token.Type = Token_Plus;} break;
        case ',':  {Token.Type = Token_Comma;} break;
        case '/':  {Token.Type = Token_ForwardSlash;} break;
        case '\\': {Token.Type = Token_BackSlash;} break;
        case '#':  {Token.Type = Token_Pound;} break;
        case '%':  {Token.Type = Token_Percent;} break;
        case '|':  {Token.Type = Token_Pipe;} break;
        case '^':  {Token.Type = Token_Circumflex;} break;
        case '$':  {Token.Type = Token_Dollar;} break;
        case '~':  {Token.Type = Token_Tildae;} break;
        case '!':  {Token.Type = Token_Exclamation;} break;
        case '@':  {Token.Type = Token_At;} break;
        case '&':  {Token.Type = Token_Ampersand;} break;
        case '_':  {Token.Type = Token_Underscore;} break;
        case '"':  {
            Token.Type = Token_String;
            Token.Text.Data = C+1;

            do {
                ++C;
                --RemainingSize;
            } while (RemainingSize && *C && *C != '"');

            Token.Text.Size = C - Token.Text.Data;

            if (*C == '"') {
                ++C;
            }
        } break;

        default : {
            if (*C == '.') {
                if (RemainingSize == 0 || !ls_parser::Digit(C[1])) {
                    Token.Type = Token_Dot;
                } else {
                    Token.Type = Token_Real;
                    ++C;
                    goto parse_number;
                }
            } else if (*C == '-') {
                if (RemainingSize == 0 || C[1] == ' ') {
                    Token.Type = Token_Minus;
                } else if (ls_parser::Digit(C[1]) || C[1] == '.') {
                    Token.Type = Token_Integer;
                    ++C;
                    goto parse_number;
                }
            } else if (ls_parser::Alpha(*C)){
                Token.Type = Token_Identifier;
                while (RemainingSize &&
                       (ls_parser::Alpha(*C) ||
                        ls_parser::Digit(*C) ||
                        *C == '_' ||
                        *C == '-'))
                {
                    ++C;
                    --RemainingSize;
                }

                Token.Text.Size = C - this->At;
            } else if (ls_parser::Digit(*C)) {
                Token.Type = Token_Integer;
                goto parse_number;
            } else {
                Token.Type = Token_Unknown;
            }

            parse_number:
                while (RemainingSize && (ls_parser::Digit(*C) || (*C == '.' && Token.Type != Token_Real))) {
                    if (*C == '.') {
                        Token.Type = Token_Real;
                    }
                    ++C;
                    --RemainingSize;
                }
                Token.Text.Size = C - this->At;
        } break;
    }

    if (Token.Type == Token_Integer) {
        Token.Integer = TokenToInt32(Token);
    } else if (Token.Type == Token_Real) {
        Token.Real = TokenToReal32(Token);
    }

    if (Token.Type >= Token_Unknown && Token.Type < Token_String) {
        Token.Text.Size = 1;
        ++C;
    }

    this->At = C;

    return Token;
}

token
ls_parser::PeekToken() {
    char *Before = this->At;
    token Token = this->GetToken();
    this->At = Before;
    return Token;
}

bool
ls_parser::RequireToken(token_ Type)
{
    char *At = this->At;
    token Token = this->GetToken();
    b32 Result = (Token.Type == Type);

    if (!Result) {
        this->At = At;
    }

    return Result;
}

void
ls_parser::FFToTokenEnd(token_ Type)
{
    token Token;
    do {
        Token = this->GetToken();
    } while (Token.Type != Type && Token.Type != Token_EndOfStream);
}

bool
ls_parser::MaybeEatToken(token_ Type)
{
    token Token = this->PeekToken();
    if (Token.Type == Type) {
        this->At += Token.Text.Size;
        return true;
    }
    return false;
}

bool
ls_parser::MaybeEatIdentifier(char *String)
{
    token Token = this->PeekToken();
    if (Token.Type == Token_Identifier && Token.Text == String) {
        this->GetToken();
        return true;
    }
    return false;
}

u32 ls_parser::
HexStringToU32(char *String)
{
    u32 Result = 0;

    if ((String[0] == '0') && (String[1] == 'x')) {
        String += 2;
    } else if (String[0] == '#') {
        String += 1;
    }

    u32 CurrentDigit = 0;
    u32 CharIndex = 0;

    char C;
    while ((C = *String++)) {
        if (ls_parser::Digit(C)) {
            CurrentDigit = (u32)(C) - 48;
        } else {
            // Assert(ls_parser::Hex(C));

            if (C >= 'a' && C <= 'f') {
                CurrentDigit = (u32)(C) - 97 + 10;
            } else if (C >= 'A' && C <= 'F') {
                CurrentDigit = (u32)(C) - 65 + 10;
            }
        }

        Result +=  CurrentDigit << (7 * 4 - (CharIndex * 4));

        ++CharIndex;
    }

    Result += 0x000000ff;

    return(Result);
}

inline b32 ls_parser::
IsControlSymbol(unsigned char C)
{
    b32 Result = false;
    if (C <= 0x1f || ((C >= 0x7f) && (C <= 0xa0))) {
        Result = true;
    }
    return(Result);
}

u32 ls_parser::
FFToString(char *String, u32 N)
{
    u32 Counter = 0;

    if (this->RemainingBytes() < N) {
        // note: ff to end of data
        this->At = this->Data + this->Size;
    } else {
        while (this->RemainingBytes() >= N) {
            if (this->EqualTo(String, N)) {
                return true;
            }

            ++this->At;
            ++Counter;
        }
    }

    return Counter;
}

u32 ls_parser::
FFToString(char *String)
{
    return FFToString(String, ls_string_Strlen(String));
}

u32 ls_parser::
FFToStringEnd(char *String, u32 N)
{
    u32 Counter = 0;

    if (this->RemainingBytes() < N) {
        // note: ff to end of data
        this->At = this->Data + this->Size;
    } else {
        while (this->RemainingBytes()) {
            LS_STRING_ASSERT(!"DO IT! JUST. DO. IT!!!!!!");
            // if (this->MatchSequence(String, N) ) {
            //     this->At += N;
            //     Counter += N;
            //     break;
            // }

            ++this->At;
            ++Counter;
        }
    }

    return Counter;
}

u32 ls_parser::
FFToStringEnd(char *String)
{
    return FFToStringEnd(String, ls_string_Strlen(String));
}

bool ls_parser::
GetLine(ls_parser *Result)
{
    ls_parser line = *this;
    line.Size = 0;
    line.Data = this->At;
    line.At = this->At;

    b32 Found = false;

    while (this->RemainingBytes()) {
        if (*this->At == '\n') {
            Found = true;
            ++this->At;
            break;
        }

        ++this->At;
        ++line.Size;
    }

    if (Result) {
        *Result = line;
    }

    return Found || (this->RemainingBytes() > 0);
}

u32 ls_parser::
TrimLeft()
{
    u32 Result = 0;

    while (this->RemainingBytes()) {
        char C = this->At[0];

        if ((C == ' ') || (C == '\t') || (C == '\r')) {
            Result |= StringWhitespace_NewWord;
        } else if (C == '\n') {
            Result |= StringWhitespace_NewLine;
        } else {
            break;
        }

        ++this->At;
    }

    return Result;
}

u32 ls_parser::
TrimRight()
{
    u32 Result = 0;

    char *C = this->Data + this->Size - 1;

    while (C >= this->Data) {
        if (Whitespace(*C)) {
            --C;
        } else {
            break;
        }
    }

    this->Size = C - this->Data + 1;

    return Result;
}


void ls_parser::
SkipUtf8CharsN(u32 N)
{
   while (N && this->RemainingBytes()) {
        u32 Codepoint = this->ReadUtf8Codepoint();
        --N;
    }
}
// TODO:

// inline u32
// ls_parser::CharsUntilStringEnd(ls_parser String, char *Sequence, u32 SequenceLength)
// {
//     u32 Count = 0;

//     while (this->RemainingBytes()) {
//         if (MatchSequence(Sequence, SequenceLength) ) {
//             Count += SequenceLength;
//             break;
//         }

//         ++Count;
//         ++this->At;
//     }

//     return Count;
// }

// u32
// ls_parser::CharsUntilChar(char C) {
//     u32 Count = 0;

//     while (this->RemainingBytes()) {
//         if (*this->At == C) { break; }
//         ++Count;
//         ++this->At;
//     }

//     return Count;
// }

// inline bool
// ls_parser::StringEqualN(char *String, u32 N)
// {
//     bool Result = true;
//     while (this->RemainingBytes() && N) {
//         if (*this->At != *String) {
//             Result = false;
//             break;
//         }
//         --N;
//         ++this->At;
//         ++String;
//     }
//     return Result;
// }

bool
ls_parser::FFToChar(char C)
{
    while (this->RemainingBytes()) {
        if (*this->At == C) {
            return true;
        }
        ++this->At;
    }
    return false;
}

bool
ls_parser::FFToAfterChar(char C)
{
    while (this->RemainingBytes()) {
        if (*this->At == C && this->RemainingBytes() > 1) {
            ++this->At;
           return true;
        }
        ++this->At;
    }

    return false;
}

// inline bool
// ls_parser::EqualsTo(char *String)
// {
//     return EqualsToString(String, ls_string_Strlen(String));
// }

void ls_string::
ConvertSlashesToUnix(ls_string String)
{
    ls_parser S = String;

    for (u32 i=0; i<S.Size; ++i) {
        char *PossibleSlash = (char *)S.At;
        u32 Codepoint = *PossibleSlash;

        // u32 Codepoint = ReadUTF8Codepoint(&String);
        if ((u8)Codepoint == '\\') {
            *PossibleSlash = '/';
        }

        ++S.At;
    }
}

u32 ls_parser::
ReadDotDotSlashes()
{
    u32 Count = 0;

    while (this->RemainingBytes()) {
        if (this->StartsWith("../", 3)) {
            ++Count;
            this->At += 3;
            continue;
        }
        break;
    }

    return Count;
}

#if 0
ls_string ls_string::
ExpandRelativePath(ls_string Path, ls_string CurrentDir)
{
    ls_stringbuf Result(tmp);
    // todo: path validation?
    // todo:  tmp2/tmp3/../tmp3/ <-- this is not supoorted (but should be... sadly)

    if (Path.Size) {
        Result.AppendString(CurrentDir);
        Result.AppendChar('/');
        Result.AppendString(Path);

        ls_parser String = Result;
        b32 Relative = true;

        if (ls_parser::Alpha(String.Data[0])) {
            if (String.Size > 2 && String.Data[1] == ':') {
                if (String.Data[2] == '/') {
                    // C:/
                    Relative = false;
                }

                String.At += 2;
            }
        } else if (String.Data[0] == '~') {
            // ~/a.txt
            if (String.Size > 1) {
                ++String.At;
            }

            Relative = false;
        } else if (String.Data[0] == '.' && String.Size > 1 && String.Data[1] == '/') {
            // ./a.txt
            ++String.At;
        }

        if (Relative && String.RemainingBytes()) {
            u32 Count = String.ReadDotDotSlashes();
            // ls_string::GoUpNFolders(&CurrentDir, Count);
            // CurrentDir.Size = CurrentDir.At - CurrentDir.Data;
            // CurrentDir.At = CurrentDir.Data;
        }
    }

    return Result;
}
#endif

u32 ls_parser::
ReadUtf8Codepoint(utf8 *String, u32 *Size)
{
    u32 Codepoint = (u8)String[0];
    u8 Bytes = 1;

    if (Codepoint >> 7 == 0) {
        Codepoint = String[0];
    } else if (Codepoint >> 5 == 0b110) {
        Bytes = 2;
        Codepoint = ((String[0] & 0b00011111) << 6) |
            (String[1] & 0b00111111);
    } else if (Codepoint >> 4 == 0b1110) {
        Bytes = 3;
        Codepoint = ((String[0] & 0b00001111) << 12) |
            ((String[1] & 0b00111111) << 6) |
            (String[2] & 0b00111111);
    } else if (Codepoint >> 3 == 0b11110) {
        Bytes = 4;
        Codepoint = ((String[0] & 0b00000111) << 18) |
            ((String[1] & 0b00111111) << 12) |
            ((String[2] & 0b00111111) << 6) |
            (String[3] & 0b00111111);
    }

    *Size = Bytes;
    return Codepoint;
}

u32 ls_parser::
ReadUtf8Codepoint()
{
    u32 Size = 0;
    u32 Codepoint = ls_parser::ReadUtf8Codepoint((utf8 *)this->At, &Size);
    this->At += Size;

    return Codepoint;
}

u32 ls_parser::
ReadUtf8CodepointBackwards()
{
    if (this->SafeToSubtract()) {
        --this->At;
    }

    u32 Codepoint = (u8)this->At[0];
    u8 ByteCount = 1;

    if (!(Codepoint & 0b10000000)) {
        return Codepoint;
    } else {
        while (this->SafeToSubtract() && ByteCount < 4) {
            --this->At;
            u8 Byte = (u8)this->At[0];
            u8 ShiftVal = ByteCount * 8;
            Codepoint += (Byte << ShiftVal);

            if (Byte & 0b01000000) {
                // todo: verify that ByteCount same as # of '1's in first byte
                break;
            }
        }
    }

    return Codepoint;
}

#endif // IMPLEMENTATION
#endif // INCLUDE_GUARD_LS_STRING
