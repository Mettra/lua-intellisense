#pragma once
#include "Token.h"
#include "TypeSystem.h"
#include <memory>
#include <vector>
#include <string>
#include <sstream>

/*
REGEX FOR PARSING

//[^\n]*                                                      Strip Comments

class (\w*)( : public (\w*))?.+?\};\r\n                       Get all class names in /1 and inheritance in /3




virtual VisitResult Visit\(\1* node\) \{ return this->Visit\(\(\3*\)node\); \}


*/


// Forward declarations of all the node types, autogen
//       \{[^}]*\};([^{]*class|)     &&&   NOTHING
//       (.*) :.*                    &&&   class\1;

class AbstractNode;
class StatementNode;
class BlockNode;
class TypeNode;
class AssignmentNode;
class VariableNode;
class VariableStatementNode;
class IdentifiedVariableNode;
class ExpressionVariableNode;
class VariableSuffixNode;
class CallNode;
class MemberCallNode;
class ArgumentNode;
class ExpressionArgumentNode;
class TableArgumentNode;
class StringArgumentNode;
class IndexNode;
class IdentifiedIndexNode;
class ExpressionIndexNode;
class BreakNode;
class ReturnNode;
class ExpressionNode;
class ValueNode;
class TableNode;
class FunctionExpressionNode;
class FunctionCallNode;
class PrefixExpressionNode;
class UnaryOperatorNode;
class BinaryOperatorNode;
class FunctionNode;
class FunctionNameNode;
class WhileNode;
class RepeatNode;
class IfNode;
class ForNode;
class NumericForNode;
class GenericForNode;
class LocalVariableNode;


// Forward declarations for the symbols
class Symbol;
class Type;
class Variable;
class Function;
class Library;

// Visitor //
enum class VisitResult
{
  Continue,
  Stop
};

