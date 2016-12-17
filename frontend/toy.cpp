#include <string>
enum Token_type {
    EOF_TOKEN = 0,
    NUMERIC_TOKEN,
    IDENTIFIER_TOKEN,
    PARAN_TOKEN,
    DEF_TOKEN
};

static int Numeric_Val;

static std::string Identifier_string;

static int get_token() {
    static int LastChar = ' ';

    while (isspace(LastChar))
        LastChar = fgetc(file);

    if (isalpha(LastChar)) {
        Identifier_string = LastChar;
        while (isalnum((LastChar = fgetc(file))))
            Identifier_string += LastChar;

        if (Identifier_string == "def")
            return DEF_TOKEN;

        return IDENTIFIER_TOKEN;
    }

    if (isdigit(LastChar)) {
        stad::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = fgetc(file);
        } while (isdigit(LastChar));

        Numeric_Val = strtod(NumStr.c_str(), 0);
        return NUMERIC_TOKEN;
    }

    if (LastChar == '#') {
        do
            LastChar = fgetc(file);
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return get_token();
    }

    if (LastChar == EOF)
        return EOF_TOKEN;

    int ThisChar = LastChar;
    LastChar = fgetc(file);
    return ThisChar;
}
