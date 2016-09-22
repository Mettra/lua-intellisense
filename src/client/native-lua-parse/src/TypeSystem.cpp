#include "TypeSystem.h"
#include "AST_Nodes.h"
#include <sstream>
#include <stack>
#include <functional>
#include <cmath>

static LibraryReference coreLibRef;

Library *CreateCoreLibrary()
{
  Library *lib = new Library();
  lib->currentRef = &coreLibRef;

  coreLibRef.library = lib;

  //Initialize core types
  lib->CreateBaseType("Nil");
  lib->CreateBaseType("Boolean");
  lib->CreateBaseType("Number");
  lib->CreateBaseType("String");
  lib->CreateBaseType("Function");
  lib->CreateBaseType("Userdata");
  lib->CreateBaseType("Thread");
  lib->CreateBaseType("Table");
  lib->CreateBaseType("VariableArgument");

  Type *globalTableType = lib->CreateBlankType("Table");
  Variable *g = lib->CreateVariable("_G", false);
  g->ResolvedType = globalTableType;
  lib->globalTable = g;

  return lib;
}

std::ostream& operator<<(std::ostream& stream, const Symbol* symbol)
{
  if (symbol == nullptr)
  {
    stream << "(nullptr)";
  }
  else
  {
    Type *resolvedType = symbol->GetResolvedType();
    if (resolvedType && resolvedType != symbol)
    {
      stream << resolvedType;

      return stream;
    }

    stream << symbol->Name;
  }

  return stream;
}

std::ostream& operator<<(std::ostream& stream, const Symbol& symbol)
{
  return stream << &symbol;
}

std::ostream& operator<<(std::ostream& stream, const ValueData* data)
{
  if (data->Type == ExpressionType::Boolean)
  {
    stream << (data->Data.Boolean == true ? "true" : "false");
  }

  if (data->Type == ExpressionType::Invalid)
  {
    stream << "INVALID";
  }

  if (data->Type == ExpressionType::Nil)
  {
    stream << "nil";
  }

  if (data->Type == ExpressionType::Number)
  {
    stream << std::to_string(data->Data.Number);
  }

  if (data->Type == ExpressionType::Reference)
  {
    stream << data->Data.Reference;
  }

  if (data->Type == ExpressionType::String)
  {
    stream << data->Data.String;
  }

  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ValueData& symbol)
{
  return stream << &symbol;
}

bool ValueData::operator == (ValueData const &rhs)
{
  if (this->Type == ExpressionType::Boolean)
  {
    return Data.Boolean == rhs.Data.Boolean;
  }

  if (this->Type == ExpressionType::Invalid)
  {
    return false;
  }

  if (this->Type == ExpressionType::Nil)
  {
    return false;
  }

  if (this->Type == ExpressionType::Number)
  {
    return Data.Number == rhs.Data.Number;
  }

  if (this->Type == ExpressionType::Reference)
  {
    return Data.Reference == rhs.Data.Reference;
  }

  if (this->Type == ExpressionType::String)
  {
    return Data.String == rhs.Data.String;
  }

  return false;
}

bool ValueData::operator==(std::string const &rhs)
{
  if (this->Type == ExpressionType::String)
  {
    return Data.String == rhs;
  }

  return false;
}

bool ValueData::operator==(unsigned const &rhs)
{
  if (this->Type == ExpressionType::Number)
  {
    return Data.Number == rhs;
  }

  return false;
}

bool ValueData::operator==(int const &rhs)
{
  if (this->Type == ExpressionType::Number)
  {
    return Data.Number == rhs;
  }

  return false;
}

bool ValueData::operator==(float const &rhs)
{
  if (this->Type == ExpressionType::Number)
  {
    return Data.Number == rhs;
  }

  return false;
}

bool ValueData::operator==(Variable *rhs)
{
  if (this->Type == ExpressionType::Reference)
  {
    return Data.Reference == rhs;
  }

  return false;
}

bool ValueData::operator==(bool rhs)
{
  if (this->Type == ExpressionType::Boolean)
  {
    return Data.Boolean == rhs;
  }

  return false;
}

void ErrorSameName(const std::string& name)
{
  std::stringstream stream;
  stream << "Duplicate symbols with the same name '" << name << "' in the same scope";
  #ifdef __EXCEPTIONS
  throw SemanticException(stream.str());
  #endif
}

void ErrorSymbolNotFound(const std::string& name)
{
  std::stringstream stream;
  stream << "The symbol '" << name << "' was not found";
  #ifdef __EXCEPTIONS
  throw SemanticException(stream.str());
  #endif
}

void AddReference(Library *library, Symbol *sym)
{
  if (library->currentRef)
  {
    sym->ReferenceCount += 1;
    library->currentRef->SymbolReferences[sym] += 1;
  }
}

template<typename T>
T* CreateBlankSymbol(Library *library, const std::string& name, bool isGlobal)
{
  std::unique_ptr<T> type = std::make_unique<T>();

  type->Owner = library;
  type->Name = name;

  T *typePtr = type.get();

  library->TempSymbols.push_back(std::move(type));
  //@TODO cleanup temp symbols via garbage collection

  AddReference(library, typePtr);

  return typePtr;
}

template<typename T>
T* CreateSymbol(Library *library, const std::string& name, bool isGlobal)
{
  //Find a value with the same name? Treat them the same!
  if (isGlobal)
  {
    auto it = library->GlobalsByName.find(name);
    if (it != library->GlobalsByName.end())
    {
      AddReference(library, it->second);

      //@TODO: Handle this properly
      return (T*)it->second;
    }
  }

  std::unique_ptr<T> type = std::make_unique<T>();

  type->Owner = library;
  type->Name = name;

  T *typePtr = type.get();

  library->AllSymbols.push_back(std::move(type));

  if (isGlobal)
  {
    library->Globals.push_back(typePtr);
    library->GlobalsByName.insert(std::make_pair(name, typePtr));
  }

  AddReference(library, typePtr);

  return typePtr;
}

Type *Symbol::GetResolvedType() const
{
  if (this == nullptr)
    return nullptr;

  Type *resolvedType = this->ResolvedType;

  while (resolvedType != nullptr)
  {
    if (resolvedType == resolvedType->ResolvedType)
      break;

    resolvedType = resolvedType->ResolvedType;
  }

  return resolvedType;
}

Type* Library::CreateType(const std::string& name, bool isGlobal)
{
  Type *type = CreateSymbol<Type>(this, name, isGlobal);
  type->ResolvedType = type;

  return type;
}

Type* Library::CreateBaseType(const std::string& name)
{
  Type *type = CreateSymbol<Type>(this, name, false);
  type->ResolvedType = type;

  BaseTypesByName.insert(std::make_pair(name, type));

  return type;
}


Type* Library::CreateBlankType(const std::string& name, bool isGlobal)
{
  Type *type = CreateBlankSymbol<Type>(this, name, isGlobal);
  type->ResolvedType = type;

  return type;
}