class Visitor
{
public:
  virtual VisitResult Visit(AbstractNode* node) { return VisitResult::Continue; }
  virtual VisitResult Visit(StatementNode* node) { return this->Visit((AbstractNode*)node); }
  virtual VisitResult Visit(BlockNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(TypeNode* node) { return this->Visit((AbstractNode*)node); }
  virtual VisitResult Visit(AssignmentNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(VariableNode* node) { return this->Visit((AbstractNode*)node); }
  virtual VisitResult Visit(VariableStatementNode* node) { return this->Visit((VariableNode*)node); }
  virtual VisitResult Visit(IdentifiedVariableNode* node) { return this->Visit((VariableNode*)node); }
  virtual VisitResult Visit(ExpressionVariableNode* node) { return this->Visit((VariableNode*)node); }
  virtual VisitResult Visit(VariableSuffixNode* node) { return this->Visit((AbstractNode*)node); }
  virtual VisitResult Visit(CallNode* node) { return this->Visit((AbstractNode*)node); }
  virtual VisitResult Visit(MemberCallNode* node) { return this->Visit((CallNode*)node); }
  virtual VisitResult Visit(ArgumentNode* node) { return this->Visit((AbstractNode*)node); }
  virtual VisitResult Visit(ExpressionArgumentNode* node) { return this->Visit((ArgumentNode*)node); }
  virtual VisitResult Visit(TableArgumentNode* node) { return this->Visit((ArgumentNode*)node); }
  virtual VisitResult Visit(StringArgumentNode* node) { return this->Visit((ArgumentNode*)node); }
  virtual VisitResult Visit(IndexNode* node) { return this->Visit((AbstractNode*)node); }
  virtual VisitResult Visit(IdentifiedIndexNode* node) { return this->Visit((IndexNode*)node); }
  virtual VisitResult Visit(ExpressionIndexNode* node) { return this->Visit((IndexNode*)node); }
  virtual VisitResult Visit(BreakNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(ReturnNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(ExpressionNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(ValueNode* node) { return this->Visit((ExpressionNode*)node); }
  virtual VisitResult Visit(TableNode* node) { return this->Visit((ExpressionNode*)node); }
  virtual VisitResult Visit(FunctionExpressionNode* node) { return this->Visit((ExpressionNode*)node); }
  virtual VisitResult Visit(FunctionCallNode* node) { return this->Visit((ExpressionNode*)node); }
  virtual VisitResult Visit(PrefixExpressionNode* node) { return this->Visit((ExpressionNode*)node); }
  virtual VisitResult Visit(UnaryOperatorNode* node) { return this->Visit((ExpressionNode*)node); }
  virtual VisitResult Visit(BinaryOperatorNode* node) { return this->Visit((ExpressionNode*)node); }
  virtual VisitResult Visit(FunctionNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(FunctionNameNode* node) { return this->Visit((AbstractNode*)node); }
  virtual VisitResult Visit(WhileNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(RepeatNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(ForNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(GenericForNode* node) { return this->Visit((ForNode*)node); }
  virtual VisitResult Visit(NumericForNode* node) { return this->Visit((ForNode*)node); }
  virtual VisitResult Visit(IfNode* node) { return this->Visit((StatementNode*)node); }
  virtual VisitResult Visit(LocalVariableNode* node) { return this->Visit((StatementNode*)node); }
};


// All of our node types inherit from this node and implement the Walk function
class AbstractNode
{
public:
  virtual ~AbstractNode()
  {}

  //Location in the document
  DocumentPosition Position;

  //Parent of this node
  AbstractNode* Parent = nullptr;

  virtual void Walk(Visitor* visitor, bool visit = true);

};


class StatementNode : public AbstractNode
{
public:

  void Walk(Visitor* visitor, bool visit = true) override;
};

class BlockNode : public StatementNode
{
public:
  // Collection of all statements in a block
  unique_vector<StatementNode> Statements;

  //Used to help with autocomplete
  std::unique_ptr<AbstractNode> End;

  void Walk(Visitor* visitor, bool visit = true) override;

  //Semantic, holds all local symbols
  std::vector<Symbol *> localSymbols;
};

//Not needed in initial ast, but will be needed for later
class TypeNode : public AbstractNode
{
public:
  Token Name;
  size_t PointerCount;

  void Walk(Visitor* visitor, bool visit = true) override;

  //* Semantic Analysis *//
  // The symbol we resolved
  // Note that symbol resolution may actually need to create a type
  // For example, a pointer to a type may need to create a symbol
  Type* SEM_Symbol;
};

class AssignmentNode : public StatementNode
{
public:
  Token Operator;
  std::vector<Token> Names;

  unique_vector<VariableStatementNode> LeftVariables;
  unique_vector<ExpressionNode> RightExpressions;

  void Walk(Visitor* visitor, bool visit = true) override;

  //Semantic
  std::vector<Symbol *> SEM_Variables;
};

class VariableNode : public AbstractNode
{
public:
  void Walk(Visitor* visitor, bool visit = true) override;
  
  Type *SEM_ResolvedSymbol;
  Variable *SEM_Variable;
};

class VariableStatementNode : public VariableNode
{
public:
  //IdentifiedVariableNode | ExpressionVariableNode
  std::unique_ptr<VariableNode> Variable;
  std::unique_ptr<VariableSuffixNode> Suffix;
  
  void Walk(Visitor* visitor, bool visit = true) override;
};

class IdentifiedVariableNode : public VariableNode
{
public:
  Token Name;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class ExpressionVariableNode : public VariableNode
{
public:
  std::unique_ptr<ExpressionNode> Expression;
  std::unique_ptr<VariableSuffixNode> Suffix;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class VariableSuffixNode : public AbstractNode
{
public:
  //Referring to the left hand side of this suffix, can be null
  std::unique_ptr<VariableSuffixNode> LeftSuffix;

  //Optional call node
  unique_vector<CallNode> CallNodes;

  //Index node, null means incomplete suffix
  std::unique_ptr<IndexNode> Index;

  void Walk(Visitor* visitor, bool visit = true) override;

  //Symbol this node is referring to
  Symbol* SEM_ResolvedSymbol;
  Variable *SEM_Variable;
};


// Call Nodes //
class CallNode : public AbstractNode
{
public:
  std::unique_ptr<ArgumentNode> Argument;

  void Walk(Visitor* visitor, bool visit = true) override;

  Symbol* SEM_ResolvedSymbol;
};

class MemberCallNode : public CallNode
{
public:
  Token Name;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class ArgumentNode : public AbstractNode
{
public:
  void Walk(Visitor* visitor, bool visit = true) override;
};

class ExpressionArgumentNode : public ArgumentNode
{
public:
  unique_vector<ExpressionNode> ExpressionList;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class TableArgumentNode : public ArgumentNode
{
public:
  std::unique_ptr<TableNode> Table;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class StringArgumentNode : public ArgumentNode
{
public:
  Token String;

  void Walk(Visitor* visitor, bool visit = true) override;
};


class IndexNode : public AbstractNode
{
public:
  void Walk(Visitor* visitor, bool visit = true) override;

  //Symbol this node is referring to
  Variable *SEM_Variable;
};

class IdentifiedIndexNode : public IndexNode
{
public:
  Token Name;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class ExpressionIndexNode : public IndexNode
{
public:
  std::unique_ptr<ExpressionNode> Expression;

  void Walk(Visitor* visitor, bool visit = true) override;
};


class BreakNode : public StatementNode
{
public:
  void Walk(Visitor* visitor, bool visit = true) override;
};

class ReturnNode : public StatementNode
{
public:
  unique_vector<ExpressionNode> ReturnValues;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class ExpressionNode : public StatementNode
{
public:
  Token Expression;

  void Walk(Visitor* visitor, bool visit = true) override;

  Type *SEM_ResolvedType;
  ValueData SEM_Value;
};

class ValueNode : public ExpressionNode
{
public:
  void Walk(Visitor* visitor, bool visit = true) override;
};

class TableNode : public ExpressionNode
{
public:
  unique_vector<IndexNode> Indicies;
  unique_vector<ExpressionNode> Values;

  //Same as Indicies and Values, but includes null
  std::vector<IndexNode *> FullIndicies;
  std::vector<ExpressionNode *> FullValues;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class FunctionExpressionNode : public ExpressionNode
{
public:
  std::unique_ptr<FunctionNode> Function;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class FunctionCallNode : public ExpressionNode
{
public:
  std::unique_ptr<VariableStatementNode> Variable;
  unique_vector<CallNode> Calls;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class PrefixExpressionNode : public ExpressionNode
{
public:
  std::unique_ptr<VariableStatementNode> LeftVar;
  unique_vector<CallNode> RightCalls;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class UnaryOperatorNode : public ExpressionNode
{
public:
  Token Operator;
  std::unique_ptr<ExpressionNode> Right;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class BinaryOperatorNode : public ExpressionNode
{
public:
  Token Operator;
  std::unique_ptr<ExpressionNode> Left;
  std::unique_ptr<ExpressionNode> Right;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class FunctionNode : public StatementNode
{
public:
  bool IsLocal = false;

  //Can be empty
  unique_vector<FunctionNameNode> Name;

  std::vector<Token> ParameterList;

  std::unique_ptr<BlockNode> Block;

  void Walk(Visitor* visitor, bool visit = true) override;

  Type *SYM_ReturnType;
  Variable *SYM_Variable;
};

class FunctionNameNode : public AbstractNode
{
public:
  Token Name;
  bool IsMemberFunc = false;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class WhileNode : public StatementNode
{
public:
  std::unique_ptr<ExpressionNode> Condition;

  std::unique_ptr<BlockNode> Block;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class RepeatNode : public StatementNode
{
public:
  std::unique_ptr<BlockNode> Block;

  std::unique_ptr<ExpressionNode> Condition;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class IfNode : public StatementNode
{
public:
  //Can be null
  std::unique_ptr<ExpressionNode> Condition;

  std::unique_ptr<BlockNode> Block;

  std::unique_ptr<IfNode> Else;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class ForNode : public StatementNode
{
public:

  std::unique_ptr<BlockNode> Block;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class NumericForNode : public ForNode
{
public:
  Token VarName;

  std::unique_ptr<ExpressionNode> Var;
  std::unique_ptr<ExpressionNode> Limit;
  std::unique_ptr<ExpressionNode> Step;


  void Walk(Visitor* visitor, bool visit = true) override;
};

class GenericForNode : public ForNode
{
public:
  std::vector<Token> Names;
  unique_vector<ExpressionNode> ExpressionList;

  void Walk(Visitor* visitor, bool visit = true) override;
};

class LocalVariableNode : public StatementNode
{
public:
  std::vector<Token> Names;
  unique_vector<ExpressionNode> ExpressionList;

  void Walk(Visitor* visitor, bool visit = true) override;
};

//  Print Visitor //
class NodePrinter
{
public:
  std::stringstream stream;

  friend std::ostream& operator<< (std::ostream& outStream, const NodePrinter& printer);

  NodePrinter()
  {
    for (size_t i = 0; i < ActiveNodes.size(); ++i)
      (stream) << "| ";

    ActiveNodes.push_back(this);
  }

  template<typename T>
  NodePrinter & operator<<(T const &rhs)
  {
    stream << rhs;
    //my_log("%s\n", stream.str().c_str());
    return *this;
  }

  NodePrinter(const std::string& text)
  {
    for (size_t i = 0; i < ActiveNodes.size(); ++i)
      (stream) << "| ";

    ActiveNodes.push_back(this);
    (stream) << text;
  }

  ~NodePrinter()
  {
    ActiveNodes.pop_back();

    if (ActiveNodes.empty())
      my_log("%s\n", stream.str().c_str());
    else
      (*ActiveNodes.back()) << "\n" << stream.str();
  }

private:
  static std::vector<NodePrinter*> ActiveNodes;
};

void ResolveTypes(AbstractNode *ast, Library *lib, LibraryReference *libRef);
void PrintTypes(AbstractNode *ast);