#include "lexer.h"
#include "a2lgrammar.h"
#include <sstream>
#include "qdebug.h"
#include "omp.h"

/**********  Class Buffer *************/

Buffer::Buffer()
{
    state= false;
    value = 0;
}

bool Buffer::isFull()
{
    return state;
}

void Buffer::read(QTextStream &in)
{
        char c;
        in >> c;


        if (c != 0 && c < 128)
        {
            value = c;
            state = true;
        }
        else if (c > 127)
        {
            value = '?';
            state = true;
        }
        else
        {
            value = 0;
            state = false;
        }


//    QChar c;
//    in >> c;

    
//    if (c != 0 && c.unicode() < 128)
//    {
//        value = c.toAscii();
//        state = true;
//    }
//    else if (c.unicode() > 127)
//    {
//        value = '?';
//        state = true;
//    }

//    else
//    {
//        value = 0;
//        state = false;
//    }
}

char Buffer::getAndClear()
{
    char c = value;
    this->clear();
    return c;
}

char Buffer::check()
{
    char c = value;
    return c;
}

void Buffer::clear()
{
    this->value = 0;
    this->state = false;
}

/**********  Class Lexer *************/

A2lLexer::A2lLexer(QObject *parent) : QObject(parent)
{
    grammar = new A2lGrammar();
    keywordsList = grammar->initKeywords();
    buffer = new Buffer();
    index = 0;
    position = 0;
    previousLine = 0;
}

A2lLexer::~A2lLexer()
{
    delete grammar;
    delete buffer;
}

std::string A2lLexer::getLexem()
{
    return lexem;
}

std::string A2lLexer::toString(TokenTyp type)
{
    switch (type)
    {
        case myUnknown:
            return "Unknown";
            break;
        case Identifier:
            return "Identifier";
            break;
        case String:
            return "String";
            break;
        case Hex:
            return "Hex";
            break;
        case Integer:
            return "Integer";
            break;
        case Comment:
            return "Comment";
            break;
        case Float:
            return "Float";
            break;
        case Plus:
            return "+";
            break;
        case Minus:
            return "-";
            break;
        case Mode:
            return "Mode";
            break;
        case BlockBegin:
            return "BlockBegin";
            break;
        case BlockEnd:
            return "BlockEnd";
            break;
        case Eof:
            return "Eof";
            break;
        case StringFormat:
            return "StringFormat";
            break;
        case Keyword:
            return "Keyword";
            break;
        case DataType:
            return "DataType";
            break;
        case Datasize:
            return "Datasize";
            break;
        case Addrtype:
            return "Addrtype";
            break;
        case Byteorder:
            return "Byteorder";
            break;
        case Indexorder:
            return "Indexorder";
            break;
        case ConversionType:
            return "ConversionType";
            break;
        case Type:
            return "Type";
            break;
        case Attribute:
            return "Attribute";
            break;
        case PrgType:
            return "PrgType";
            break;
        case MemoryType:
            return "MemoryType";
            break;
        case MemAttribute:
            return "MemAttribute";
            break;
        default:
            return "Unknown";
    }
}

TokenTyp A2lLexer::getNextToken(QTextStream &in)
{
    lexem = "";
    TokenTyp token = myUnknown;
    char ch;

    //First check if the buffer is empty or full
    //and process it if necessary
    if (buffer->isFull())
    {
        token = begin(in, buffer->getAndClear());
    }
    //Else read next char
    else
    {
        if (in.atEnd())
        {
            token = Eof;
            buffer->clear();
        }
        else
        {
            in >> ch;
            token = begin(in, ch);
        }
    }

    // emit return token for progressBar
    if (in.pos() - position > 20000 || in.atEnd())
    {
        emit returnedToken(in.pos() - position + line - previousLine );
        position = in.pos();
        previousLine = line;
    }

    return token;
}

bool A2lLexer::isSeparator(char ch)
{
    if (ch == '\n' || ch == '\r' || ch == '\t' || ch == 0 || ch == 32)
        return true;
    else
        return false;
}

bool A2lLexer::isDigit(char ch)
{
    if ((ch <= '9') && (ch >= '0'))
        return true;
    else
        return false;
}

bool A2lLexer::isHexDigit(char ch)
{
    if ((('0' <= ch) && (ch <= '9')) || (('A' <= ch) && (ch <= 'F'))  || (('a' <= ch) && (ch <= 'f')))
        return true;
    else
        return false;
}

