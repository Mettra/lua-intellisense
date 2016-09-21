#pragma once
#include <unordered_map>
#include "Token.h"

namespace Lexer
{
  class DfaState
  {
  public:
    DfaState(int acceptingToken)
      :acceptingToken(acceptingToken)
    {}

    int acceptingToken = 0;
    DfaState *default_edge = nullptr;
    DfaState *failure_edge = nullptr;

    std::unordered_map<char, DfaState *> edges;
    std::unordered_map<std::string, Token::Type::Enum> keywords;
  };

  DfaState* CreateLanguageDfa();
  void ReadToken(DfaState* startingState, const char* stream, Token& outToken, unsigned &lineNumber, unsigned &charNumber);
  void DeleteStateAndChildren(DfaState* root);
}