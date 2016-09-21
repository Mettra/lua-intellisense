#include "Descent_Parser.h"
#include "Token.h"
#include <vector>

//  Descent_Info  //
#ifdef PARSER_DEBUG
bool DescentInfo::Accept()
{
  isAccepted = true;
  return true;
}

DescentInfo *DescentInfo::GetCurrentRule()
{
  if (CurrentRules.empty())
    return nullptr;

  return CurrentRules.back();
}

void DescentInfo::PrintTabs()
{
  for (size_t i = 0; i < CurrentRules.size(); ++i)
    stream << "| ";
}

std::vector<DescentInfo *> DescentInfo::CurrentRules;

DescentInfo::DescentInfo(const char* ruleName) :
isAccepted(false),
name(ruleName)
{
  PrintTabs();
  stream << ruleName << "\n";
  CurrentRules.push_back(this);
}

DescentInfo::~DescentInfo()
{
  CurrentRules.pop_back();

  if (isAccepted || std::uncaught_exception())
  {
    PrintTabs();

    if (std::uncaught_exception())
      stream << "End" << name << "*\n";
    else
      stream << "End" << name << "\n";

    if (CurrentRules.empty())
      my_log("%s\n", stream.str().c_str());
    else
      CurrentRules.back()->stream << stream.str();
  }
}


void DescentInfo::AcceptToken(Token::Type::Enum tokenType)
{
  DescentInfo *currentRule = GetCurrentRule();
  if (currentRule == nullptr)
  {
    my_log("ERROR! Trying to accept a token without defining a rule. (Did you forget to create to contstruct a DescentInfo?)\n");
    return;
  }

  currentRule->PrintTabs();
  currentRule->stream << "Accepted: " << TokenNames[tokenType] << "\n";
}
#endif


void RemoveWhitespaceAndComments(std::vector<Token>& tokens)
{
  for (auto it = tokens.begin(); it != tokens.end();)
  {
    if (it->EnumTokenType == Token::Type::Whitespace ||
      it->EnumTokenType == Token::Type::Comment)
    {
      it = tokens.erase(it);
    }
    else
      ++it;
  }
}


std::string TokenIdentifier(Token &t)
{
  return std::string(t.Text, t.Length);
}

struct RecursiveParser
{
  RecursiveParser(std::vector<Token>& tokens)
    :tokens(tokens), tokenStream(0)
  {}

  std::vector<Token>& tokens;
  unsigned tokenStream;
  bool throwException = false;

  std::vector<ParsingException> errors;

  DocumentPosition GetCurrentPosition()
  {
    if (tokenStream - 1 >= tokens.size() || tokenStream - 1 < 0)
      return DocumentPosition();

    return tokens[tokenStream - 1].Position;
  }

  bool Accept(Token::Type::Enum type)
  {
    if (tokenStream >= tokens.size())
      return nullptr;

    if (tokens[tokenStream].EnumTokenType == type)
    {
      tokenStream++;
      DescentInfo::AcceptToken(type);
      return true;
    }

    return nullptr;
  }

  bool Accept(Token::Type::Enum type, Token &t)
  {
    if (tokenStream >= tokens.size())
      return nullptr;

    if (tokens[tokenStream].EnumTokenType == type)
    {
      t = tokens[tokenStream];
      tokenStream++;
      DescentInfo::AcceptToken(type);
      return true;
    }

    return nullptr;
  }

  bool Expect(Token::Type::Enum type)
  {
    if (Accept(type))
      return true;
    else
    {
      if (tokenStream >= tokens.size())
        errors.push_back(ParsingException("End of token stream!", tokens[tokens.size() - 1].Position));
      else
        errors.push_back(ParsingException(std::string("Expected ") + TokenNames[(int)type] + ", found " + TokenNames[(int)tokens[tokenStream].EnumTokenType] + ".", tokens[tokenStream].Position));

      if (throwException)
        throw errors.back();

      return false;
    }
  }

  bool Expect(Token::Type::Enum type, Token &t)
  {
    if (Accept(type, t))
      return true;
    else
    {
      if (tokenStream >= tokens.size())
        errors.push_back(ParsingException("End of token stream!", tokens[tokens.size() - 1].Position));
      else
        errors.push_back(ParsingException(std::string("Expected ") + TokenNames[(int)type] + ", found " + TokenNames[(int)tokens[tokenStream].EnumTokenType] + ".", tokens[tokenStream].Position));

      if (throwException)
        throw errors.back();

      return false;
    }
  }

  struct AllTrue
  {
    static bool Collect()
    {
      return true;
    }

    template<typename Arg, typename... Args>
    static bool Collect(Arg a, Args... args)
    {
      return a && Collect(args...);
    }
  };

  template<typename Arg>
  std::string CollectTokenNames(Arg a)
  {
    return std::string(TokenNames[(int)a]);
  }

  template<typename Arg, typename... Args>
  std::string CollectTokenNames(Arg a, Args... args)
  {
    return std::string(TokenNames[(int)a]) + " or " + CollectTokenNames(args...);
  }


  template<typename... Args>
  bool ExpectOne(Args... args)
  {
    if (AllTrue::Collect(Accept(args)...))
      return true;
    else
    {
      if (tokenStream >= tokens.size())
        errors.push_back(ParsingException("End of token stream!"));
      else
        errors.push_back(ParsingException("Expected "s + CollectTokenNames(args...) + ", found " + TokenNames[(int)tokens[tokenStream].EnumTokenType] + "."));

      if (throwException)
        throw errors.back();

      return false;
    }
  }

  bool Expect(bool value)
  {
    if (!value)
    {
      //errors.push_back(ParsingException("Expected this operation to succeed.", -1));
      //
      //if (throwException)
      //  throw errors.back();

      return false;
    }

    return true;
  }

  template<typename T>
  T Expect(T value)
  {
    if (!value)
    {
      //errors.push_back(ParsingException());
      //
      //if (throwException)
      //  throw errors.back();

      return nullptr;
    }

    return std::move(value);
  }

  bool Expect(Token::Type::Enum type, std::vector<Token>& tokens, unsigned &tokenStream, std::string const &error)
  {
    if (Accept(type))
      return true;
    else
    {
      errors.push_back(ParsingException(error, tokens[tokenStream].Position));

      if (throwException)
        throw errors.back();

      return false;
    }
  }


  // ------------------------------------------------ //
  //                Start of Rules                    //
  // ------------------------------------------------ //
  std::unique_ptr<AbstractNode> Start()
  {
    DescentInfo rule("Start");

    std::unique_ptr<FunctionNode> mainFunction = std::make_unique<FunctionNode>();
    mainFunction->Position = GetCurrentPosition();

    mainFunction->Block = Chunk();

    if (tokenStream != tokens.size())
    {
      errors.push_back(ParsingException("Syntax error near '" + tokens[tokenStream].str() + "'", tokens[tokenStream].Position));
      if (throwException)
        throw errors.back();
    }

    //Destroy the "end" node of the main chunk
    mainFunction->Block->End = nullptr;

    return rule.Accept(std::move(mainFunction));
  }