bool A2lLexer::isLetter(char ch)
{
    if ((ch <= 'z' && ch >= 'a') || (ch <= 'Z' && ch >= 'A'))
        return true;
    else
        return false;
}

bool A2lLexer::isA2mlSym(char ch)
{
    if (ch == '{' || ch == '}' ||
        ch == '[' || ch == ']' ||
        ch == '(' || ch == ')' ||
        ch == ';' || ch == '*' ||
        ch == '=' || ch == ',' ||
        ch == ':')
        return true;
    else
        return false;

}

TokenTyp A2lLexer::begin(QTextStream &in, char ch)
{
    TokenTyp token = myUnknown;

    //ignore white space and tabs and new line as first character
    while (ch == '\t'  || ch == '\r' || ch == 32 || ch == '\n')
    {        
        if (ch == '\n')
            line++;
        in >> ch;
    }

    index = in.pos() - 1;
    //if at end of file
    if (ch == 0)
    {
        token = Eof;
        buffer->clear();
    }
    else
    {
        if (ch == '/')
        {
            in >> ch;
            if (ch == '/')
                token = commentL(in);
            else if (ch == '*')
                token = commentM(in);
            else if (isLetter(ch))
                token = block(in, ch);
            else
            {
                token = myUnknown;
                lexem = "/" + ch;
            }
        }
        else if (ch == '"')
        {
            token = string(in);
        }
        else if (ch == '-')
        {
            //token = Minus;
            //lexem = '-';
            //buffer->clear();
            token = this->number(in, ch);
        }
        else if (ch == '+')
        {
            //token = Plus;
            //lexem = '+';
            //buffer->clear();
            token = this->number(in, ch);
        }
        else if (isDigit(ch))
        {
            token = this->number(in, ch);
        }
        else if(isLetter(ch) || ch == '_')
        {
            token = this->identifier(in, ch);
        }
        /*else if (this->isA2mlSym(ch))
        {
            token = new Token;
            token = A2ml;
            lexem += ch;
            buffer->clear();
        }*/
        else
        {
            lexem += ch;
            buffer->clear();
        }
    }
     return token;
}

TokenTyp A2lLexer::commentM(QTextStream &in)
{
    TokenTyp token;

    token = Comment;
    lexem = "/*";

    bool exit = false;
    while (!exit)
    {
        buffer->read(in);
        if (buffer->check() == '*')
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
            while (buffer->check() == '*')
            {
                lexem += buffer->getAndClear();
                buffer->read(in);
            }

            if (buffer->check() == '/')
            {
                lexem += buffer->getAndClear();
                exit = true;
            }
            else if (buffer->check() == 0)
                exit = true;
            else
            {
                if (buffer->check() == '\n')
                    line++;
                lexem +=  buffer->getAndClear();
            }

        }
        else if (buffer->check() == 0)
        {
            exit = true;
        }
        else
        {
            if (buffer->check() == '\n')
                line++;
            lexem +=  buffer->getAndClear();
        }

    }
    return token;
}

TokenTyp A2lLexer::commentL(QTextStream &in)
{
    TokenTyp token;

    token = Comment;
    lexem = "//";

    buffer->read(in);
    while (buffer->check() != '\n' && buffer->check() != '\r' && buffer->check() != 0)
    {
        lexem += buffer->getAndClear();
        buffer->read(in);
    }
    return token;
}

TokenTyp A2lLexer::block(QTextStream &in, char &ch)
{
    TokenTyp token;
    lexem += '/';
    lexem += ch;

    buffer->read(in);
    while (isLetter(buffer->check()) || isDigit(buffer->check()) || buffer->check() == '_')
    {
         lexem += buffer->getAndClear();
         buffer->read(in);
    }

    if (lexem == "/begin")
        token = BlockBegin;
    else if (lexem == "/end")
        token = BlockEnd;
    else
        token = myUnknown;

    return token;
}

TokenTyp A2lLexer::identifier(QTextStream &in, char &ch)
{
    TokenTyp token = Identifier;
    lexem += ch;

    buffer->read(in);

    bool exit = false;
    while (!exit)
    {
        token = getPartialString(in);
        if (token == Identifier)
        {
            if (buffer->check() == '.')
            {
                lexem += buffer->getAndClear();
                buffer->read(in);
            }
            else if (isSeparator(buffer->check()))
                exit = true;
            else
            {
                lexem += buffer->getAndClear();
                token = myUnknown;
                exit = true;
            }
        }
        else
            exit = true;
    }

   if (token == Identifier)
    {

        TokenTyp tok = keywordsList.value(lexem.c_str());
        if (tok != 0)
            token = tok;
    }

    return token;
}

