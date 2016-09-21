#pragma once
#include <string>

void my_log(const char* format, ...);

extern const char* TokenNames[];

struct DocumentPosition
{
  DocumentPosition(unsigned line = 0, unsigned c = 0)
    :Line(line), Character(c)
  {

  }

  bool operator <(DocumentPosition &rhs)
  {
    if (Line != rhs.Line)
      return Line < rhs.Line;

    return Character < rhs.Character;
  }

  bool operator ==(DocumentPosition &rhs)
  {
    return Line == rhs.Line && Character == rhs.Character;
  }

  unsigned Line;
  unsigned Character;
};

class Token
{
public:
  Token();
  Token(const char* str, size_t length, int type);
  const char* Text;
  size_t Length;
  DocumentPosition Position;

  // A convenience function to get a sliced version of the string
  std::string str() const;

  // Allow streams to output our token
  friend std::ostream& operator<<(std::ostream& out, const Token& token);

  // A conversion to bool (only true if the token type is not Invalid)
  // This is useful for shorthanding some 'Expect' operations
  operator bool() const;

  struct Type
  {
    enum Enum
    {
#define TOKEN(Name, Value) Name,
#include "Tokens.inl"
#undef TOKEN
      EnumCount
    };
  };

  //Union to easily convert between enum and TokenType
  union
  {
    int TokenType;
    Token::Type::Enum EnumTokenType;
  };
};