Variable* Library::CreateVariable(const std::string& name, bool isGlobal)
{
  Variable *var = CreateSymbol<Variable>(this, name, isGlobal);
  var->Type = VariableType::Default;

  if (isGlobal)
  {
    //AddToGlobalTable(var)
    globalTable->GetResolvedType()->Members.push_back(var);
  }

  return var;
}
Type* Library::CreateMultipleType(std::vector<Type *> const &types, bool isGlobal)
{
  //If there is only one type, treat as single type
  if (types.size() == 1)
  {
    return types[0];
  }

  std::string normalizedName = "MultipleType(";

  bool first = true;
  for (Type * t : types)
  {
    if (!first)
    {
      normalizedName += ", ";
    }

    if (t)
      normalizedName += t->Name;
    else
      normalizedName += "(nullptr)";

    first = false;
  }
  normalizedName += ")";

  Type *type = CreateBlankSymbol<Type>(this, normalizedName, isGlobal);
  type->ResolvedType = type;

  for (Type *t : types)
  {
    type->MultipleTypes.push_back(t);
  }

  return type;
}

Type* Library::AddPossibleType(Type *baseType, Type *newType, bool isGlobal)
{
  if (newType == nullptr)
    return baseType;

  //Add new possible type
  baseType->PossibleTypes.push_back(newType);

  //If there is only one, treat as normal type
  if (baseType->PossibleTypes.size() == 1)
  {
    baseType->ResolvedType = newType;
    return baseType;
  }
  else
  {
    baseType->ResolvedType = baseType;
  }

  //Create new normalized name
  std::string normalizedName = "PossibleType(";
  bool first = true;
  for (Type * t : baseType->PossibleTypes)
  {
    if (!first)
    {
      normalizedName += " OR ";
    }

    normalizedName += t->Name;

    first = false;
  }
  normalizedName += ")";

  baseType->Name = normalizedName;

  return baseType;
}

Type *Library::CreateFunctionType(FunctionNode *function)
{
  std::string normalizedName = "Function(";

  //@TODO add parameters

  normalizedName += ") - ";
  normalizedName += function->SYM_ReturnType->GetResolvedType()->Name;

  Type *functionType = CreateType(normalizedName, false);
  functionType->ReturnType = function->SYM_ReturnType;

  return functionType;
}

void Library::Clean()
{
  for (auto &sym : AllSymbols)
  {
    sym->hasClearedRefs = false;
  }

  for (auto &sym : TempSymbols)
  {
    sym->hasClearedRefs = false;
  }

  //Then cleanup any symbols with no more references
  for (auto &sym : AllSymbols)
  {
    sym->Clean();
  }

  for (auto &sym : TempSymbols)
  {
    sym->Clean();
  }

  //Remove unreferenced global variables
  std::vector<Symbol *> badGlobals;
  for (unsigned i = 0; i < Globals.size(); ++i)
  {
    auto &v = Globals[i];

    if (v->ReferenceCount == 0)
    {
      badGlobals.push_back(v);
      std::swap(Globals.back(), Globals[i]);
      Globals.pop_back();
      i--;
    }
  }

  for (auto &sym : badGlobals)
  {
    GlobalsByName.erase(sym->Name);
  }

  CleanMembers(AllSymbols);
  CleanMembers(TempSymbols);
}

class ResolveTypesVisitor : public Visitor
{
public:
  Library *lib;
  std::stack<FunctionNode *> functionStack;
  std::stack<BlockNode *> blockStack;
  std::vector<Symbol *> parentStack;

  Type *GetBaseType(std::string const &name)
  {
    auto it = lib->BaseTypesByName.find(name);
    if (it == lib->BaseTypesByName.end())
    {
    }
    else
    {
      if (dynamic_cast<Type *>(it->second))
        return (Type *)it->second;
    }

    ErrorSymbolNotFound(name);

    return nullptr;
  }

  Variable *GetVariable(std::string const &name)
  {
    if (name == "_G")
      return lib->globalTable;

    //Traverse stack in reverse for variable
    for (int i = parentStack.size() - 1; i >= 0; i--)
    {
      Symbol *sym = parentStack[i];
      for (Variable *var : sym->Members)
      {
        if (var->Name == name)
          return var;
      }
    }

    //Traverse global stack for variable
    for(Symbol *sym : lib->globalTable->GetResolvedType()->Members)
    {
      if (sym->Name == name && dynamic_cast<Variable *>(sym) != nullptr)
      {
          return dynamic_cast<Variable *>(sym);
      }
    }

    return nullptr;
  }

  //Function takes a type and tries to predict the outcome of a function call on that type.
  //Pair - First argument is if the prediction succeeds, second is the type of the call (null if failed)
  std::pair<bool, Type *> PredictFunctionCall(Type *currentType)
  {
    //If there is a multi-type, we need to search the first for a function call
    if (currentType->MultipleTypes.size() > 0)
    {
      //First, predict calling the first argument
      auto ret = PredictFunctionCall(currentType->MultipleTypes[0]);

      //If successful, discard the other types
      if (ret.first)
      {
        return std::make_pair(true, ret.second);
      }

      //Otherwise, the prediction failed
      return std::make_pair(false, (Type *)nullptr);
    }

    //Otherwise, Search through all possible outcomes for a successful type
    Type *resolvedType = currentType->GetResolvedType();
    if (resolvedType->PossibleTypes.size() > 0)
    {
      std::vector<Type *> types;

      for (Type *type : currentType->PossibleTypes)
      {
        //Try to predict this type
        auto ret = PredictFunctionCall(type);
        if (ret.first)
        {
          types.push_back(type);
        }
      }

      //If just one, return it
      if (types.size() == 1)
      {
        return std::make_pair(true, types[0]);
      }

      //Otherwise, Create a possibility space from the valid predicted types
      if (types.size() > 0)
      {
        Type *newType = lib->CreateBlankType("");

        for (Type *t : types)
        {
          lib->AddPossibleType(newType, t);
        }

        return std::make_pair(true, newType);
      }
    }

    //Lastly, check for a function
    if (currentType->ReturnType)
    {
      return std::make_pair(true, currentType->ReturnType);
    }

    return std::make_pair(false, (Type *)nullptr);
  };