  std::unique_ptr<BlockNode> Chunk()
  {
    DescentInfo rule("Chunk");

    std::unique_ptr<BlockNode> blockNode = std::make_unique<BlockNode>();
    blockNode->Position = GetCurrentPosition();

    //@REM: This is a hack, but it shouldn't interfere with anything
    blockNode->Position.Character++;

    //Any number of statements
    for (;;)
    {
      std::unique_ptr<StatementNode> statementNode = Statement();
      if (statementNode)
      {
        //Optional Semicolon
        Accept(Token::Type::Semicolon);

        blockNode->Statements.push_back(std::move(statementNode));
        continue;
      }

      break;
    }

    //Optionally followed by a last statement
    std::unique_ptr<StatementNode> lastStatement = LastStatement();
    if (lastStatement)
    {
      //Optional Semicolon
      Accept(Token::Type::Semicolon);

      blockNode->Statements.push_back(std::move(lastStatement));
    }

    blockNode->End = std::make_unique<AbstractNode>();
    blockNode->End->Position = GetCurrentPosition();

    return rule.Accept(std::move(blockNode));
  }

  std::unique_ptr<StatementNode> Statement()
  {
    DescentInfo rule("Statement");

    //Resolve ambiguity, both Assignment and FunctionCall can start with a var
    auto var_or_exp = VariableStatement();
    if (var_or_exp)
    {

      //If invalid, it should be a function call
      if (Invalid_Variable(var_or_exp))
      {
        std::unique_ptr<FunctionCallNode> functionCallNode = std::make_unique<FunctionCallNode>();
        functionCallNode->Position = GetCurrentPosition();

        //Copy should be function calls
        for (auto &&node : var_or_exp->Suffix->CallNodes)
        {
          functionCallNode->Calls.push_back(std::move(node));
        }
        var_or_exp->Suffix->CallNodes.clear();

        functionCallNode->Variable = std::move(var_or_exp);

        return rule.Accept(std::move(functionCallNode));
      }
      else
      {
        std::unique_ptr<AssignmentNode> assignmentNode = std::make_unique<AssignmentNode>();
        assignmentNode->LeftVariables.push_back(std::move(var_or_exp));

        bool assigned = Expect(Assignment(assignmentNode));
        if (assigned)
        {
          assignmentNode->Position = GetCurrentPosition();
        }

        return rule.Accept(std::move(assignmentNode));
      }

      /*
      //If its an expression, or if the next token is a : or (, it has to be a function call
      if (dynamic_cast<CallableExpressionNode *>(var_or_exp.get()) || (tokens[tokenStream].EnumTokenType == Token::Type::Colon || tokens[tokenStream].EnumTokenType == Token::Type::OpenParentheses))
      {
        std::unique_ptr<FunctionCallNode> functionCallNode = std::make_unique<FunctionCallNode>();
        functionCallNode->LeftVar = std::move(var_or_exp);

        Expect(FunctionCall(functionCallNode));

        return rule.Accept(std::move(functionCallNode));
      }
      //Else it should be a normal assignment
      else if (dynamic_cast<CallableVariableNode *>(var_or_exp.get()))
      {
        std::unique_ptr<AssignmentNode> assignmentNode = std::make_unique<AssignmentNode>();

        Expect(Assignment(assignmentNode));

        assignmentNode->LeftVariables.push_back(std::move(dynamic_cast<CallableVariableNode *>(var_or_exp.get())->Variable));
      }
      //Shouldn't happen
      else
      {
        throw ParsingException("Error determening assignment or function call rule!");
      }
      */
    }

    if (Accept(Token::Type::Do))
    {
      std::unique_ptr<BlockNode> blockNode = Chunk();
      Expect(Token::Type::End);
      blockNode->End->Position = GetCurrentPosition();

      return rule.Accept(std::move(blockNode));
    }

    if (Accept(Token::Type::While))
    {
      std::unique_ptr<WhileNode> whileNode = std::make_unique<WhileNode>();
      whileNode->Position = GetCurrentPosition();
      whileNode->Condition = Expect(Expression());

      Expect(Token::Type::Do);

      whileNode->Block = Expect(Chunk());

      Expect(Token::Type::End);
      whileNode->Block->End->Position = GetCurrentPosition();

      return rule.Accept(std::move(whileNode));
    }

    if (Accept(Token::Type::Repeat))
    {
      std::unique_ptr<RepeatNode> repeatNode = std::make_unique<RepeatNode>();
      repeatNode->Position = GetCurrentPosition();
      repeatNode->Block = Expect(Chunk());

      Expect(Token::Type::Until);

      repeatNode->Condition = Expect(Expression());
      repeatNode->Block->End->Position = GetCurrentPosition();

      return rule.Accept(std::move(repeatNode));
    }

    if (Accept(Token::Type::If))
    {
      std::unique_ptr<IfNode> ifNode = std::make_unique<IfNode>();
      ifNode->Position = GetCurrentPosition();
      ifNode->Condition = Expect(Expression());

      Expect(Token::Type::Then);

      ifNode->Block = Expect(Chunk());

      ifNode->Else = Else();

      Expect(Token::Type::End);
      if (ifNode->Else == nullptr)
      {
        ifNode->Block->End->Position = GetCurrentPosition();
      }
      else
      {
        ifNode->Block->End->Position = ifNode->Else->Position;

        //Traverse all the way down and correct the leaf node
        IfNode *currentNode = ifNode.get();
        while (currentNode->Else)
        {
          currentNode = currentNode->Else.get();
        }

        currentNode->Block->End->Position = GetCurrentPosition();
      }

      return rule.Accept(std::move(ifNode));
    }

    if (Accept(Token::Type::For))
    {
      std::vector<Token> names;
      Expect(IdentifierList(names));

      //Numeric
      if (Accept(Token::Type::Assignment))
      {
        std::unique_ptr<NumericForNode> numericNode = std::make_unique<NumericForNode>();
        numericNode->Position = GetCurrentPosition();
        numericNode->Var = Expect(Expression());
        Expect(Token::Type::Comma);
        numericNode->Limit = Expect(Expression());

        if (Accept(Token::Type::Comma))
        {
          numericNode->Step = Expect(Expression());
        }

        Expect(Token::Type::Do);
        numericNode->Block = Expect(Chunk());
        Expect(Token::Type::End);
        numericNode->Block->End->Position = GetCurrentPosition();

        return rule.Accept(std::move(numericNode));
      }
      //Generic
      else if (Accept(Token::Type::In))
      {
        std::unique_ptr<GenericForNode> genericNode = std::make_unique<GenericForNode>();
        genericNode->Position = GetCurrentPosition();
        genericNode->Names = names;

        Expect(ExpressionList(genericNode->ExpressionList));

        Expect(Token::Type::Do);
        genericNode->Block = Expect(Chunk());
        Expect(Token::Type::End);
        genericNode->Block->End->Position = GetCurrentPosition();

        return rule.Accept(std::move(genericNode));
      }
      else
      {
        errors.push_back(ParsingException(std::string("Expected = or in, found ") + TokenNames[(int)tokens[tokenStream - 1].EnumTokenType] + ".", tokens[tokenStream - 1].Position));
        if (throwException)
          throw errors.back();

        return false;
      }
    }

    if (Accept(Token::Type::Function))
    {
      std::unique_ptr<FunctionNode> functionNode = std::make_unique<FunctionNode>();
      functionNode->Position = GetCurrentPosition();

      std::unique_ptr<FunctionNameNode> nameNode = std::make_unique<FunctionNameNode>();
      Expect(Token::Type::Identifier, nameNode->Name);
      nameNode->Position = GetCurrentPosition();
      functionNode->Name.push_back(std::move(nameNode));

      for (;;)
      {
        if (Accept(Token::Type::Dot))
        {
          std::unique_ptr<FunctionNameNode> nameNode = std::make_unique<FunctionNameNode>();
          Expect(Token::Type::Identifier, nameNode->Name);
          nameNode->Position = GetCurrentPosition();
          functionNode->Name.push_back(std::move(nameNode));

          continue;
        }

        if (Accept(Token::Type::Colon))
        {
          std::unique_ptr<FunctionNameNode> nameNode = std::make_unique<FunctionNameNode>();
          nameNode->IsMemberFunc = true;
          Expect(Token::Type::Identifier, nameNode->Name);
          nameNode->Position = GetCurrentPosition();
          functionNode->Name.push_back(std::move(nameNode));

        }

        break;
      }

      FunctionBody(functionNode);
      return rule.Accept(std::move(functionNode));
    }

    if (Accept(Token::Type::Local))
    {
      if (Accept(Token::Type::Function))
      {
        std::unique_ptr<FunctionNode> functionNode = std::make_unique<FunctionNode>();
        functionNode->Position = GetCurrentPosition();
        functionNode->IsLocal = true;

        std::unique_ptr<FunctionNameNode> nameNode = std::make_unique<FunctionNameNode>();
        Expect(Token::Type::Identifier, nameNode->Name);
        nameNode->Position = GetCurrentPosition();
        functionNode->Name.push_back(std::move(nameNode));

        FunctionBody(functionNode);
        return rule.Accept(std::move(functionNode));
      }

      std::unique_ptr<LocalVariableNode> localVarNode = std::make_unique<LocalVariableNode>();
      localVarNode->Position = GetCurrentPosition();
      Expect(IdentifierList(localVarNode->Names));

      if (Accept(Token::Type::Assignment))
      {
        Expect(ExpressionList(localVarNode->ExpressionList));
      }

      return rule.Accept(std::move(localVarNode));
    }

    return nullptr;
  }

