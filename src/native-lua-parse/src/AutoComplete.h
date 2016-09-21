#pragma once
#include "AST_Nodes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

enum class CompletionItemKind {
  Text = 1,
  Method = 2,
  Function = 3,
  Constructor = 4,
  Field = 5,
  Variable = 6,
  Class = 7,
  Interface = 8,
  Module = 9,
  Property = 10,
  Unit = 11,
  Value = 12,
  Enum = 13,
  Keyword = 14,
  Snippet = 15,
  Color = 16,
  File = 17,
  Reference = 18
};

struct AutoCompleteEntry
{
  std::string name;
  CompletionItemKind entryKind = CompletionItemKind::Text;
};



void ResolveAutocomplete(AbstractNode *ast, int lineNumber, int charNumber, std::vector<AutoCompleteEntry> &output, Library *lib, std::vector<Token> &tokens);