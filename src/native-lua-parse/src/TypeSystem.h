#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

template <typename T>
class unique_vector : public std::vector<std::unique_ptr<T>>
{
public:
  bool push_back(std::unique_ptr<T> ptr)
  {
    if (ptr)
    {
      vector::push_back(std::move(ptr));
      return true;
    }
    return false;
  }
};

class Type;
class Library;
class Variable;

template<typename T>
T *CleanSymbol(T *sym)
{
  if (sym)
  {
    //sym->Clean();

    if (sym->ReferenceCount == 0)
      return nullptr;
  }

  return sym;
}

template <typename V>
void CleanMembers(V &symVec)
{
  //for (unsigned i = 0; i < symVec.size(); ++i)
  //{
  //  T *v = symVec[i];
  //  v->Clean();
  //}

  RemoveUnreferencedElements(symVec);
}

template <typename V>
void RemoveUnreferencedElements(V &symVec)
{
  for (unsigned i = 0; i < symVec.size(); ++i)
  {
    auto &v = symVec[i];

    if (v == nullptr || v->ReferenceCount == 0)
    {
      std::swap(symVec.back(), symVec[i]);
      symVec.pop_back();
      i--;
    }
  }
}

// A symbol is anything that must be identified by name
class Symbol
{
public:
  virtual ~Symbol()
  {}

  //Symbol name
  std::string Name; 
  
  //Type symbol resolves to
  Type *ResolvedType; 
  
  //Parent of symbol (i.e type inside a class, table, or function)
  Symbol *Parent;

  std::vector<Variable *> Members;

  //Library symbol is apart of
  Library* Owner;

  unsigned ReferenceCount = 0;
  
  bool hasClearedRefs = true;

  Type *GetResolvedType() const;

  virtual void Clean()
  {
    if (hasClearedRefs)
      return;

    hasClearedRefs = true;

    Parent = CleanSymbol(Parent);
    ResolvedType = CleanSymbol(ResolvedType);

    CleanMembers(Members);
  }
};

std::ostream& operator<<(std::ostream& stream, const Symbol* symbol);
std::ostream& operator<<(std::ostream& stream, const Symbol& symbol);

// Could be a global, local, or even class member variable
enum class VariableType
{
  Default,
  Field,
  TableValue,
  Function,
  Method
};

enum class ExpressionType
{
  Invalid,
  Number,
  String,
  Boolean,
  Nil,
  Reference,
  VariableArgument
};

struct ValueData
{
  struct
  {
    std::string String;
    union
    {
      float Number;
      bool Boolean;

      Variable *Reference;
    };
  } Data;

  //General ==
  bool operator==(ValueData const &rhs);

  //Specific ==
  bool operator==(std::string const &rhs);
  bool operator==(unsigned const &rhs);
  bool operator==(int const &rhs);
  bool operator==(float const &rhs);
  bool operator==(Variable *rhs);
  bool operator==(bool rhs);

  ExpressionType Type = ExpressionType::Invalid;
};

std::ostream& operator<<(std::ostream& stream, const ValueData* symbol);
std::ostream& operator<<(std::ostream& stream, const ValueData& symbol);

class Variable : public Symbol
{
public:
  VariableType Type;
  bool Predictive = false;

  //Value of variable
  ValueData Value;
  VariableType ValueType = VariableType::Default;

  // Whether or not this variable is a parameter
  bool IsParameter;

  virtual void Clean()
  {
    if (hasClearedRefs)
      return;
    Symbol::Clean();

    if (Value.Type == ExpressionType::Reference)
      Value.Data.Reference = CleanSymbol(Value.Data.Reference);
  }
};

class TableData : public Variable
{
public:
  ValueData Index;
  bool indexExpression = false;

  virtual void Clean()
  {
    if (hasClearedRefs)
      return;
    Symbol::Clean();

    if (Index.Type == ExpressionType::Reference)
      Index.Data.Reference = CleanSymbol(Index.Data.Reference);
  }
};

class Type : public Symbol
{
public:
  //If the type is comprised of multiple types
  std::vector<Type *> MultipleTypes;

  //If the type is comprised of multiple possibilities
  std::vector<Type *> PossibleTypes;

  bool Predictive = false;

  //If type is a function, it needs a return type
  Type *ReturnType;

  void CopyType(Type *newType)
  {
    if (newType == nullptr || newType == this)
      return;

    Name = newType->Name;
    MultipleTypes = newType->MultipleTypes;
    PossibleTypes = newType->PossibleTypes;
    ReturnType = newType->ReturnType;

    Parent = newType->Parent;
    for (Variable *var : newType->Members)
    {
      Members.push_back(var);
    }

    //@REM: Reference count
  }

  virtual void Clean()
  {
    if (hasClearedRefs)
      return;
    Symbol::Clean();

    ReturnType = CleanSymbol(ReturnType);
    CleanMembers(MultipleTypes);
    CleanMembers(PossibleTypes);
  }
};

class LibraryReference
{
public:
  ~LibraryReference();

  Library *library;
  std::unordered_map<Symbol *, unsigned> SymbolReferences;
};

// The library owns all symbols and is responsible for destroying them
class Library
{
public:
  // The library owns all symbols and will cleanup their memory upon destruction
  unique_vector<Symbol> AllSymbols;

  unique_vector<Symbol> TempSymbols;

  Variable *globalTable;
  std::vector<Symbol*> Globals;
  std::unordered_map<std::string, Symbol*> GlobalsByName;

  std::unordered_map<std::string, Symbol*> BaseTypesByName;

  LibraryReference *currentRef;
  
  //Used for un-named types
  Type*     CreateBlankType(const std::string& name, bool isGlobal = false);

  //Used for base types
  Type *    CreateBaseType(const std::string& name);

  Type*     CreateType(const std::string& name, bool isGlobal = false);
  Type*     CreateFunctionType(class FunctionNode *node);
  Type*     CreateMultipleType(std::vector<Type *> const &types, bool isGlobal = false);
  Type*     AddPossibleType(Type *baseType, Type *newType, bool isGlobal = false);
  Variable* CreateVariable(const std::string& name, bool isGlobal = false);

  void Clean();
};

class SemanticException : public std::exception
{
public:
  SemanticException(){}
  SemanticException(const std::string& error)
    :Error(error)
  {}

  const char* what() const override { return Error.c_str(); }
  std::string Error;
};

Library *CreateCoreLibrary();