  std::unique_ptr<StatementNode> LastStatement()
  {
    DescentInfo rule("LastStatement");

    std::unique_ptr<StatementNode> statementNode = nullptr;

    if (Accept(Token::Type::Break))
    {
      statementNode = std::make_unique<BreakNode>();
      statementNode->Position = GetCurrentPosition();
    }
    else if (Accept(Token::Type::Return))
    {
      std::unique_ptr<ReturnNode> returnNode = std::make_unique<ReturnNode>();
      returnNode->Position = GetCurrentPosition();

      ExpressionList(returnNode->ReturnValues);

      statementNode = std::move(returnNode);
    }

    return rule.Accept(std::move(statementNode));
  }

  bool Assignment(std::unique_ptr<AssignmentNode> &assignmentNode)
  {
    DescentInfo rule("Assignment");

    //Grab rest of variables
    std::unique_ptr<VariableStatementNode> variable;
    for (;;)
    {
      if (Accept(Token::Type::Comma))
      {
        variable = VariableStatement();
        if (variable)
        {
          assignmentNode->LeftVariables.push_back(std::move(variable));
          continue;
        }
      }

      break;
    }

    //Get =, +=, -=, *=, or /= sign
    if (Accept(Token::Type::Assignment, assignmentNode->Operator) ||
      Accept(Token::Type::AssignmentDivide, assignmentNode->Operator) ||
      Accept(Token::Type::AssignmentMinus, assignmentNode->Operator) ||
      Accept(Token::Type::AssignmentMultiply, assignmentNode->Operator) ||
      Accept(Token::Type::AssignmentPlus, assignmentNode->Operator))
    {}
    else
    {
      errors.push_back(ParsingException(std::string("Expected =, +=, -=, *=, or /=, found ") + TokenNames[(int)tokens[tokenStream - 1].EnumTokenType] + ".", tokens[tokenStream - 1].Position));
      if (throwException)
        throw errors.back();

      return false;
    }

    //Get expressions
    Expect(ExpressionList(assignmentNode->RightExpressions));

    return true;
  }

  std::unique_ptr<VariableStatementNode> VariableStatement()
  {
    DescentInfo rule("VariableStatement");

    std::unique_ptr<VariableStatementNode> variable = std::make_unique<VariableStatementNode>();
    variable->Position = GetCurrentPosition();

    //If we find a name
    Token name;
    if (Accept(Token::Type::Identifier, name))
    {
      std::unique_ptr<IdentifiedVariableNode> identifierVariable = std::make_unique<IdentifiedVariableNode>();
      identifierVariable->Name = name;
      identifierVariable->Position = GetCurrentPosition();
      
      variable->Variable = std::move(identifierVariable);
    }
    //We need an expression variable
    else if (Accept(Token::Type::OpenParentheses))
    {
      std::unique_ptr<ExpressionVariableNode> expressionVariable = std::make_unique<ExpressionVariableNode>();
      expressionVariable->Position = GetCurrentPosition();

      //Grab expression and closing )
      expressionVariable->Expression = Expect(Expression());
      Expect(Token::Type::CloseParentheses);

      expressionVariable->Suffix = std::move(VariableSuffix());

      //If the suffix should be a function call, fix it up and return
      if (expressionVariable->Suffix && expressionVariable->Suffix->Index == nullptr)
      {
        variable->Suffix = std::move(expressionVariable->Suffix);
        expressionVariable->Suffix = nullptr;

        variable->Variable = std::move(expressionVariable);

        return rule.Accept(std::move(variable));
      }

      variable->Variable = std::move(expressionVariable);
    }
    else
      return nullptr;


    //Possible 0 or more suffix
    std::unique_ptr<VariableSuffixNode> currentSuffix = nullptr;

    for (;;)
    {
      std::unique_ptr<VariableSuffixNode> newSuffix = VariableSuffix();
      if (newSuffix)
      {
        //If there was a last suffix, set the left pointer
        newSuffix->LeftSuffix = std::move(currentSuffix);
        currentSuffix = std::move(newSuffix);

        continue;
      }

      break;
    }

    variable->Suffix = std::move(currentSuffix);


    return rule.Accept(std::move(variable));
  }

