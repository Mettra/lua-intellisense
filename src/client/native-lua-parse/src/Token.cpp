#include "Token.h"
#include <stdarg.h>
#include <cstring>

//Create names for all of the tokens
const char* TokenNames[] =
{
#define TOKEN(Name, Value) #Name,
#include "Tokens.inl"
#undef TOKEN
};

Token::Token() 
: Text(""), Length(0), TokenType(0)
{}

Token::Token(const char* text, size_t length, int type) 
  : Text(text), Length(length), TokenType(type)
{}

std::ostream& operator<<(std::ostream& out, const Token& token)
{
  out << token.str();
  return out;
}

std::string Token::str() const
{
  if (Text == nullptr || Length <= 0)
    return std::string();

  return std::string(Text, Length);
}

Token::operator bool() const
{
  return this->TokenType != Token::Type::Invalid;
}

#ifdef NODE_PRINT
std::string internal_parse_stdout;

void my_log(const char* format, ...)
{
  char buffer[1024];
  std::memset(buffer, 0, sizeof(buffer));

  va_list argptr;
  va_start(argptr, format);
  vsnprintf(buffer, sizeof(buffer) - 1, format, argptr);
  va_end(argptr);

  internal_parse_stdout += std::string(buffer);
}

#else
void my_log(const char* format, ...)
{
  va_list argptr;
  va_start(argptr, format);
  vfprintf(stderr, format, argptr);
  va_end(argptr);
}
#endif
