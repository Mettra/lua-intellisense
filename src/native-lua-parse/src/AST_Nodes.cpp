#include "AST_Nodes.h"

// Walk Functions //
void AbstractNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
}

void StatementNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;

  AbstractNode::Walk(visitor, false);
}

void BlockNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;

  StatementNode::Walk(visitor, false);

  for (auto &node : this->Statements)
  {
    node->Walk(visitor);
  }

  if(this->End)
    this->End->Walk(visitor);
}

void TypeNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;

  AbstractNode::Walk(visitor, false);
}

void AssignmentNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;

  StatementNode::Walk(visitor, false);

  for (auto &node : this->LeftVariables)
  {
    node->Walk(visitor);
  }

  for (auto &node : this->RightExpressions)
  {
    node->Walk(visitor);
  }
}

void VariableNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;

  AbstractNode::Walk(visitor, false);
}

void VariableStatementNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;

  VariableNode::Walk(visitor, false);

  if(this->Variable)
    this->Variable->Walk(visitor);

  if (this->Suffix)
    this->Suffix->Walk(visitor);
}

void IdentifiedVariableNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;

  VariableNode::Walk(visitor, false);

  //@REMIND Type node?
}

void ExpressionVariableNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;

  VariableNode::Walk(visitor, false);

  this->Expression->Walk(visitor);

  if (this->Suffix)
    this->Suffix->Walk(visitor);
}

void VariableSuffixNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  AbstractNode::Walk(visitor, false);

  if (this->LeftSuffix)
    this->LeftSuffix->Walk(visitor);

  for (auto &node : this->CallNodes)
  {
    node->Walk(visitor);
  }

  if (this->Index)
    this->Index->Walk(visitor);
}

void CallNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  AbstractNode::Walk(visitor, false);

  if(this->Argument)
    this->Argument->Walk(visitor);
}

void MemberCallNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  CallNode::Walk(visitor, false);
}

void ArgumentNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  AbstractNode::Walk(visitor, false);
}

void ExpressionArgumentNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ArgumentNode::Walk(visitor, false);

  for (auto &node : this->ExpressionList)
  {
    node->Walk(visitor);
  }
}

void TableArgumentNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ArgumentNode::Walk(visitor, false);

  this->Table->Walk(visitor);
}

void StringArgumentNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ArgumentNode::Walk(visitor, false);
}

void IndexNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  AbstractNode::Walk(visitor, false);
}

void IdentifiedIndexNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  IndexNode::Walk(visitor, false);
}

void ExpressionIndexNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  IndexNode::Walk(visitor, false);

  this->Expression->Walk(visitor);
}

void BreakNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  StatementNode::Walk(visitor, false);
}

void ReturnNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  StatementNode::Walk(visitor, false);

  for (auto &node : this->ReturnValues)
  {
    node->Walk(visitor);
  }
}

void ExpressionNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  StatementNode::Walk(visitor, false);
}

void ValueNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ExpressionNode::Walk(visitor, false);
}

void TableNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ExpressionNode::Walk(visitor, false);

  for (auto &node : this->Indicies)
  {
    node->Walk(visitor);
  }

  for (auto &node : this->Values)
  {
    node->Walk(visitor);
  }
}

void FunctionExpressionNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ExpressionNode::Walk(visitor, false);

  this->Function->Walk(visitor);
}

void FunctionCallNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ExpressionNode::Walk(visitor, false);

  this->Variable->Walk(visitor);

  for (auto &node : this->Calls)
  {
    node->Walk(visitor);
  }
}

void PrefixExpressionNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ExpressionNode::Walk(visitor, false);

  this->LeftVar->Walk(visitor);

  for (auto &node : this->RightCalls)
  {
    node->Walk(visitor);
  }
}

void UnaryOperatorNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ExpressionNode::Walk(visitor, false);

  this->Right->Walk(visitor);
}

void BinaryOperatorNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ExpressionNode::Walk(visitor, false);

  this->Left->Walk(visitor);
  this->Right->Walk(visitor);
}

void FunctionNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  StatementNode::Walk(visitor, false);

  for (auto &node : this->Name)
  {
    node->Walk(visitor);
  }

  this->Block->Walk(visitor);
}

void FunctionNameNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  AbstractNode::Walk(visitor, false);
}

void WhileNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  StatementNode::Walk(visitor, false);

  this->Condition->Walk(visitor);
  this->Block->Walk(visitor);
}

void RepeatNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  StatementNode::Walk(visitor, false);

  this->Block->Walk(visitor);
  this->Condition->Walk(visitor);
}

void IfNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  StatementNode::Walk(visitor, false);

  if (this->Condition)
    this->Condition->Walk(visitor);

  this->Block->Walk(visitor);

  if (this->Else)
    this->Else->Walk(visitor);
}

void ForNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;

  StatementNode::Walk(visitor, false);

  this->Block->Walk(visitor);
}

void NumericForNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ForNode::Walk(visitor, false);

  this->Var->Walk(visitor);
  this->Limit->Walk(visitor);

  if (this->Step)
    this->Step->Walk(visitor);

  this->Block->Walk(visitor);
}

void GenericForNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  ForNode::Walk(visitor, false);

  for (auto &node : this->ExpressionList)
  {
    node->Walk(visitor);
  }

  this->Block->Walk(visitor);
}

void LocalVariableNode::Walk(Visitor* visitor, bool visit)
{
  if (visit && visitor->Visit(this) == VisitResult::Stop)
    return;
  StatementNode::Walk(visitor, false);

  for (auto &node : this->ExpressionList)
  {
    node->Walk(visitor);
  }
}