  //Evaluate assignment types
  bool ResolveAssignment = false;
  bool ValidAssignment = true;
  virtual VisitResult Visit(AssignmentNode* node)
  {
    //First evaluate all of the expressions
    for (auto &child : node->RightExpressions)
    {
      child->Walk(this);
    }

    //Grab all types from the expressions
    std::vector<Type *> expressionTypes;
    std::vector<ValueData> expressionData;
    int num_varargs = 0;

    for (auto &expr : node->RightExpressions)
    {
      if (expr->SEM_ResolvedType && expr->SEM_ResolvedType->GetResolvedType() == GetBaseType("VariableArgument"))
        num_varargs += 1;

      expressionTypes.push_back(expr->SEM_ResolvedType);
      expressionData.push_back(expr->SEM_Value);
    }

    //If there are varargs, figure out how many spaces they occupy and fix the vectors
    if (num_varargs > 0)
    {
      int other_args = expressionTypes.size() - num_varargs;
      int num_variables = node->LeftVariables.size();

      int varargs_length = (num_variables - other_args) / num_varargs;

      std::vector<Type *> new_expressionTypes;
      std::vector<ValueData> new_expressionData;

      //For each variable argument, make a predictive type
      for (int i = 0; i < expressionTypes.size(); ++i)
      {
        Type *t = expressionTypes[i];
        if (t && t->GetResolvedType() == GetBaseType("VariableArgument"))
        {
          for (int v = 0; v < varargs_length; ++v)
          {
            Type *predictiveType = lib->CreateBlankType("Predictive");
            predictiveType->Predictive = true;

            new_expressionTypes.push_back(predictiveType);
            new_expressionData.push_back(ValueData());
          }
        }
        else
        {
          new_expressionTypes.push_back(expressionTypes[i]);
          new_expressionData.push_back(expressionData[i]);
        }
      }

      expressionTypes = new_expressionTypes;
      expressionData = new_expressionData;
    }
    
    if (node->Operator.TokenType == Token::Type::Invalid)
    {
      ValidAssignment = false;
    }
    else
    {
      ValidAssignment = true;
    }

    //Then put the necessary types into the variables
    for (int i = 0; i < node->LeftVariables.size(); ++i)
    {
      ResolveAssignment = true;
      node->LeftVariables[i]->Walk(this);
      ResolveAssignment = false;

      if (i < expressionTypes.size())
      {
        node->LeftVariables[i]->SEM_ResolvedSymbol = expressionTypes[i];

        if (node->LeftVariables[i]->SEM_Variable)
        {
          node->LeftVariables[i]->SEM_Variable->Value = expressionData[i];

          //If the variable is predictive, we need to transfer its predictions to the new type
          //if (node->LeftVariables[i]->SEM_Variable->Predictive)
          //{
          //  node->LeftVariables[i]->SEM_Variable->Predictive = false;
          //
          //  for (Variable *var : node->LeftVariables[i]->SEM_Variable->Members)
          //  {
          //    expressionTypes[i]->Members.push_back(var);
          //  }
          //}

          if (node->LeftVariables[i]->SEM_Variable->ResolvedType && node->LeftVariables[i]->SEM_Variable->ResolvedType->Predictive)
          {
            Type *newType = expressionTypes[i];
            Type *prediction = node->LeftVariables[i]->SEM_Variable->ResolvedType;

            prediction->CopyType(newType);
          }
          else
          {
            node->LeftVariables[i]->SEM_Variable->ResolvedType = expressionTypes[i];
          }
        }
      }
      else
      {
        node->LeftVariables[i]->SEM_ResolvedSymbol = GetBaseType("Nil");
      }
    }

    ValidAssignment = true;

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BlockNode* node)
  {
    blockStack.push(node);

    for (auto &child : node->Statements)
    {
      child->Walk(this);
    }

    blockStack.pop();

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableNode* node)
  {


    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableStatementNode* node)
  {
    if(node->Variable)
      node->Variable->Walk(this);

    //Set symbol to the variable's symbol
    node->SEM_ResolvedSymbol = node->Variable->SEM_ResolvedSymbol;
    node->SEM_Variable = node->Variable->SEM_Variable;

    //@TODO Handle suffix changing type
    if (node->Suffix)
    {
      node->Suffix->SEM_ResolvedSymbol = node->SEM_ResolvedSymbol;
      node->Suffix->Walk(this);
      node->SEM_ResolvedSymbol = node->Suffix->SEM_ResolvedSymbol->GetResolvedType();
      node->SEM_Variable = node->Suffix->SEM_Variable;
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IdentifiedVariableNode* node)
  {
    Variable *var = GetVariable(node->Name.str());

    if (var)
    {
      node->SEM_ResolvedSymbol = var->ResolvedType->GetResolvedType();
      node->SEM_Variable = var;
    }
    //If we can't find a var, make a global one!
    else if(ValidAssignment)
    {
      Variable *var = lib->CreateVariable(node->Name.str(), true);
      var->Type = VariableType::Field;

      Type *predictiveType = lib->CreateBlankType("Predictive");
      predictiveType->Predictive = true;

      var->ResolvedType = predictiveType;

      node->SEM_ResolvedSymbol = var->GetResolvedType();
      node->SEM_Variable = var;
    }

    //@REMIND Type node?

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionVariableNode* node)
  {
    node->Expression->Walk(this);

    //First resolve the symbol to the type
    node->SEM_ResolvedSymbol = node->Expression->SEM_ResolvedType;

    //@TODO Handle suffix changing type
    if (node->Suffix)
    {
      node->Suffix->SEM_ResolvedSymbol = node->SEM_ResolvedSymbol;
      node->Suffix->Walk(this);
      node->SEM_ResolvedSymbol = node->Suffix->SEM_ResolvedSymbol->GetResolvedType();
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableSuffixNode* node)
  {
    //Resolve everything to the left
    bool resolveAssignment = ResolveAssignment;

    ResolveAssignment = false;
    if (node->LeftSuffix)
    {
      node->LeftSuffix->SEM_ResolvedSymbol = node->SEM_ResolvedSymbol;
      node->LeftSuffix->Walk(this);
      node->SEM_ResolvedSymbol = node->LeftSuffix->SEM_ResolvedSymbol;
      node->SEM_Variable = node->LeftSuffix->SEM_Variable;
    }
    ResolveAssignment = resolveAssignment;

    //Resolve function calls
    for (auto &callNode : node->CallNodes)
    {
      callNode->SEM_ResolvedSymbol = node->SEM_ResolvedSymbol;
      callNode->Walk(this);
      node->SEM_ResolvedSymbol = callNode->SEM_ResolvedSymbol;
      node->SEM_Variable = nullptr;
    }

    //Resolve indecies
    if (node->Index)
    {
      node->Index->Walk(this);

      //If we have a symbol, try to resolve the type
      if (node->SEM_ResolvedSymbol)
      {
        //If we are going to do assignment, create the variable beforehand
        if (ResolveAssignment)
        {

          //If the symbol is a table, add an entry
          if (node->SEM_ResolvedSymbol->Name == "Table")
          {
            ValueData newIndex;
            bool isValid = true;

            IdentifiedIndexNode *identityNode = dynamic_cast<IdentifiedIndexNode *>(node->Index.get());
            if (identityNode)
            {
              newIndex.Type = ExpressionType::String;
              newIndex.Data.String = identityNode->Name.str();

              if (newIndex.Data.String.empty())
                isValid = false;
            }

            ExpressionIndexNode *expressionNode = dynamic_cast<ExpressionIndexNode *>(node->Index.get());
            if (expressionNode)
            {
              newIndex = expressionNode->Expression->SEM_Value;
            }

            if (isValid)
            {
              //First, check if there is already a variable with the same index
              TableData *existingData = nullptr;
              for (Variable *var : node->SEM_ResolvedSymbol->Members)
              {
                TableData *tblData = dynamic_cast<TableData *>(var);
                if (tblData && tblData->Index == newIndex)
                {
                  existingData = tblData;
                  break;
                }
              }

              //If the same index exists
              if (existingData)
              {
                //If this type was predictive, replace it
                if (existingData->Predictive)
                {
                  if (expressionNode)
                  {
                    existingData->indexExpression = true;
                    expressionNode->SEM_Variable = existingData;
                  }
                  else
                  {
                    identityNode->SEM_Variable = existingData;
                  }

                  existingData->Index = newIndex;
                }
                //Otherwise, there is a conflict
                else
                {
                  //@TODO: Fix this conflict, probably with superposision
                }

                node->SEM_Variable = existingData;
                node->SEM_ResolvedSymbol = existingData->GetResolvedType();
              }
              //Otherwise, no overlap. Add as new variable
              else
              {
                TableData *tableVar = (TableData *)CreateBlankSymbol<TableData>(lib, "TableVar", false);
                tableVar->Type = VariableType::TableValue;

                if (expressionNode)
                {
                  tableVar->indexExpression = true;
                  expressionNode->SEM_Variable = tableVar;
                }
                else
                {
                  identityNode->SEM_Variable = tableVar;
                }

                tableVar->Index = newIndex;

                tableVar->Parent = node->SEM_ResolvedSymbol;
                node->SEM_ResolvedSymbol->Members.push_back(tableVar);
                node->SEM_Variable = tableVar;
              }
            }
          }

          return VisitResult::Stop;
        }

        IdentifiedIndexNode *identityNode = dynamic_cast<IdentifiedIndexNode *>(node->Index.get());
        if (identityNode)
        {
          for (Variable *var : node->SEM_ResolvedSymbol->Members)
          {
            if (var->Type == VariableType::TableValue)
            {
              TableData *data = (TableData *)var;

              //If the index is a string, and matches the node
              if (data->Index == identityNode->Name.str())
              {
                node->SEM_ResolvedSymbol = data->GetResolvedType();
                node->SEM_Variable = data;
                return VisitResult::Stop;
              }
            }
          }

          if (ValidAssignment)
          {
            //If we get here, no symbol has been found. Create a temporary one for prediction
            ValueData newIndex;
            newIndex.Type = ExpressionType::String;
            newIndex.Data.String = identityNode->Name.str();

            if (!newIndex.Data.String.empty())
            {
              TableData *tableVar = (TableData *)CreateBlankSymbol<TableData>(lib, "TableVar", false);
              tableVar->Type = VariableType::TableValue;
              tableVar->Predictive = true;

              tableVar->Index = newIndex;
              tableVar->ResolvedType = nullptr;

              tableVar->Parent = node->SEM_ResolvedSymbol;
              node->SEM_ResolvedSymbol->Members.push_back(tableVar);

              Type *predictiveType = lib->CreateBlankType("Predictive");
              predictiveType->Predictive = true;
              tableVar->ResolvedType = predictiveType;

              node->SEM_Variable = tableVar;
              node->SEM_ResolvedSymbol = predictiveType;

              identityNode->SEM_Variable = tableVar;
              return VisitResult::Stop;
            }

            //Base case, no resolved symbol
            node->SEM_ResolvedSymbol = nullptr;
          }
        }
        else
        {
          ExpressionIndexNode *expressionNode = dynamic_cast<ExpressionIndexNode *>(node->Index.get());
          if (expressionNode)
          {
            for (Variable *var : node->SEM_ResolvedSymbol->Members)
            {
              if (var->Type == VariableType::TableValue)
              {
                TableData *data = (TableData *)var;

                //If the index is a string, and matches the node
                if (data->Index == expressionNode->Expression->SEM_Value)
                {
                  node->SEM_ResolvedSymbol = data->GetResolvedType();
                  node->SEM_Variable = data;
                  return VisitResult::Stop;
                }
              }
            }

            node->SEM_ResolvedSymbol = nullptr;
          }
        }
      }
      //If there is no resolved type, but there is a variable to the left, then we can predict that we are part of a table
      else if(node->LeftSuffix && node->LeftSuffix->SEM_Variable)
      {
        Type *parentPrediction = lib->CreateBlankType("Predictive");
        parentPrediction->Predictive = true;
        node->LeftSuffix->SEM_Variable->ResolvedType = parentPrediction;
        node->LeftSuffix->SEM_ResolvedSymbol = parentPrediction;

        node->SEM_ResolvedSymbol = parentPrediction;

        IdentifiedIndexNode *identityNode = dynamic_cast<IdentifiedIndexNode *>(node->Index.get());
        if (identityNode)
        {
          ValueData newIndex;
          newIndex.Type = ExpressionType::String;
          newIndex.Data.String = identityNode->Name.str();
          if (!newIndex.Data.String.empty())
          {
            TableData *tableVar = (TableData *)CreateBlankSymbol<TableData>(lib, "TableVar", false);
            tableVar->Type = VariableType::TableValue;
            tableVar->Predictive = true;

            tableVar->Index = newIndex;
            tableVar->ResolvedType = nullptr;

            tableVar->Parent = node->SEM_ResolvedSymbol;
            node->SEM_ResolvedSymbol->Members.push_back(tableVar);

            Type *predictiveType = lib->CreateBlankType("Predictive");
            predictiveType->Predictive = true;
            tableVar->ResolvedType = predictiveType;

            node->SEM_Variable = tableVar;
            node->SEM_ResolvedSymbol = predictiveType;

            identityNode->SEM_Variable = tableVar;
            return VisitResult::Stop;
          }
        }
      }
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(CallNode* node)
  {
    //If we don't have a symbol, we can't call the function
    if (node->SEM_ResolvedSymbol == nullptr)
      return VisitResult::Stop;

    if(node->Argument)
      node->Argument->Walk(this);

    Type *resType = node->SEM_ResolvedSymbol->GetResolvedType();

    if (resType && resType->ReturnType)
    {
      resType = resType->ReturnType->GetResolvedType();
    }
    else
    {
      //If there is a type, there is potential for a possible return type.
      //Must search resursively through all posibilities to find return types.
      if (resType)
      {
        auto ret = PredictFunctionCall(resType->GetResolvedType());
        if (ret.first)
        {
          //Search was successful at narrowing, now we need to do the function call.

          //If its a possibility space, resolve all of them
          if (ret.second->PossibleTypes.size() > 0)
          {
            Type *newType = lib->CreateBlankType("");

            for (Type *t : ret.second->PossibleTypes)
            {
              lib->AddPossibleType(newType, t->ReturnType->GetResolvedType());
            }

            resType = newType;
          }
          else
          {
            //If its a multi-call, call the first arg and discard the rest
            if (ret.second->MultipleTypes.size() > 0)
            {
              resType = ret.second->MultipleTypes[0]->ReturnType->GetResolvedType();
            }
            else
            {
              resType = ret.second->ReturnType->GetResolvedType();
            }
          }
        }
        //Otherwise, search was a failure. Set it to nullptr
        else
        {
          resType = nullptr;
        }
      }
    }

    node->SEM_ResolvedSymbol = resType->GetResolvedType();

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(MemberCallNode* node)
  {
    //If we don't have a symbol, we can't call the function
    if (node->SEM_ResolvedSymbol == nullptr)
      return VisitResult::Stop;

    std::string name = node->Name.str();

    for (Variable *var : node->SEM_ResolvedSymbol->Members)
    {
      TableData *tblData = dynamic_cast<TableData *>(var);
      if (tblData && tblData->Index == name)
      {
        node->SEM_ResolvedSymbol = tblData->GetResolvedType();

        //Resolve the call with the parent node
        return Visit((CallNode *)node);
      }
    }


    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ArgumentNode* node)
  {


    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionArgumentNode* node)
  {
    for (auto &e_node : node->ExpressionList)
    {
      e_node->Walk(this);
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


    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IndexNode* node)
  {


    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IdentifiedIndexNode* node)
  {


    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionIndexNode* node)
  {
    node->Expression->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BreakNode* node)
  {


    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ReturnNode* node)
  {
    std::vector<Type *> returnTypes;

    for (auto &r_node : node->ReturnValues)
    {
      r_node->Walk(this);
      returnTypes.push_back(r_node->SEM_ResolvedType);
    }

    Type *returnType = lib->CreateMultipleType(returnTypes);

    lib->AddPossibleType(functionStack.top()->SYM_ReturnType, returnType);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ValueNode* node)
  {
    if (node->Expression.TokenType == Token::Type::IntegerLiteral)
    {
      node->SEM_ResolvedType = GetBaseType("Number");

      node->SEM_Value.Type = ExpressionType::Number;
      node->SEM_Value.Data.Number = std::strtoul(node->Expression.str().c_str(), nullptr, 10);
    }

    if (node->Expression.TokenType == Token::Type::FloatLiteral)
    {
      node->SEM_ResolvedType = GetBaseType("Number");

      node->SEM_Value.Type = ExpressionType::Number;
      node->SEM_Value.Data.Number = std::strtof(node->Expression.str().c_str(), nullptr);
    }

    if (node->Expression.TokenType == Token::Type::StringLiteral)
    {
      node->SEM_ResolvedType = GetBaseType("String");

      //Resolve all of the string literal decorations
      std::string str = node->Expression.str();
      if (str[0] == '\"' || str[0] == '\'')
      {
        str = str.substr(1, str.size() - 2);
      }
      else if (str[0] == '[')
      {
        str = str.substr(2, str.size() - 3);
      }

      node->SEM_Value.Type = ExpressionType::String;
      node->SEM_Value.Data.String = str;
    }

    if (node->Expression.TokenType == Token::Type::True || node->Expression.TokenType == Token::Type::False)
    {
      node->SEM_ResolvedType = GetBaseType("Boolean");

      node->SEM_Value.Type = ExpressionType::Boolean;
      node->SEM_Value.Data.Boolean = node->Expression.TokenType == Token::Type::True ? true : false;
    }

    if (node->Expression.TokenType == Token::Type::Nil)
    {
      node->SEM_ResolvedType = GetBaseType("Nil");
      node->SEM_Value.Type = ExpressionType::Nil;
    }

    if (node->Expression.TokenType == Token::Type::Identifier)
    {
      Variable *var = GetVariable(node->Expression.str());
      node->SEM_ResolvedType = var->GetResolvedType();
      node->SEM_Value.Type = ExpressionType::Reference;
      node->SEM_Value.Data.Reference = var;
    }

    if (node->Expression.TokenType == Token::Type::VariableDot)
    {
      node->SEM_ResolvedType = GetBaseType("VariableArgument");
      node->SEM_Value.Type = ExpressionType::VariableArgument;
    }

    return Visitor::Visit(node);
  }

  virtual VisitResult Visit(TableNode* node)
  {
    for (auto &i_node : node->Indicies)
    {
      i_node->Walk(this);
    }

    for (auto &v_node : node->Values)
    {
      v_node->Walk(this);
    }

    Type *tableType = lib->CreateBlankType("Table");

    int arrayIndex = 1;
    for (int i = 0; i < node->FullValues.size(); ++i)
    {
      //If there is no value, member is automatically nil
      if (node->FullValues[i] != nullptr)
      {
        TableData *tableVar = (TableData *)CreateBlankSymbol<TableData>(lib, "TableVar", false);
        tableVar->Type = VariableType::TableValue;

        //If there is no corresponding index, default to array index
        if (node->FullIndicies[i] == nullptr)
        {
          tableVar->Index.Data.Number = arrayIndex;
          tableVar->Index.Type = ExpressionType::Number;
        }
        else
        {
          IdentifiedIndexNode *identityNode = dynamic_cast<IdentifiedIndexNode *>(node->FullIndicies[i]);
          if (identityNode)
          {
            tableVar->Index.Data.String = identityNode->Name.str();
            tableVar->Index.Type = ExpressionType::String;
          }
          else
          {
            ExpressionIndexNode *expressionNode = dynamic_cast<ExpressionIndexNode *>(node->FullIndicies[i]);
            if (expressionNode)
            {
              tableVar->Index = expressionNode->Expression->SEM_Value;
            }
          }
        }

        tableVar->ResolvedType = node->FullValues[i]->SEM_ResolvedType->GetResolvedType();
        tableVar->Value = node->FullValues[i]->SEM_Value;
        tableVar->Parent = tableType;
        tableType->Members.push_back(tableVar);
      }
    }

    node->SEM_ResolvedType = tableType;

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionExpressionNode* node)
  {
    node->Function->Walk(this);

    node->SEM_ResolvedType = node->Function->SYM_Variable->GetResolvedType();

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionCallNode* node)
  {
    node->Variable->Walk(this);
    node->SEM_ResolvedType = node->Variable->SEM_ResolvedSymbol->GetResolvedType();

    for (auto &callNode : node->Calls)
    {
      callNode->SEM_ResolvedSymbol = node->SEM_ResolvedType;
      callNode->Walk(this);
      node->SEM_ResolvedType = callNode->SEM_ResolvedSymbol->GetResolvedType();
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(PrefixExpressionNode* node)
  {
    node->LeftVar->Walk(this);

    if (node->LeftVar->SEM_Variable)
      node->SEM_Value = node->LeftVar->SEM_Variable->Value;

    if (node->LeftVar->SEM_ResolvedSymbol)
    {
      node->SEM_ResolvedType = node->LeftVar->SEM_ResolvedSymbol->GetResolvedType();

      //Resolve function calls
      for (auto &callNode : node->RightCalls)
      {
        callNode->SEM_ResolvedSymbol = node->SEM_ResolvedType;
        callNode->Walk(this);
        node->SEM_ResolvedType = callNode->SEM_ResolvedSymbol->GetResolvedType();
      }
    }

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(UnaryOperatorNode* node)
  {


    node->Right->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BinaryOperatorNode* node)
  {
    //Evaluate both sides
    node->Left->Walk(this);
    node->Right->Walk(this);
    
    Type *NumberType = GetBaseType("Number");
    Type *StringType = GetBaseType("String");

    //Handle add, sub, mul, div, mod, pow
    if (node->Operator.EnumTokenType == Token::Type::Plus || node->Operator.EnumTokenType == Token::Type::Minus || node->Operator.EnumTokenType == Token::Type::Multiply ||
      node->Operator.EnumTokenType == Token::Type::Divide || node->Operator.EnumTokenType == Token::Type::Modulo || node->Operator.EnumTokenType == Token::Type::Exponent)
    {
      //First check if both are numeric
      if (node->Left->SEM_ResolvedType == NumberType && node->Right->SEM_ResolvedType == NumberType)
      {
        node->SEM_ResolvedType = GetBaseType("Number");

        //If both are const numbers, do the operation
        if (node->Left->SEM_Value.Type == ExpressionType::Number || node->Right->SEM_Value.Type == ExpressionType::Number)
        {
          node->SEM_Value.Type = ExpressionType::Number;

          if (node->Operator.EnumTokenType == Token::Type::Plus)
          {
            node->SEM_Value.Data.Number = node->Left->SEM_Value.Data.Number + node->Right->SEM_Value.Data.Number;
          }
          
          if (node->Operator.EnumTokenType == Token::Type::Minus)
          {
            node->SEM_Value.Data.Number = node->Left->SEM_Value.Data.Number - node->Right->SEM_Value.Data.Number;
          }
          
          if (node->Operator.EnumTokenType == Token::Type::Multiply)
          {
            node->SEM_Value.Data.Number = node->Left->SEM_Value.Data.Number * node->Right->SEM_Value.Data.Number;
          }

          if (node->Operator.EnumTokenType == Token::Type::Divide)
          {
            node->SEM_Value.Data.Number = node->Left->SEM_Value.Data.Number / node->Right->SEM_Value.Data.Number;
          }
          
          if (node->Operator.EnumTokenType == Token::Type::Modulo)
          {
            node->SEM_Value.Data.Number = std::fmod(node->Left->SEM_Value.Data.Number, node->Right->SEM_Value.Data.Number);
          }
          
          if(node->Operator.EnumTokenType == Token::Type::Exponent)
          {
            node->SEM_Value.Data.Number = std::pow(node->Left->SEM_Value.Data.Number, node->Right->SEM_Value.Data.Number);
          }
        }

        return VisitResult::Stop;
      }

      //Not numeric, need to handle with metatable
      //@TODO Handle metatable

      return VisitResult::Stop;
    }

    
    // Handle concat
    if (node->Operator.EnumTokenType == Token::Type::Concat)
    {
      // (String or Number) and (String or Number)
      if ((node->Left->SEM_ResolvedType == NumberType || node->Left->SEM_ResolvedType == StringType) && (node->Right->SEM_ResolvedType == NumberType || node->Right->SEM_ResolvedType == StringType))
      {
        node->SEM_ResolvedType = GetBaseType("String");

        //@TODO Handle const values
        return VisitResult::Stop;
      }
      else
      {
        //Need to handle with metatable
        //@TODO Handle metatable
      }
    }

    //Handle eq
    if (node->Operator.EnumTokenType == Token::Type::EqualsTo || node->Operator.EnumTokenType == Token::Type::NotEqualsTo)
    {
      //If types are not the same, eq is false
      if (node->Left->SEM_ResolvedType != node->Right->SEM_ResolvedType)
      {
        //@REM Should be false

        node->SEM_ResolvedType = GetBaseType("Boolean");
        return VisitResult::Stop;
      }

      if (node->Left->SEM_ResolvedType == node->Right->SEM_ResolvedType)
      {
        //@REM Should be compared
        node->SEM_ResolvedType = GetBaseType("Boolean");
        return VisitResult::Stop;
      }

      //Otherwise, metamethod
      //@TODO Handle metatable
    }

    //Handle lt
    if (node->Operator.EnumTokenType == Token::Type::LessThan || node->Operator.EnumTokenType == Token::Type::GreaterThan)
    {
      //If both are numbers, do numeric comp
      if (node->Left->SEM_ResolvedType == NumberType && node->Right->SEM_ResolvedType == NumberType)
      {
        node->SEM_ResolvedType = GetBaseType("Boolean");
        return VisitResult::Stop;
      }

      //If both are strings, do lexicographic comp
      if (node->Left->SEM_ResolvedType == StringType && node->Right->SEM_ResolvedType == StringType)
      {
        node->SEM_ResolvedType = GetBaseType("Boolean");
        return VisitResult::Stop;
      }

      //Otherwise, metamethod
      //@TODO Handle metatable
    }

    //Handle le
    if (node->Operator.EnumTokenType == Token::Type::LessThanOrEqualTo || node->Operator.EnumTokenType == Token::Type::GreaterThanOrEqualTo)
    {
      //If both are numbers, do numeric comp
      if (node->Left->SEM_ResolvedType == NumberType && node->Right->SEM_ResolvedType == NumberType)
      {
        node->SEM_ResolvedType = GetBaseType("Boolean");
        return VisitResult::Stop;
      }

      //If both are strings, do lexicographic comp
      if (node->Left->SEM_ResolvedType == StringType && node->Right->SEM_ResolvedType == StringType)
      {
        node->SEM_ResolvedType = GetBaseType("Boolean");
        return VisitResult::Stop;
      }

      //Otherwise, metamethod
      //@TODO Handle metatable
      //@REM in absance of __le metamethod, Lua tries the "lt", assuming that a <= b is equivalent to not (b < a).
    }

    //Handle and/or
    if (node->Operator.EnumTokenType == Token::Type::And || node->Operator.EnumTokenType == Token::Type::Or)
    {
      //If both are boolean, return boolean
      Type *BooleanType = GetBaseType("Boolean");
      if (node->Left->SEM_ResolvedType == BooleanType && node->Right->SEM_ResolvedType == BooleanType)
      {
        node->SEM_ResolvedType = GetBaseType("Boolean");
        return VisitResult::Stop;
      }
    }


    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionNode* node)
  {
    //If this is the global function
    if (parentStack.size() == 0)
    {
      node->SYM_Variable = lib->CreateVariable("GlobalChunk");
    }
    else
    {
      //Create name
      std::string functionName;
      bool first = true;
      bool isMemberFunc = false;

      Variable *functionVar = nullptr;
      for (int i = 0; i < node->Name.size(); ++i)
      {
        FunctionNameNode *nameNode = node->Name[i].get();
        functionName = nameNode->Name.str();

        isMemberFunc = nameNode->IsMemberFunc;

        if (i >= node->Name.size() - 1)
          break;

        if (i == 0)
        {
          functionVar = GetVariable(functionName);

          //If no variable is found, predict it
          if (functionVar == nullptr && !functionName.empty())
          {
            Variable *var = lib->CreateVariable(functionName, node->IsLocal == false);
            var->Type = VariableType::Field;

            Type *predictiveType = lib->CreateBlankType("Predictive");
            predictiveType->Predictive = true;

            var->ResolvedType = predictiveType;
            functionVar = var;
          }
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

            //If no variable is found, predict it
            if (functionVar == nullptr && !functionName.empty())
            {
              ValueData newIndex;
              newIndex.Type = ExpressionType::String;
              newIndex.Data.String = functionName;

              TableData *tableVar = (TableData *)CreateBlankSymbol<TableData>(lib, "TableVar", false);
              tableVar->Type = VariableType::TableValue;
              tableVar->Predictive = true;

              tableVar->Index = newIndex;
              tableVar->ResolvedType = nullptr;

              tableVar->Parent = temp;
              temp->Members.push_back(tableVar);

              Type *predictiveType = lib->CreateBlankType("Predictive");
              predictiveType->Predictive = true;
              tableVar->ResolvedType = predictiveType;

              functionVar = tableVar;
            }
          }
        }
      }

      //This function belongs inside a class
      if (functionVar != nullptr)
      {
        //Create function, and add it to the class
        ValueData newIndex;
        newIndex.Type = ExpressionType::String;
        newIndex.Data.String = functionName;

        TableData *tableVar = (TableData *)CreateBlankSymbol<TableData>(lib, "TableVar", false);
        tableVar->Type = VariableType::TableValue;
        tableVar->Index = newIndex;
        tableVar->ValueType = VariableType::Function;

        node->SYM_Variable = tableVar;
        functionVar->GetResolvedType()->Members.push_back(node->SYM_Variable);

        if (isMemberFunc)
        {
          tableVar->ValueType = VariableType::Method;

          Variable *var = lib->CreateVariable("self", false);
          var->ResolvedType = functionVar->GetResolvedType();

          node->SYM_Variable->Members.push_back(var);
          node->Block->localSymbols.push_back(var);
        }
      }
      else
      {
        bool anonymous_function = functionName == "";

        //anonymous_function function
        if (anonymous_function)
        {
          static int function_id = 0;
          function_id++;

          functionName = "Un-named function #";
          functionName += std::to_string(function_id);
        }

        node->SYM_Variable = lib->CreateVariable(functionName, anonymous_function == false && node->IsLocal == false);
        node->SYM_Variable->Type = VariableType::Function;

        //If its local, add to parent
        if (node->IsLocal && !anonymous_function)
        {
          parentStack.back()->Members.push_back(node->SYM_Variable);
          node->SYM_Variable->Parent = parentStack.back();
        }
      }
    }

    parentStack.push_back(node->SYM_Variable);

    //Push the parameters as local vars, predict their types
    //@TODO: Fully develop function type resolution
    for (Token &t : node->ParameterList)
    {
      Type *predictiveType = lib->CreateBlankType("Predictive");
      predictiveType->Predictive = true;

      Variable *var = lib->CreateVariable(t.str());
      var->ResolvedType = predictiveType;

      parentStack.back()->Members.push_back(var);
      var->Parent = parentStack.back();
      node->Block->localSymbols.push_back(var);
    }

    

    node->SYM_ReturnType = lib->CreateBlankType("");
    node->SYM_ReturnType->ResolvedType = GetBaseType("Nil");

    functionStack.push(node);
    node->Block->Walk(this);
    functionStack.pop();

    parentStack.pop_back();

    Type *functionType = lib->CreateFunctionType(node);
    node->SYM_Variable->ResolvedType = functionType;

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
      node->Condition->Walk(this);

    node->Block->Walk(this);

    if (node->Else)
      node->Else->Walk(this);

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

    node->Block->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(GenericForNode* node)
  {


    for (auto &e_node : node->ExpressionList)
    {
      e_node->Walk(this);
    }

    node->Block->Walk(this);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(LocalVariableNode* node)
  {
    std::vector<Type *> expressionTypes;
    std::vector<ValueData> expressionData;
    int num_varargs = 0;

    for (auto &e_node : node->ExpressionList)
    {
      e_node->Walk(this);

      if (e_node->SEM_ResolvedType && e_node->SEM_ResolvedType->MultipleTypes.size() > 0)
      {
        //If we have multiple types, push them separately
        for (Type *t : e_node->SEM_ResolvedType->MultipleTypes)
        {
          if (t && t->GetResolvedType() == GetBaseType("VariableArgument"))
            num_varargs += 1;

          expressionTypes.push_back(t);
          expressionData.push_back(ValueData());
        }
      }
      else
      {
        if (e_node->SEM_ResolvedType && e_node->SEM_ResolvedType->GetResolvedType() == GetBaseType("VariableArgument"))
          num_varargs += 1;

        expressionTypes.push_back(e_node->SEM_ResolvedType);
        expressionData.push_back(e_node->SEM_Value);
      }
    }

    //If there are varargs, figure out how many spaces they occupy and fix the vectors
    if (num_varargs > 0)
    {
      int other_args = expressionTypes.size() - num_varargs;
      int num_variables = node->Names.size();

      int varargs_length = (num_variables - other_args) / num_varargs;

      std::vector<Type *> new_expressionTypes;
      std::vector<ValueData> new_expressionData;

      //For each variable argument, make a predictive type
      for (int i = 0; i < expressionTypes.size(); ++i)
      {
        Type *t = expressionTypes[i];
        if (t && t->GetResolvedType() == GetBaseType("VariableArgument"))
        {
          for (int v = 0; v < varargs_length; ++v)
          {
            Type *predictiveType = lib->CreateBlankType("Predictive");
            predictiveType->Predictive = true;

            new_expressionTypes.push_back(predictiveType);
            new_expressionData.push_back(ValueData());
          }
        }
        else
        {
          new_expressionTypes.push_back(expressionTypes[i]);
          new_expressionData.push_back(expressionData[i]);
        }
      }

      expressionTypes = new_expressionTypes;
      expressionData = new_expressionData;
    }

    for (int i = 0; i < node->Names.size(); ++i)
    {
      Variable *var = lib->CreateVariable(node->Names[i].str());
      var->Type = VariableType::Field;

      if (i < expressionTypes.size())
      {
        var->ResolvedType = expressionTypes[i];
        var->Value = expressionData[i];
      }
      else
      {
        var->ResolvedType = GetBaseType("Nil");
      }

      parentStack.back()->Members.push_back(var);
      var->Parent = parentStack.back();

      blockStack.top()->localSymbols.push_back(var);
    }

    return VisitResult::Stop;
  }
};

class PrintTypeVisitor : public Visitor
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

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(IdentifiedVariableNode* node)
  {
    NodePrinter printer;
    printer << "IdentifiedVariableNode(" << node->Name << ")" << " - " << node->SEM_ResolvedSymbol;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ExpressionVariableNode* node)
  {
    NodePrinter printer;
    printer << "ExpressionVariableNode" << " - " << node->SEM_ResolvedSymbol;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(VariableSuffixNode* node)
  {
    NodePrinter printer;
    printer << "VariableSuffixNode" << " - " << node->SEM_ResolvedSymbol;

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
    printer << "ExpressionNode(" << node->SEM_ResolvedType << ")";

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(ValueNode* node)
  {
    NodePrinter printer;
    printer << "ValueNode(" << node->Expression << ")" << " - " << node->SEM_ResolvedType << " : " << node->SEM_Value;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(TableNode* node)
  {
    NodePrinter printer;
    printer << "TableNode" << " - " << node->SEM_ResolvedType;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionExpressionNode* node)
  {
    NodePrinter printer;
    printer << "FunctionExpressionNode" << " - " << node->SEM_ResolvedType;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionCallNode* node)
  {
    NodePrinter printer;
    printer << "FunctionCallNode" << " - " << node->SEM_ResolvedType;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(PrefixExpressionNode* node)
  {
    NodePrinter printer;
    printer << "PrefixExpressionNode" << " - " << node->SEM_ResolvedType << " : " << node->SEM_Value;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(UnaryOperatorNode* node)
  {
    NodePrinter printer;
    printer << "UnaryOperatorNode(" << node->Operator << ")" << " - " << node->SEM_ResolvedType << " : " << node->SEM_Value;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(BinaryOperatorNode* node)
  {
    NodePrinter printer;
    printer << "BinaryOperatorNode(" << node->Operator << ")" << " - " << node->SEM_ResolvedType << " : " << node->SEM_Value;

    node->Walk(this, false);

    return VisitResult::Stop;
  }

  virtual VisitResult Visit(FunctionNode* node)
  {
    NodePrinter printer;
    printer << "FunctionNode" << " - " << node->SYM_ReturnType;

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

    printer << " - ";

    for (auto &expr : node->ExpressionList)
    {
      printer << expr->SEM_ResolvedType->GetResolvedType() << " ";
    }

    node->Walk(this, false);

    return VisitResult::Stop;
  }
};

//Parent Resolver
class ParentVisitor : public Visitor
{
public:
  virtual VisitResult Visit(BlockNode* node)
  {
    for (auto &child : node->Statements)
    {
      child->Parent = node;
    }

    //Short circuit the parent for correct autocomplete.
    if(node->End)
      node->End->Parent = node->Parent;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(AssignmentNode* node)
  {
    for (auto &child : node->LeftVariables)
    {
      child->Parent = node;
    }

    for (auto &child : node->RightExpressions)
    {
      child->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(VariableStatementNode* node)
  {
    node->Variable->Parent = node;

    if (node->Suffix)
    {
      node->Suffix->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(ExpressionVariableNode* node)
  {
    node->Expression->Parent = node;

    if (node->Suffix)
    {
      node->Suffix->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(VariableSuffixNode* node)
  {
    if (node->LeftSuffix)
    {
      node->LeftSuffix->Parent = node;
    }

    for (auto &child : node->CallNodes)
    {
      child->Parent = node;
    }

    if (node->Index)
    {
      node->Index->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(CallNode* node)
  {
    if(node->Argument)
      node->Argument->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(ExpressionArgumentNode* node)
  {
    for (auto &child : node->ExpressionList)
    {
      child->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(TableArgumentNode* node)
  {
    node->Table->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(ExpressionIndexNode* node)
  {
    node->Expression->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(ReturnNode* node)
  {
    for (auto &child : node->ReturnValues)
    {
      child->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(TableNode* node)
  {
    for (auto &child : node->Indicies)
    {
      child->Parent = node;
    }

    for (auto &child : node->Values)
    {
      child->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(FunctionExpressionNode* node)
  {
    node->Function->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(FunctionCallNode* node)
  {
    node->Variable->Parent = node;

    for (auto &child : node->Calls)
    {
      child->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(PrefixExpressionNode* node)
  {
    node->LeftVar->Parent = node;

    for (auto &child : node->RightCalls)
    {
      child->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(UnaryOperatorNode* node)
  {
    node->Right->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(BinaryOperatorNode* node)
  {
    node->Left->Parent = node;
    node->Right->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(FunctionNode* node)
  {
    for (auto &child : node->Name)
    {
      child->Parent = node;
    }

    node->Block->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(WhileNode* node)
  {
    node->Condition->Parent = node;
    node->Block->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(RepeatNode* node)
  {
    node->Block->Parent = node;
    node->Condition->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(IfNode* node)
  {
    if (node->Condition)
    {
      node->Condition->Parent = node;
    }

    node->Block->Parent = node;

    if (node->Else)
    {
      node->Else->Parent = node;
    }

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(ForNode* node)
  {
    node->Block->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(NumericForNode* node)
  {
    node->Var->Parent = node;
    node->Limit->Parent = node;

    if (node->Step)
    {
      node->Step->Parent = node;
    }

    node->Block->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(GenericForNode* node)
  {
    for (auto &child : node->ExpressionList)
    {
      child->Parent = node;
    }

    node->Block->Parent = node;

    return VisitResult::Continue;
  }

  virtual VisitResult Visit(LocalVariableNode* node)
  {
    for (auto &child : node->ExpressionList)
    {
      child->Parent = node;
    }
    return VisitResult::Continue;
  }
};

LibraryReference::~LibraryReference()
{
  int a = 5;

  //First reduce all reference counts we own
  for (auto &pair : SymbolReferences)
  {
    if(pair.first->ReferenceCount > 0)
      pair.first->ReferenceCount -= 1;
  }

  library->Clean();
}

void ResolveTypes(AbstractNode *ast, Library *lib, LibraryReference *libRef)
{
  lib->currentRef = libRef;
  libRef->library = lib;

  ResolveTypesVisitor resTypes;
  resTypes.lib = lib;

  ast->Walk(&resTypes);

  ParentVisitor parentResolve;
  ast->Walk(&parentResolve);

  lib->currentRef = nullptr;
}

void PrintTypes(AbstractNode *ast)
{
  PrintTypeVisitor visitor;
  ast->Walk(&visitor);
}