  std::unique_ptr<VariableSuffixNode> VariableSuffix()
  {
    DescentInfo rule("VariableSuffix");

    std::unique_ptr<VariableSuffixNode> suffixNode = std::make_unique<VariableSuffixNode>();
    suffixNode->Position = GetCurrentPosition();

    for (;;)
    {
      //Attempt to find a call node
      std::unique_ptr<CallNode> callNode = Call();
      if (callNode)
      {
        //If found, add it to the list
        suffixNode->CallNodes.push_back(std::move(callNode));
        continue;
      }

      break;
    }

    //Then look for an index node
    if (Accept(Token::Type::OpenSquare))
    {
      //This means it is an expression index
      std::unique_ptr<ExpressionIndexNode> expressionIndex = std::make_unique<ExpressionIndexNode>();
      expressionIndex->Position = GetCurrentPosition();

      expressionIndex->Expression = Expect(Expression());
      Expect(Token::Type::CloseSquare);

      suffixNode->Index = std::move(expressionIndex);
    }
    else if (Accept(Token::Type::Dot))
    {
      //Must be an identifier index
      std::unique_ptr<IdentifiedIndexNode> identifiedIndex = std::make_unique<IdentifiedIndexNode>();
      identifiedIndex->Position = GetCurrentPosition();

      Accept(Token::Type::Identifier, identifiedIndex->Name);

      suffixNode->Index = std::move(identifiedIndex);
    }
    else
    {
      if (suffixNode->CallNodes.size() > 0)
      {
        return rule.Accept(std::move(suffixNode));
      }

      return nullptr;
    }

    return rule.Accept(std::move(suffixNode));
  }

  std::unique_ptr<CallNode> Call()
  {
    DescentInfo rule("Call");

    std::unique_ptr<CallNode> callNode;

    //If colon, it is a member call node
    if (Accept(Token::Type::Colon))
    {
      std::unique_ptr<MemberCallNode> memberCallNode = std::make_unique<MemberCallNode>();
      memberCallNode->Position = GetCurrentPosition();

      bool valid = Expect(Token::Type::Identifier, memberCallNode->Name);

      callNode = std::move(memberCallNode);

      //Invalid call node should be returned for autocomplete
      if (!valid)
        return std::move(callNode);
    }
    //Standard call node
    else
    {
      callNode = std::make_unique<CallNode>();
      callNode->Position = GetCurrentPosition();
    }

    callNode->Argument = Arguments();
    if (callNode->Argument == nullptr)
    {
      return nullptr;
    }

    return rule.Accept(std::move(callNode));
  }

  std::unique_ptr<ArgumentNode> Arguments()
  {
    DescentInfo rule("Arguments");

    std::unique_ptr<ArgumentNode> node = nullptr;

    if (Accept(Token::Type::OpenParentheses))
    {
      std::unique_ptr<ExpressionArgumentNode> expressionNode = std::make_unique <ExpressionArgumentNode>();
      expressionNode->Position = GetCurrentPosition();

      ExpressionList(expressionNode->ExpressionList);

      Expect(Token::Type::CloseParentheses);

      node = std::move(expressionNode);
    }
    else
    {
      Token string;
      if (Accept(Token::Type::StringLiteral, string))
      {
        //@TODO See if this ever gets triggered...

        std::unique_ptr<StringArgumentNode> stringNode = std::make_unique <StringArgumentNode>();
        stringNode->String = string;
        stringNode->Position = GetCurrentPosition();

        node = std::move(stringNode);
      }
      else
      {
        std::unique_ptr<TableNode> tableNode = Table();
        if (!tableNode)
        {
          return nullptr;
        }

        std::unique_ptr<TableArgumentNode> tableArgumentNode = std::make_unique <TableArgumentNode>();
        tableArgumentNode->Table = std::move(tableNode);
        tableArgumentNode->Position = GetCurrentPosition();

        node = std::move(tableArgumentNode);
      }
    }


    return rule.Accept(std::move(node));
  }

  std::unique_ptr<TableNode> Table()
  {
    DescentInfo rule("Table");

    if (!Accept(Token::Type::OpenCurley))
      return nullptr;

    std::unique_ptr<TableNode> tableNode = std::make_unique<TableNode>();
    tableNode->Position = GetCurrentPosition();

    FieldList(tableNode);

    Expect(Token::Type::CloseCurley);

    return rule.Accept(std::move(tableNode));
  }

  void FieldList(std::unique_ptr<TableNode> &tableNode)
  {
    DescentInfo rule("FieldList");

    std::unique_ptr<IndexNode> index = nullptr;
    std::unique_ptr<ExpressionNode> value = nullptr;
    if (!Field(index, value))
      return;

    //Found a field! Insert into table
    tableNode->FullIndicies.push_back(index.get());
    tableNode->FullValues.push_back(value.get());
    tableNode->Indicies.push_back(std::move(index));
    tableNode->Values.push_back(std::move(value));

    //Get more fields
    bool end_separator = false;

    for (;;)
    {
      if (!Accept(Token::Type::Comma) && !Accept(Token::Type::Semicolon))
        break;

      index = nullptr;
      value = nullptr;
      if (!Field(index, value))
      {
        end_separator = true;
        break;
      }

      //Found a field! Insert into table
      tableNode->FullIndicies.push_back(index.get());
      tableNode->FullValues.push_back(value.get());
      tableNode->Indicies.push_back(std::move(index));
      tableNode->Values.push_back(std::move(value));
    }

    //If no end separator was found
    if (!end_separator)
    {
      if (!Accept(Token::Type::Comma))
        Accept(Token::Type::Semicolon);
    }
  }

  bool Field(std::unique_ptr<IndexNode> &index, std::unique_ptr<ExpressionNode> &value)
  {
    DescentInfo rule("Field");

    Token name;

    //'[' exp ']' '=' exp
    if (Accept(Token::Type::OpenSquare))
    {
      std::unique_ptr<ExpressionIndexNode> indexExpression = std::make_unique<ExpressionIndexNode>();
      indexExpression->Position = GetCurrentPosition();

      indexExpression->Expression = Expect(Expression());
      Expect(Token::Type::CloseSquare);
      Expect(Token::Type::Assignment);

      std::unique_ptr<ExpressionNode> expression = Expect(Expression());

      index = std::move(indexExpression);
      value = std::move(expression);
      return rule.Accept();
    }
    //NAME '=' exp
    else if (Accept(Token::Type::Identifier, name))
    {
      std::unique_ptr<IdentifiedIndexNode> indexIdentified = std::make_unique<IdentifiedIndexNode>();
      indexIdentified->Name = name;
      indexIdentified->Position = GetCurrentPosition();

      if (Accept(Token::Type::Assignment))
      {
        std::unique_ptr<ExpressionNode> expression = Expect(Expression());
        value = std::move(expression);
      }

      index = std::move(indexIdentified);
      return rule.Accept();
    }
    //exp
    else
    {
      std::unique_ptr<ExpressionNode> expression = Expression();
      if (!expression)
        return false;

      index = nullptr;
      value = std::move(expression);
      return rule.Accept();
    }

    return false;
  }