TokenTyp A2lLexer::number(QTextStream &in, char &ch)
{
    TokenTyp token;
    lexem += ch;

    buffer->read(in);
    while(isDigit(buffer->check()))
    {        
        lexem += buffer->getAndClear();
        buffer->read(in);
    }

    if (buffer->check() == '.')
    {
        lexem += buffer->getAndClear();
        buffer->read(in);
        while(isDigit(buffer->check()))
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
        }

        if (buffer->check() == 'E' || buffer->check() == 'e')
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
            if (buffer->check() == '+' || buffer->check() == '-')
            {
                lexem += buffer->getAndClear();
                buffer->read(in);
                while(isDigit(buffer->check()))
                {
                    lexem += buffer->getAndClear();
                    buffer->read(in);
                }
                token = Float;
                return token;
            }

            else
            {
                token = myUnknown;
                return token;
            }

        }
        else
        {
            token = Float;
            return token;
        }

    }
    else if(buffer->check() == 'E' || buffer->check() == 'e')
    {
        lexem += buffer->getAndClear();
        buffer->read(in);
        if (buffer->check() == '+' || buffer->check() == '-')
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
            while(isDigit(buffer->check()))
            {
                lexem += buffer->getAndClear();
                buffer->read(in);
            }
            token = Float;
            return token;
        }
        else
        {
            token = myUnknown;
            return token;
        }
    }
    else if (buffer->check() == 'x')
    {
         token = hexadecimal(in);
         return token;
    }
    else
    {
        token = Integer;
        return token;
    }
}

TokenTyp A2lLexer::string(QTextStream &in)
{
    TokenTyp token;
    lexem = '"';

    buffer->read(in);

    // StringFormat
    if (buffer->check() == '%')
    {
        token = StringFormat;
        lexem += buffer->getAndClear();
        buffer->read(in);
        while(isDigit(buffer->check()))
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
        }
        if (buffer->check() == '.')
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
            while(isDigit(buffer->check()))
            {
                lexem += buffer->getAndClear();
                buffer->read(in);
            }
            if (buffer->check() == '"')
            {
                lexem += buffer->getAndClear();
                return token;
            }
        }
    }

    // String
    token = String;
    bool exit = false;
    while (!exit)
    {        
        while (buffer->check() != '"' && buffer->check() != '\\' && buffer->check() != '\n' && buffer->check() != 0 )
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
        }

        if (buffer->check() == '"')
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
            if (buffer->check() == '"')
            {
                lexem += buffer->getAndClear();
                buffer->read(in);
            }
            else
                exit = true;

        }
        else if (buffer->check() == '\\')
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
            lexem += buffer->getAndClear();
            buffer->read(in);
        }
        else
        {
            token = myUnknown;
            exit = true;
        }
    }

    return token;
}

TokenTyp A2lLexer::hexadecimal(QTextStream &in)
{
    TokenTyp token;
    token = Hex;
    lexem = "0x";

    buffer->read(in);
    while (isHexDigit(buffer->check()))
    {
        lexem += buffer->getAndClear();
        buffer->read(in);
    }
    return token;
}

TokenTyp A2lLexer::getPartialString(QTextStream &in)
{
    TokenTyp token = Identifier;

    while (isDigit(buffer->check()) || isLetter(buffer->check()) || buffer->check() == '_')
    {
        lexem += buffer->getAndClear();
        buffer->read(in);        
    }

    if (buffer->check() == '[')
    {
        lexem += buffer->getAndClear();
        buffer->read(in);        

        while (isDigit(buffer->check()) || isLetter(buffer->check()) || buffer->check() == '_')
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
        }

        if (buffer->check() == ']')
        {
            lexem += buffer->getAndClear();
            buffer->read(in);
        }
        else
            token = myUnknown;
    }
    else if (buffer->check() == '.') {}
    else if (!isSeparator(buffer->check()))
    {
        lexem += buffer->getAndClear();
        token = myUnknown;
        buffer->read(in);
    }

    return token;
}

int A2lLexer::getLine()
{
    return line;
}

void A2lLexer::initialize()
{
    line = 1;
}

void A2lLexer::backward(QTextStream &in)
{
    int l = lexem.length() + 1;
    in.seek(in.pos() - l);
}
