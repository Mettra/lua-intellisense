#pragma once
#include "AST_Nodes.h"
#include <exception>
#include <string>
#include <sstream>

//Undefine if you want no printing information
//#define PARSER_DEBUG


class ParsingException : public std::exception
{
public:
  ParsingException()
  {}

  ParsingException(const std::string& error, DocumentPosition position)
  {
    //this->error += "File:";
    //this->error += std::to_string(line_num);
    //this->error += ": ";
    this->error += error;
    this->position = position;
  }

  const char* what() const throw() override
  {
    return error.c_str();
  }

  std::string error;
  DocumentPosition position;
};

//Class used for debugging info when generating the ast.
#ifdef PARSER_DEBUG
class DescentInfo
{
public:
  DescentInfo(const char *ruleName);
  ~DescentInfo();

  //When accepting a rule, if a statement evaluates to true then the rule should be accepted.
  template <typename T>
  T Accept(T result)
  {
    if (result)
      this->Accept();
    return std::move(result);
  }
  
  bool Accept();

  //Called when token is accepted
  static void AcceptToken(Token::Type::Enum tokenType);


private:
  bool isAccepted;
  static std::vector<DescentInfo *> CurrentRules; //Used for recursive rules
  std::stringstream stream;
  std::string name;

  void PrintTabs();
  static DescentInfo *GetCurrentRule();
};
#else //Non-debug version of DescentInfo
class DescentInfo
{
public:
  DescentInfo(const char *ruleName){}
  ~DescentInfo(){}

  //When accepting a rule, if a statement evaluates to true then the rule should be accepted.
  template <typename T>
  T Accept(T result)
  {
    if (result)
      this->Accept();
    return std::move(result);
  }

  bool Accept(){ return true; }

  static void AcceptToken(Token::Type::Enum tokenType) {}
};
#endif


std::unique_ptr<AbstractNode> RecognizeTokens(std::vector<Token> &tokens, std::vector<ParsingException> *error = nullptr, bool throwException = false);
void RemoveWhitespaceAndComments(std::vector<Token> &tokens);
void PrintTree(AbstractNode* node);
void GenerateTree(AbstractNode* node);