  bool ExpressionList(unique_vector<ExpressionNode> &expressions)
  {
    DescentInfo rule("ExpressionList");

    std::unique_ptr<ExpressionNode> expression = Expression();
    if (!expression)
      return false;

    expressions.push_back(std::move(expression));

    //Get more expressions
    if (Accept(Token::Type::Comma))
    {
      for (;;)
      {
        expression = Expect(Expression());
        expressions.push_back(std::move(expression));

        if (!Accept(Token::Type::Comma))
          break;
      }
    }
    
    return rule.Accept();
  }

  std::unique_ptr<ExpressionNode> Expression()
  {
    DescentInfo rule("Expression");

    auto expression = Expression1();
    if (!(expression))
      return nullptr;

    //Token Operator;
    //if (Accept(Token::Type::Assignment, Operator) ||
    //  Accept(Token::Type::AssignmentPlus, Operator) ||
    //  Accept(Token::Type::AssignmentMinus, Operator) ||
    //  Accept(Token::Type::AssignmentMultiply, Operator) ||
    //  Accept(Token::Type::AssignmentDivide, Operator))
    //{
    //  std::unique_ptr<BinaryOperatorNode> binOpNode = std::make_unique<BinaryOperatorNode>();
    //  binOpNode->Operator = Operator;
    //  binOpNode->Left = std::move(expression);
    //  binOpNode->Right = Expression();
    //  expression = std::move(binOpNode);
    //}

    return rule.Accept(std::move(expression));
  }

  std::unique_ptr<ExpressionNode> Expression1()
  {
    DescentInfo rule("Expression1");

    auto expression = Expression2();
    if (!(expression))
      return nullptr;

    Token Operator;
    for (;;)
    {
      if (Accept(Token::Type::Or, Operator))
      {
        std::unique_ptr<BinaryOperatorNode> binOpNode = std::make_unique<BinaryOperatorNode>();
        binOpNode->Position = GetCurrentPosition();
        binOpNode->Operator = Operator;
        binOpNode->Left = std::move(expression);
        binOpNode->Right = Expect(Expression2());
        expression = std::move(binOpNode);
        continue;
      }

      break;
    }

    return rule.Accept(std::move(expression));
  }

  std::unique_ptr<ExpressionNode> Expression2()
  {
    DescentInfo rule("Expression2");

    auto expression = Expression3();
    if (!(expression))
      return nullptr;

    Token Operator;

    for (;;)
    {
      if (Accept(Token::Type::And, Operator))
      {
        std::unique_ptr<BinaryOperatorNode> binOpNode = std::make_unique<BinaryOperatorNode>();
        binOpNode->Position = GetCurrentPosition();
        binOpNode->Operator = Operator;
        binOpNode->Left = std::move(expression);
        binOpNode->Right = Expect(Expression3());
        expression = std::move(binOpNode);
        continue;
      }

      break;
    }

    return rule.Accept(std::move(expression));
  }

  std::unique_ptr<ExpressionNode> Expression3()
  {
    DescentInfo rule("Expression3");

    auto expression = Expression4();
    if (!(expression))
      return nullptr;

    Token Operator;

    for (;;)
    {
      if (Accept(Token::Type::LessThan, Operator)
        || (Accept(Token::Type::GreaterThan, Operator))
        || (Accept(Token::Type::LessThanOrEqualTo, Operator))
        || (Accept(Token::Type::GreaterThanOrEqualTo, Operator))
        || (Accept(Token::Type::EqualsTo, Operator))
        || (Accept(Token::Type::NotEqualsTo, Operator)))
      {
        std::unique_ptr<BinaryOperatorNode> binOpNode = std::make_unique<BinaryOperatorNode>();
        binOpNode->Position = GetCurrentPosition();
        binOpNode->Operator = Operator;
        binOpNode->Left = std::move(expression);
        binOpNode->Right = Expect(Expression4());
        expression = std::move(binOpNode);

        continue;
      }

      break;
    }

    return rule.Accept(std::move(expression));
  }

  std::unique_ptr<ExpressionNode> Expression4()
  {
    DescentInfo rule("Expression4");

    auto expression = Expression5();
    if (!(expression))
      return nullptr;

    Token Operator;

    for (;;)
    {
      if (Accept(Token::Type::Concat, Operator))
      {
        std::unique_ptr<BinaryOperatorNode> binOpNode = std::make_unique<BinaryOperatorNode>();
        binOpNode->Position = GetCurrentPosition();
        binOpNode->Operator = Operator;
        binOpNode->Left = std::move(expression);
        binOpNode->Right = Expect(Expression5());
        expression = std::move(binOpNode);

        continue;
      }

      break;
    }

    return rule.Accept(std::move(expression));
  }

  std::unique_ptr<ExpressionNode> Expression5()
  {
    DescentInfo rule("Expression5");

    auto expression = Expression6();
    if (!(expression))
      return nullptr;

    Token Operator;

    for (;;)
    {
      if (Accept(Token::Type::Plus, Operator) || Accept(Token::Type::Minus, Operator))
      {
        std::unique_ptr<BinaryOperatorNode> binOpNode = std::make_unique<BinaryOperatorNode>();
        binOpNode->Position = GetCurrentPosition();
        binOpNode->Operator = Operator;
        binOpNode->Left = std::move(expression);
        binOpNode->Right = Expect(Expression6());
        expression = std::move(binOpNode);

        continue;
      }

      break;
    }

    return rule.Accept(std::move(expression));
  }

  std::unique_ptr<ExpressionNode> Expression6()
  {
    DescentInfo rule("Expression6");

    auto expression = Expression7();
    if (!(expression))
      return nullptr;

    Token Operator;

    for (;;)
    {
      if (Accept(Token::Type::Multiply, Operator)
        || Accept(Token::Type::Divide, Operator)
        || Accept(Token::Type::Modulo, Operator))
      {
        std::unique_ptr<BinaryOperatorNode> binOpNode = std::make_unique<BinaryOperatorNode>();
        binOpNode->Position = GetCurrentPosition();
        binOpNode->Operator = Operator;
        binOpNode->Left = std::move(expression);
        binOpNode->Right = Expect(Expression7());
        expression = std::move(binOpNode);

        continue;
      }

      break;
    }

    return rule.Accept(std::move(expression));
  }

