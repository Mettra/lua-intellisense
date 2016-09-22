TOKEN(Invalid,        "Invalid")

//Generic tokens for the lexer
TOKEN(Identifier,     "Identifier"      )
TOKEN(IntegerLiteral, "IntegerLiteral"  )
TOKEN(FloatLiteral,   "FloatLiteral"    )
TOKEN(StringLiteral,  "StringLiteral"   )
TOKEN(Whitespace,     "Whitespace"      )
TOKEN(Newline,        "Newline"         )
TOKEN(Comment,        "Comment"         )


//Denotes start of the symbol tokens (+, -, =, ...)
TOKEN(SymbolStart, "SymbolStart")
#include "Tokens_Symbol.inl"

//Denotes start of the keyword tokens (function, and, or, etc)
TOKEN(KeywordStart, "KeywordStart")
#include "Tokens_Keyword.inl"