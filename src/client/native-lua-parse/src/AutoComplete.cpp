#include "AutoComplete.h"
#include "AST_Nodes.h"
#include <sstream>
#include <stack>
#include <functional>

const char* keywords[] =
{
  // The TOKEN macro is used like TOKEN(Class, "class")
#define TOKEN(Name, Value) Value,
#include "Tokens_Keyword.inl"
#undef TOKEN
};

class LocationPrinter : public Visitor
{
public:
  DocumentPosition lastPosition;

  virtual VisitResult Visit(AbstractNode* node)
  {
    NodePrinter printer;
    printer << "AbstractNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BlockNode* node)
  {
    NodePrinter printer;
    printer << "BlockNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(StatementNode* node)
  {
    NodePrinter printer;
    printer << "StatementNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(TypeNode* node)
  {
    NodePrinter printer;
    printer << "TypeNode(" << node->Name << ")";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

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
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    NodePrinter typePrinter;
    typePrinter << "Types(";

    for (auto &var : node->LeftVariables)
    {
      typePrinter << var->SEM_ResolvedSymbol << " ";
    }

    typePrinter << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableNode* node)
  {
    NodePrinter printer;
    printer << "VariableNode" << " - " << node->SEM_ResolvedSymbol;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableStatementNode* node)
  {
    NodePrinter printer;
    printer << "VariableStatementNode" << " - " << node->SEM_ResolvedSymbol;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IdentifiedVariableNode* node)
  {
    NodePrinter printer;
    printer << "IdentifiedVariableNode(" << node->Name << ")" << " - " << node->SEM_ResolvedSymbol;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionVariableNode* node)
  {
    NodePrinter printer;
    printer << "ExpressionVariableNode" << " - " << node->SEM_ResolvedSymbol;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableSuffixNode* node)
  {
    NodePrinter printer;
    printer << "VariableSuffixNode" << " - " << node->SEM_ResolvedSymbol;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(CallNode* node)
  {
    NodePrinter printer;
    printer << "CallNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(MemberCallNode* node)
  {
    NodePrinter printer;
    printer << "MemberCallNode(" << node->Name << ")";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ArgumentNode* node)
  {
    NodePrinter printer;
    printer << "ArgumentNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionArgumentNode* node)
  {
    NodePrinter printer;
    printer << "ExpressionArgumentNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(TableArgumentNode* node)
  {
    NodePrinter printer;
    printer << "TableArgumentNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(StringArgumentNode* node)
  {
    NodePrinter printer;
    printer << "StringArgumentNode(" << node->String << ")";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }


  virtual VisitResult Visit(IndexNode* node)
  {
    NodePrinter printer;
    printer << "IndexNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IdentifiedIndexNode* node)
  {
    NodePrinter printer;
    printer << "IdentifiedIndexNode(" << node->Name << ")";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionIndexNode* node)
  {
    NodePrinter printer;
    printer << "ExpressionIndexNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }


  virtual VisitResult Visit(BreakNode* node)
  {
    NodePrinter printer;
    printer << "BreakNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ReturnNode* node)
  {
    NodePrinter printer;
    printer << "ReturnNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionNode* node)
  {
    NodePrinter printer;
    printer << "ExpressionNode(" << node->SEM_ResolvedType << ")";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ValueNode* node)
  {
    NodePrinter printer;
    printer << "ValueNode(" << node->Expression << ")" << " - " << node->SEM_ResolvedType << " : " << node->SEM_Value;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(TableNode* node)
  {
    NodePrinter printer;
    printer << "TableNode" << " - " << node->SEM_ResolvedType;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionExpressionNode* node)
  {
    NodePrinter printer;
    printer << "FunctionExpressionNode" << " - " << node->SEM_ResolvedType;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionCallNode* node)
  {
    NodePrinter printer;
    printer << "FunctionCallNode" << " - " << node->SEM_ResolvedType;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(PrefixExpressionNode* node)
  {
    NodePrinter printer;
    printer << "PrefixExpressionNode" << " - " << node->SEM_ResolvedType << " : " << node->SEM_Value;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(UnaryOperatorNode* node)
  {
    NodePrinter printer;
    printer << "UnaryOperatorNode(" << node->Operator << ")" << " - " << node->SEM_ResolvedType << " : " << node->SEM_Value;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BinaryOperatorNode* node)
  {
    NodePrinter printer;
    printer << "BinaryOperatorNode(" << node->Operator << ")" << " - " << node->SEM_ResolvedType << " : " << node->SEM_Value;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionNode* node)
  {
    NodePrinter printer;
    printer << "FunctionNode" << " - " << node->SYM_ReturnType;
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

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
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(WhileNode* node)
  {
    NodePrinter printer;
    printer << "WhileNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IfNode* node)
  {
    NodePrinter printer;
    printer << "IfNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ForNode* node)
  {
    NodePrinter printer;
    printer << "ForNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

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
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(NumericForNode* node)
  {
    NodePrinter printer;
    printer << "NumericForNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(RepeatNode* node)
  {
    NodePrinter printer;
    printer << "RepeatNode";
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

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

    printer << " - ";

    for (auto &expr : node->ExpressionList)
    {
      printer << expr->SEM_ResolvedType->GetResolvedType() << " ";
    }
    printer << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    node->Walk(this, false);

    return VisitResult::Stop;
  }
};

class LocateNode : public Visitor
{
public:
  LocateNode(DocumentPosition position)
    :position(position)
  {}

  DocumentPosition position;
  AbstractNode *foundNode = nullptr;

  virtual VisitResult Visit(AbstractNode* node)
  {
    if (foundNode == nullptr)
      foundNode = node;

    if (node->Position == position || (node->Position < position && (foundNode->Position == node->Position || foundNode->Position < node->Position)))
      foundNode = node;

    node->Walk(this, false);

    return VisitResult::Stop;
  }
};

class GenerateAutoComplete : public Visitor
{
public:
  GenerateAutoComplete(std::vector<AutoCompleteEntry> &entries)
    :entries(entries)
  {}

  std::vector<AutoCompleteEntry> &entries;
  Library *lib;
  Token::Type::Enum foundToken;

  void AddEntry(AutoCompleteEntry e)
  {
    for (AutoCompleteEntry &entry : entries)
    {
      if (entry.name == e.name)
        return;
    }

    if (foundToken == Token::Type::Colon && e.entryKind != CompletionItemKind::Method)
      return;

    entries.push_back(e);
  }

  Variable *GetVariable(AbstractNode *node, std::string const &name)
  {
    //Add all locals this should know about
    std::vector<BlockNode *> parentStack;
    AbstractNode *currentNode = node;
    while (currentNode != nullptr)
    {
      BlockNode *block = dynamic_cast<BlockNode *>(currentNode);
      if (block)
      {
        parentStack.push_back(block);
      }

      currentNode = currentNode->Parent;
    }

    //Traverse local stack in reverse for variable
    for (int i = parentStack.size() - 1; i >= 0; i--)
    {
      BlockNode *block = parentStack[i];
      for (Symbol *sym : block->localSymbols)
      {
        Variable *var = dynamic_cast<Variable *>(sym);
        if (var && var->Name == name)
          return var;
      }
    }

    //Traverse global stack for variable
    for (Variable *var : lib->globalTable->Members)
    {
      if (var->Name == name)
      {
        return dynamic_cast<Variable *>(var);
      }
    }

    return nullptr;
  }

  CompletionItemKind ResolveEntryKind(Variable *sym)
  {
    if (sym == nullptr)
      return CompletionItemKind::Text;

    Type *type = sym->GetResolvedType();
    if (type && (type->Name == "Table" || type->Name == "Predictive"))
    {
      return CompletionItemKind::Module;
    }

    VariableType varType = sym->ValueType;
    if (varType == VariableType::Default)
    {
      varType = sym->Type;
    }

    switch (varType)
    {
    case VariableType::Method:
      return CompletionItemKind::Method;

    case VariableType::Field:
      return CompletionItemKind::Field;

    case VariableType::TableValue:
      return CompletionItemKind::Field;

    case VariableType::Function:
      return CompletionItemKind::Function;
    }

    //Default to text
    return CompletionItemKind::Text;
  }

  void AddMembers(Symbol *sym)
  {
    if (sym == nullptr || sym->GetResolvedType() == nullptr)
      return;

    for (Variable *var : sym->GetResolvedType()->Members)
    {
      TableData *tableData = dynamic_cast<TableData *>(var);
      if (tableData)
      {
        if (tableData->Index.Type == ExpressionType::String && !tableData->Index.Data.String.empty())
        {
          AutoCompleteEntry entry;
          entry.name = tableData->Index.Data.String;
          entry.entryKind = ResolveEntryKind(tableData);

          AddEntry(entry);
        }
      }
      else
      {
        if (!var->Name.empty())
        {
          AutoCompleteEntry entry;
          entry.name = var->Name;
          entry.entryKind = ResolveEntryKind(var);

          AddEntry(entry);
        }
      }
    }
  }

  //Autocomplete inside of function names
  virtual VisitResult Visit(FunctionNameNode *node)
  {
    NodePrinter printer;
    printer << "FunctionNameNode" << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    FunctionNode *func = dynamic_cast<FunctionNode *>(node->Parent);
    if (func)
    {
      //First, get our position in the array
      unsigned arr_pos = 0;
      for (int i = 0; i < func->Name.size(); ++i)
      {
        arr_pos = i;
        if (func->Name[i].get() == node)
          break;
      }
      
      if (arr_pos >= func->Name.size())
        arr_pos = func->Name.size() - 1;

      Variable *functionVar = nullptr;
      for (int i = 0; i <= arr_pos; ++i)
      {
        FunctionNameNode *nameNode = func->Name[i].get();
        std::string functionName = nameNode->Name.str();

        if (i >= func->Name.size() - 1)
          break;

        if (i == 0)
        {
          functionVar = GetVariable(node, functionName);
        }
        else
        {
          if (functionVar)
          {
            //Save and reset the function var
            Variable *temp = functionVar;
            functionVar = nullptr;

            for (Variable *var : temp->GetResolvedType()->Members)
            {
              if (var->Type == VariableType::TableValue)
              {
                TableData *data = (TableData *)var;

                //If the index is a string, and matches the node
                if (data->Index == functionName)
                {
                  functionVar = data;
                }
              }
            }
          }
        }
      }

      //If we found the correct variable, add its members
      AddMembers(functionVar);
    }

    return VisitResult::Stop;
  }

  //Autocomplete for : syntax
  virtual VisitResult Visit(MemberCallNode* node)
  {
    NodePrinter printer;
    printer << "MemberCallNode" << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    FunctionCallNode *callNode = dynamic_cast<FunctionCallNode *>(node->Parent);
    if (callNode)
    {
      if (callNode->Variable->SEM_Variable)
        AddMembers(callNode->Variable->SEM_Variable);
      else if (callNode->Variable->SEM_ResolvedSymbol)
        AddMembers(callNode->Variable->SEM_ResolvedSymbol);
      else
        Visit((AbstractNode *)node);
    }

    return VisitResult::Stop;
  }

  //virtual VisitResult Visit(IdentifiedVariableNode* node)
  //{
  //  NodePrinter printer;
  //  printer << "IdentifiedVariableNode" << "   [" << node->Position.Line << "," << node->Position.Character << "]";
  //  printer << "-> " << node->SEM_Variable << " -- " << node->SEM_ResolvedSymbol;
  //
  //  //Try to add by variable, then by resolved type
  //  if (node->SEM_Variable)
  //    AddMembers(node->SEM_Variable);
  //  else if (node->SEM_ResolvedSymbol)
  //    AddMembers(node->SEM_ResolvedSymbol);
  //  else
  //    Visit((AbstractNode *)node);
  //
  //  return VisitResult::Stop;
  //}

  //For IdentifiedIndexNode, it will be in the form var(.arg)*.last
  virtual VisitResult Visit(IdentifiedIndexNode* node)
  {
    NodePrinter printer;
    printer << "IdentifiedIndexNode" << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    //First, attempt to find the symbol to the left of us
    VariableSuffixNode *vs = dynamic_cast<VariableSuffixNode *>(node->Parent);
    if (vs)
    {
      //If the suffix has a valid symbol to the left
      if (vs->LeftSuffix && vs->LeftSuffix->Index)
      {
        //If the index has a variable, add the members
        if (vs->LeftSuffix->Index->SEM_Variable)
        {
          AddMembers(vs->LeftSuffix->Index->SEM_Variable);
          return VisitResult::Stop;
        }
        else if (vs->SEM_Variable)
        {
          AddMembers(vs->SEM_Variable);
          return VisitResult::Stop;
        }
        else
        {
          AddMembers(vs->SEM_ResolvedSymbol);
          return VisitResult::Stop;
        }
      }

      //If the suffix has a return type, use that
      if (vs->SEM_ResolvedSymbol && vs->SEM_ResolvedSymbol->GetResolvedType() && vs->SEM_ResolvedSymbol->GetResolvedType()->ReturnType)
      {
        AddMembers(vs->SEM_ResolvedSymbol->GetResolvedType()->ReturnType);
        return VisitResult::Stop;
      }
      
      //Otherwise, try to get the parent
      if (dynamic_cast<VariableStatementNode *>(vs->Parent))
      {
        AddMembers(dynamic_cast<VariableStatementNode *>(vs->Parent)->Variable->SEM_Variable);
        return VisitResult::Stop;
      }
    }

    

    return VisitResult::Stop;
  }

  //Base case, just get locals and globals
  virtual VisitResult Visit(AbstractNode* node)
  {
    NodePrinter printer;
    printer << "AbstractNode" << "   [" << node->Position.Line << "," << node->Position.Character << "]";

    //Add all locals this should know about
    AbstractNode *currentNode = node;
    while (currentNode != nullptr)
    {
      BlockNode *block = dynamic_cast<BlockNode *>(currentNode);
      if (block)
      {
        for (Symbol *sym : block->localSymbols)
        {
          AutoCompleteEntry entry;
          entry.name = sym->Name;
          entry.entryKind = ResolveEntryKind(dynamic_cast<Variable *>(sym));

          AddEntry(entry);
        }
      }

      currentNode = currentNode->Parent;
    }

    //Add all globals as well
    for (Symbol *sym : lib->Globals)
    {
      AutoCompleteEntry entry;
      entry.name = sym->Name;
      entry.entryKind = ResolveEntryKind(dynamic_cast<Variable *>(sym));

      AddEntry(entry);
    }

    //Add everything from the global table
    AddMembers(lib->globalTable);

    //Finally, add in the keywords
    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i)
    {
      AutoCompleteEntry entry;
      entry.name = std::string(keywords[i]);
      entry.entryKind = CompletionItemKind::Keyword;

      AddEntry(entry);
    }

    return VisitResult::Stop;
  }

  //Used to trigger variable lookup for index nodes
  virtual VisitResult Visit(VariableSuffixNode* node)
  {
    node->Walk(this, false);
    return VisitResult::Stop;
  }
};

void ResolveAutocomplete(AbstractNode *ast, int lineNumber, int charNumber, std::vector<AutoCompleteEntry> &output, Library *lib, std::vector<Token> &tokens)
{
  //Get specific token from line + char
  Token::Type::Enum currentTokenType = Token::Type::Invalid;
  DocumentPosition pos(lineNumber, charNumber);

  for (int i = 0; i < tokens.size(); ++i)
  {
    if (tokens[i].Position.Line == lineNumber && tokens[i].Position.Character == charNumber)
      currentTokenType = tokens[i].EnumTokenType;
  }
  my_log("GOT %i NOT %i\n", currentTokenType, Token::Type::Dot);

  LocateNode visitor(DocumentPosition(lineNumber, charNumber));
  ast->Walk(&visitor);

  LocationPrinter print;
  ast->Walk(&print);
  my_log("\n");
  //visitor.foundNode->Walk(&print);
  //my_log("\n");

  GenerateAutoComplete autoCompleteVisitor(output);
  autoCompleteVisitor.lib = lib;
  autoCompleteVisitor.foundToken = currentTokenType;

  //if (currentTokenType == Token::Type::Dot || currentTokenType == Token::Type::Colon)
  visitor.foundNode->Walk(&autoCompleteVisitor);
  //else
  //  autoCompleteVisitor.Visit(visitor.foundNode);
}

//@TODO: Fix the weird bug when deleting a name that has been autocompleted