  std::unique_ptr<ExpressionNode> Expression7()
  {
    DescentInfo rule("Expression7");

    std::unique_ptr<UnaryOperatorNode> firstOp;
    std::unique_ptr<UnaryOperatorNode> *unaryOp = nullptr;

    Token Operator;

    auto AssignUnary = [this, &Operator, &unaryOp, &firstOp]() {
      std::unique_ptr<UnaryOperatorNode> unaryOpNode = std::make_unique<UnaryOperatorNode>();
      unaryOpNode->Position = GetCurrentPosition();
      unaryOpNode->Operator = Operator;

      if (unaryOp)
      {
        (*unaryOp)->Right = std::move(unaryOpNode);
        unaryOp = (std::unique_ptr<UnaryOperatorNode> *)&(*unaryOp)->Right;
      }
      else
      {
        firstOp = std::move(unaryOpNode);
        unaryOp = &firstOp;
      }
    };

    for (;;)
    {
      if (Accept(Token::Type::Minus, Operator))
      {
        AssignUnary();
        continue;
      }

      if (Accept(Token::Type::Not, Operator))
      {
        AssignUnary();
        continue;
      }

      if (Accept(Token::Type::Length, Operator))
      {
        AssignUnary();
        continue;
      }

      break;
    }

    auto expression = Expression8();

    if (unaryOp)
    {
      (*unaryOp)->Right = std::move(expression);
      expression = std::move(firstOp);
    }

    return rule.Accept(std::move(expression));
  }

  std::unique_ptr<ExpressionNode> Expression8()
  {
    DescentInfo rule("Expression8");

    auto expression = Expression9();
    if (!(expression))
      return nullptr;

    Token Operator;

    for (;;)
    {
      if (Accept(Token::Type::Exponent, Operator))
      {
        std::unique_ptr<BinaryOperatorNode> binOpNode = std::make_unique<BinaryOperatorNode>();
        binOpNode->Position = GetCurrentPosition();
        binOpNode->Operator = Operator;
        binOpNode->Left = std::move(expression);
        binOpNode->Right = Expect(Expression9());
        expression = std::move(binOpNode);

        continue;
      }

      break;
    }

    return rule.Accept(std::move(expression));
  }

  std::unique_ptr<ExpressionNode> Expression9()
  {
    DescentInfo rule("Expression9");

    std::unique_ptr<ExpressionNode> expressionNode;
    auto value = Value();
    if (value)
    {
      expressionNode = std::move(value);
    }
    else
    {
      auto function = FunctionExpression();
      if (function)
      {
        expressionNode = std::move(function);
      }
      else
      {
        auto functionCall = PrefixExpression();
        if (functionCall)
        {
          expressionNode = std::move(functionCall);
        }
        else
        {
          auto table = Table();
          if (table)
          {
            expressionNode = std::move(table);
          }
          else
          {
            return nullptr;
          }
        }
      }
    }

    return rule.Accept(std::move(expressionNode));
  }

  std::unique_ptr<ExpressionNode> Value()
  {
    DescentInfo rule("Expression");

    std::unique_ptr<ValueNode> valueNode = std::make_unique<ValueNode>();
    Token valueToken;

    if (Accept(Token::Type::Nil, valueNode->Expression) ||
        Accept(Token::Type::False, valueNode->Expression) ||
        Accept(Token::Type::True, valueNode->Expression) ||
        Accept(Token::Type::IntegerLiteral, valueNode->Expression) ||
        Accept(Token::Type::FloatLiteral, valueNode->Expression) ||
        Accept(Token::Type::StringLiteral, valueNode->Expression) ||
        Accept(Token::Type::VariableDot, valueNode->Expression))
    {
      valueNode->Position = GetCurrentPosition();
      return rule.Accept(std::move(valueNode));
    }

    return nullptr;
  }

  std::unique_ptr<FunctionExpressionNode> FunctionExpression()
  {
    DescentInfo rule("Function");

    std::unique_ptr<FunctionExpressionNode> functionNode = std::make_unique<FunctionExpressionNode>();
    functionNode->Position = GetCurrentPosition();

    if (!Accept(Token::Type::Function))
      return nullptr;

    functionNode->Function = std::make_unique<FunctionNode>();

    FunctionBody(functionNode->Function);

    return rule.Accept(std::move(functionNode));
  }

  void FunctionBody(std::unique_ptr<FunctionNode> &functionNode)
  {
    DescentInfo rule("FunctionBody");

    bool validBody = true;
    functionNode->Position = GetCurrentPosition();

    Expect(Token::Type::OpenParentheses);

    ParameterList(functionNode);

    Expect(Token::Type::CloseParentheses);

    functionNode->Block = Expect(Chunk());

    Expect(Token::Type::End);
    functionNode->Block->End->Position = GetCurrentPosition();

    rule.Accept();
  }

  bool ParameterList(std::unique_ptr<FunctionNode> &functionNode)
  {
    DescentInfo rule("ParameterList");

    //If we get names for parameters
    Token identifier;
    if (Accept(Token::Type::Identifier, identifier))
    {
      functionNode->ParameterList.push_back(identifier);

      for (;;)
      {
        //If a comma is found, it can either be a name or a ...
        if (Accept(Token::Type::Comma))
        {
          //If its a name, there may be more names, so save it and continue
          if (Accept(Token::Type::Identifier, identifier))
          {
            functionNode->ParameterList.push_back(identifier);
            continue;
          }

          //If it is a ..., it MUST be the end of the parameter list
          if (Accept(Token::Type::VariableDot, identifier))
          {
            functionNode->ParameterList.push_back(identifier);
          }
        }

        break;
      }
    }
    else if (Accept(Token::Type::VariableDot, identifier))
    {
      functionNode->ParameterList.push_back(identifier);
    }
    
    return rule.Accept();
  }

  bool IdentifierList(std::vector<Token> &identifiers)
  {
    DescentInfo rule("IdentifierList");

    Token identifier;
    if (!Accept(Token::Type::Identifier, identifier))
      return false;

    identifiers.push_back(identifier);

    //Get more expressions
    if (Accept(Token::Type::Comma))
    {
      for (;;)
      {
        Expect(Token::Type::Identifier, identifier);
        identifiers.push_back(identifier);

        if (!Accept(Token::Type::Comma))
          break;
      }
    }

    return rule.Accept();
  }

  bool Invalid_Variable(std::unique_ptr<VariableStatementNode> &VarExp)
  {
    if (VarExp->Suffix && VarExp->Suffix->Index == nullptr)
    {
      return true;
    }

    return false;
  }

  std::unique_ptr<PrefixExpressionNode> PrefixExpression()
  {
    DescentInfo rule("PrefixExpression");

    std::unique_ptr<PrefixExpressionNode> prefixNode = std::make_unique<PrefixExpressionNode>();
    prefixNode->Position = GetCurrentPosition();

    prefixNode->LeftVar = VariableStatement();
    if (!prefixNode->LeftVar)
      return nullptr;

    //If the variable should be a function call
    if (Invalid_Variable(prefixNode->LeftVar))
    {
      for (auto &&node : prefixNode->LeftVar->Suffix->CallNodes)
      {
        prefixNode->RightCalls.push_back(std::move(node));
      }
      prefixNode->LeftVar->Suffix->CallNodes.clear();
    }

    //Grab call nodes - nameAndArgs*
    for (;;)
    {
      //Attempt to find a call node
      std::unique_ptr<CallNode> callNode = Call();
      if (callNode)
      {
        //If found, add it to the list
        prefixNode->RightCalls.push_back(std::move(callNode));
        continue;
      }

      break;
    }

    return rule.Accept(std::move(prefixNode));
  }

  /*
  std::unique_ptr<CallableNode> VarOrExp()
  {
    DescentInfo rule("VarOrExp");

    std::unique_ptr<CallableNode> callableNode = nullptr;

    auto var = VariableStatement();
    if (var)
    {
      std::unique_ptr<CallableVariableNode> varNode = std::make_unique<CallableVariableNode>();
      varNode->Variable = std::move(var);

      callableNode = std::move(varNode);
    }
    else if (Accept(Token::Type::OpenParentheses))
    {
      std::unique_ptr<CallableExpressionNode> expNode = std::make_unique<CallableExpressionNode>();
      expNode->Expression = Expect(Expression());
      Expect(Token::Type::CloseParentheses);

      callableNode = std::move(expNode);
    }

    return rule.Accept(std::move(callableNode));
  }
  */

  std::unique_ptr<FunctionCallNode> FunctionCall(std::unique_ptr<FunctionCallNode> &functionCallNode)
  {
    DescentInfo rule("FunctionCall");

    std::unique_ptr<CallNode> callNode = Expect(Call());
    functionCallNode->Calls.push_back(std::move(callNode));

    //Grab call nodes - nameAndArgs+
    for (;;)
    {
      //Attempt to find a call node
      callNode = Call();
      if (callNode)
      {
        //If found, add it to the list
        functionCallNode->Calls.push_back(std::move(callNode));
        continue;
      }

      break;
    }

    return rule.Accept(std::move(functionCallNode));
  }

  std::unique_ptr<IfNode> Else()
  {
    DescentInfo rule("Else");

    if (Accept(Token::Type::Elseif))
    {
      std::unique_ptr<IfNode> elseNode = std::make_unique<IfNode>();
      elseNode->Position = GetCurrentPosition();

      elseNode->Condition = Expect(Expression());

      Expect(Token::Type::Then);

      elseNode->Block = Expect(Chunk());

      elseNode->Else = Else();

      if (elseNode->Else != nullptr)
      {
        elseNode->Block->End->Position = elseNode->Else->Position;
      }

      return rule.Accept(std::move(elseNode));
    }

    if (Accept(Token::Type::Else))
    {
      std::unique_ptr<IfNode> elseNode = std::make_unique<IfNode>();
      elseNode->Position = GetCurrentPosition();

      elseNode->Block = Expect(Chunk());
      
      return rule.Accept(std::move(elseNode));
    }

    return nullptr;
  }
};


std::unique_ptr<AbstractNode> RecognizeTokens(std::vector<Token> &tokens, std::vector<ParsingException> *error, bool throwException)
{
  RecursiveParser parser(tokens);
  parser.throwException = throwException;

  std::unique_ptr<AbstractNode> ast = std::move(parser.Start());

  if (error != nullptr)
  {
    *error = parser.errors;
  }

  return std::move(ast);
}
std::vector<NodePrinter*> NodePrinter::ActiveNodes;

class PrintVisitor : public Visitor
{
public:
  virtual VisitResult Visit(AbstractNode* node)
  {
    NodePrinter printer;
    printer << "AbstractNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BlockNode* node)
  {
    NodePrinter printer;
    printer << "BlockNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(StatementNode* node)
  {
    NodePrinter printer;
    printer << "StatementNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }


  virtual VisitResult Visit(TypeNode* node)
  {
    NodePrinter printer;
    printer << "TypeNode(" << node->Name << ")";
    
    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(AssignmentNode* node)
  {
    NodePrinter printer;
    printer << "AssignmentNode [" << node->Operator << "](";

    for (auto &token : node->Names)
    {
      printer << token << " ";
    }

    printer << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableNode* node)
  {
    NodePrinter printer;
    printer << "VariableNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableStatementNode* node)
  {
    NodePrinter printer;
    printer << "VariableStatementNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IdentifiedVariableNode* node)
  {
    NodePrinter printer;
    printer << "IdentifiedVariableNode(" << node->Name << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionVariableNode* node)
  {
    NodePrinter printer;
    printer << "ExpressionVariableNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableSuffixNode* node)
  {
    NodePrinter printer;
    printer << "VariableSuffixNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(CallNode* node)
  {
    NodePrinter printer;
    printer << "CallNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(MemberCallNode* node)
  {
    NodePrinter printer;
    printer << "MemberCallNode(" << node->Name << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ArgumentNode* node)
  {
    NodePrinter printer;
    printer << "ArgumentNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionArgumentNode* node)
  {
    NodePrinter printer;
    printer << "ExpressionArgumentNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(TableArgumentNode* node)
  {
    NodePrinter printer;
    printer << "TableArgumentNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(StringArgumentNode* node)
  {
    NodePrinter printer;
    printer << "StringArgumentNode(" << node->String << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }


  virtual VisitResult Visit(IndexNode* node)
  {
    NodePrinter printer;
    printer << "IndexNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IdentifiedIndexNode* node)
  {
    NodePrinter printer;
    printer << "IdentifiedIndexNode(" << node->Name << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionIndexNode* node)
  {
    NodePrinter printer;
    printer << "ExpressionIndexNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }


  virtual VisitResult Visit(BreakNode* node)
  {
    NodePrinter printer;
    printer << "BreakNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ReturnNode* node)
  {
    NodePrinter printer;
    printer << "ReturnNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionNode* node)
  {
    NodePrinter printer;
    printer << "ExpressionNode(" << node->Expression << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ValueNode* node)
  {
    NodePrinter printer;
    printer << "ValueNode(" << node->Expression << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(TableNode* node)
  {
    NodePrinter printer;
    printer << "TableNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionExpressionNode* node)
  {
    NodePrinter printer;
    printer << "FunctionExpressionNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionCallNode* node)
  {
    NodePrinter printer;
    printer << "FunctionCallNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(PrefixExpressionNode* node)
  {
    NodePrinter printer;
    printer << "PrefixExpressionNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(UnaryOperatorNode* node)
  {
    NodePrinter printer;
    printer << "UnaryOperatorNode(" << node->Operator << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BinaryOperatorNode* node)
  {
    NodePrinter printer;
    printer << "BinaryOperatorNode(" << node->Operator << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionNode* node)
  {
    NodePrinter printer;
    printer << "FunctionNode";

    for (auto &token : node->ParameterList)
    {
      printer << " | " << token;
    }

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionNameNode* node)
  {
    NodePrinter printer;
    printer << "FunctionNameNode(" << node->Name << "," << node->IsMemberFunc << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(WhileNode* node)
  {
    NodePrinter printer;
    printer << "WhileNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IfNode* node)
  {
    NodePrinter printer;
    printer << "IfNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ForNode* node)
  {
    NodePrinter printer;
    printer << "ForNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(GenericForNode* node)
  {
    NodePrinter printer;
    printer << "GenericForNode";

    for (auto &token : node->Names)
    {
      printer << " | " << token;
    }

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(NumericForNode* node)
  {
    NodePrinter printer;
    printer << "NumericForNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(RepeatNode* node)
  {
    NodePrinter printer;
    printer << "RepeatNode";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(LocalVariableNode* node)
  {
    NodePrinter printer;
    printer << "LocalVariableNode";

    for (auto &token : node->Names)
    {
      printer << " | " << token;
    }

    node->Walk(this, false);

    return VisitResult::Stop;
  }
};

void PrintTree(AbstractNode* node)
{
  PrintVisitor visitor;

  node->Walk(&visitor);
}



/*
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
EXPARAMENT
*/

//void (\w*)::(\w*).*\r\n{\r\n.*if.*\r\n.*return;.*(\r\n)?(\r\n)?.*::.*
//  virtual VisitResult Visit\(\1* node\)\n{\n

// this -> node
// Walk(visitor) -> Walk(this)
// }\r\n\r\nvir  -> \nreturn VisitResult::Stop;\n}\r\n\r\nvir         (DONT FORGET LAST ONE)

#include <stdio.h>
#include <stdarg.h>

class GenerateVisitor : public Visitor
{
public:
  virtual VisitResult Visit(StatementNode* node)
  {
    return VisitResult::Continue;
  }

  virtual VisitResult Visit(BlockNode* node)
  {

    for (auto &node : node->Statements)
    {
      node->Walk(this);
      my_log("\n");
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(TypeNode* node)
  {

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(AssignmentNode* node)
  {
    for (auto &node : node->LeftVariables)
    {
      node->Walk(this);
    }

    my_log(" %s ", node->Operator.str().c_str());

    for (auto &node : node->RightExpressions)
    {
      node->Walk(this);
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableStatementNode* node)
  {

    node->Variable->Walk(this);

    if (node->Suffix)
      node->Suffix->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IdentifiedVariableNode* node)
  {
    my_log("%s", node->Name.str().c_str());

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionVariableNode* node)
  {

    node->Expression->Walk(this);

    if (node->Suffix)
      node->Suffix->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableSuffixNode* node)
  {

    if (node->LeftSuffix)
      node->LeftSuffix->Walk(this);

    for (auto &node : node->CallNodes)
    {
      node->Walk(this);
    }

    if (node->Index)
      node->Index->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(CallNode* node)
  {
    my_log("(");
    if(node->Argument)
      node->Argument->Walk(this);
    my_log(")");

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(MemberCallNode* node)
  {
    my_log(":%s", node->Name.str().c_str());

    my_log("(");
    if(node->Argument)
      node->Argument->Walk(this);
    my_log(")");

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ArgumentNode* node)
  {

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionArgumentNode* node)
  {
    if (node->ExpressionList.size() > 0)
    {
      node->ExpressionList[0]->Walk(this);

      for (unsigned i = 1; i < node->ExpressionList.size(); ++i)
      {
        my_log(",");
        node->ExpressionList[i]->Walk(this);
      }
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(TableArgumentNode* node)
  {

    node->Table->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(StringArgumentNode* node)
  {
    my_log("%s", node->String.str().c_str());
    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IndexNode* node)
  {

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IdentifiedIndexNode* node)
  {
    my_log(".%s", node->Name.str().c_str());
    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionIndexNode* node)
  {
    my_log("[");
    node->Expression->Walk(this);
    my_log("]");

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BreakNode* node)
  {
    my_log("break");
    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ReturnNode* node)
  {
    my_log("return ");
    for (auto &node : node->ReturnValues)
    {
      node->Walk(this);
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionNode* node)
  {

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ValueNode* node)
  {
    my_log("%s", node->Expression.str().c_str());
    return VisitResult::Stop;
  }

  virtual VisitResult Visit(TableNode* node)
  {
    my_log("{");

    //@TODO Fix table printing

    /*
    for (auto &node : node->Indicies)
    {
      node->Walk(this);
    }

    for (auto &node : node->Values)
    {
      node->Walk(this);
    }
    */

    my_log("}");

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionExpressionNode* node)
  {

    node->Function->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionCallNode* node)
  {

    node->Variable->Walk(this);

    for (auto &node : node->Calls)
    {
      node->Walk(this);
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(PrefixExpressionNode* node)
  {
    my_log("(");
    node->LeftVar->Walk(this);
    my_log(")");

    for (auto &node : node->RightCalls)
    {
      node->Walk(this);
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(UnaryOperatorNode* node)
  {
    my_log("%s", node->Operator.str().c_str());
    node->Right->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BinaryOperatorNode* node)
  {
    node->Left->Walk(this);

    my_log(" %s ", node->Operator.str().c_str());

    node->Right->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionNode* node)
  {
    my_log("function");

    if (node->Name.size() > 0)
    {
      my_log(" %s", node->Name[0]->Name.str().c_str());
      for (unsigned i = 1; i < node->Name.size(); ++i)
      {
        node->Name[i]->Walk(this);
      }
    }

    my_log("(");
    if (node->ParameterList.size() > 0)
    {
      my_log("%s", node->ParameterList[0].str().c_str());

      for (unsigned i = 1; i < node->ParameterList.size(); ++i)
      {
        my_log(", %s", node->ParameterList[i].str().c_str());
      }
    }
    my_log(")");

    my_log("\n");
    node->Block->Walk(this);

    my_log("end\n");

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionNameNode* node)
  {
    my_log("%s%s", (node->IsMemberFunc ? ":" : "."), node->Name.str().c_str());
    return VisitResult::Stop;
  }

  virtual VisitResult Visit(WhileNode* node)
  {

    node->Condition->Walk(this);
    node->Block->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(RepeatNode* node)
  {

    node->Block->Walk(this);
    node->Condition->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IfNode* node)
  {
    if (node->Condition)
    {
      my_log("if ");
      node->Condition->Walk(this);
    }

    my_log(" then\n");

    node->Block->Walk(this);

    if (node->Else)
    {
      my_log("else");
      node->Else->Walk(this);
    }

    my_log("end\n");

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ForNode* node)
  {

    node->Block->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(NumericForNode* node)
  {

    node->Var->Walk(this);
    node->Limit->Walk(this);

    if (node->Step)
      node->Step->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(GenericForNode* node)
  {

    for (auto &node : node->ExpressionList)
    {
      node->Walk(this);
    }

    node->Block->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(LocalVariableNode* node)
  {
    my_log("local %s", node->Names[0].str().c_str());
    
    for (unsigned i = 1; i < node->Names.size(); ++i)
    {
      my_log(",%s", node->Names[i].str().c_str());
    }

    my_log(" = ");

    for (auto &node : node->ExpressionList)
    {
      node->Walk(this);
    }

    return VisitResult::Stop;
  }
};

void GenerateTree(AbstractNode* node)
{
  GenerateVisitor visitor;

  node->Walk(&visitor);
}