#include "Lexer_DFA.h"
#include <functional>
#include <vector>

namespace Lexer
{
  const char* keywords[] =
  {
    // The TOKEN macro is used like TOKEN(Class, "class")
#define TOKEN(Name, Value) Value,
#include "Tokens_Keyword.inl"
#undef TOKEN
  };

  Token::Type::Enum keyword_type[] =
  {
    // The TOKEN macro is used like TOKEN(Class, "class")
#define TOKEN(Name, Value) Token::Type::Name,
#include "Tokens_Keyword.inl"
#undef TOKEN
  };

  const char* symbols[] =
  {
    // The TOKEN macro is used like TOKEN(Class, "class")
#define TOKEN(Name, Value) Value,
#include "Tokens_Symbol.inl"
#undef TOKEN
  };

  Token::Type::Enum symbol_type[] =
  {
    // The TOKEN macro is used like TOKEN(Class, "class")
#define TOKEN(Name, Value) Token::Type::Name,
#include "Tokens_Symbol.inl"
#undef TOKEN
  };

  DfaState* Lexer_CreateState(int acceptingToken)
  {
    return new DfaState(acceptingToken);
  }

  void AddEdge(DfaState* from, DfaState* to, char c)
  {
    from->edges.insert(std::make_pair(c, to));
  }

  void AddDefaultEdge(DfaState* from, DfaState* to)
  {
    from->default_edge = to;
  }

  bool InsertState(DfaState *new_state, std::vector<DfaState *> &all_states)
  {
    bool found = false;
    for (DfaState *state : all_states)
    {
      if (state == new_state)
      {
        found = true;
        break;
      }
    }

    if (!found)
      all_states.push_back(new_state);

    return found == false;
  }

  DfaState* TraverseAndCopy(DfaState *state, std::vector<std::pair<DfaState *, DfaState *>> &replacements, std::vector<DfaState *> &all_states)
  {
    //If state exists AND haven't visited
    if (state && InsertState(state, all_states))
    {
      //Create a new copy of the state, add it to list of replacements
      DfaState *state_copy = new DfaState(state->acceptingToken);
      replacements.push_back(std::make_pair(state, state_copy));

      //Walk Edges
      for (auto pair : state->edges)
      {
        DfaState *copy = TraverseAndCopy(pair.second, replacements, all_states);

        //If copy is null it wasn't copied, we need to find its replacement
        if (copy == nullptr)
        {
          for (auto &&replacement_pair : replacements)
          {
            if (replacement_pair.first == pair.second)
            {
              copy = replacement_pair.second;
              break;
            }
          }
        }

        //Duplicate the edge with the deep copied node
        state_copy->edges.insert(std::make_pair(pair.first, copy));
      }


      //Deep copy failure edge
      DfaState *copy = TraverseAndCopy(state->failure_edge, replacements, all_states);

      //If copy is null it wasn't copied, we need to find its replacement
      if (copy == nullptr)
      {
        for (auto &&replacement_pair : replacements)
        {
          if (replacement_pair.first == state->failure_edge)
          {
            copy = replacement_pair.second;
            break;
          }
        }
      }

      state_copy->failure_edge = copy;


      //Finally, deep copy default edge
      copy = TraverseAndCopy(state->default_edge, replacements, all_states);

      //If copy is null it wasn't copied, we need to find its replacement
      if (copy == nullptr)
      {
        for (auto &&replacement_pair : replacements)
        {
          if (replacement_pair.first == state->default_edge)
          {
            copy = replacement_pair.second;
            break;
          }
        }
      }

      state_copy->default_edge = copy;

      return state_copy;
    }
    else
      return nullptr;
  }

  DfaState *DeepCopyWithReplacement(DfaState *state)
  {
    std::vector<DfaState *> all_states;
    std::vector<std::pair<DfaState *, DfaState *>> replacements;

    return TraverseAndCopy(state, replacements, all_states);
  }

  void ParseToken(DfaState* startingState, const char* stream, Token& outToken, unsigned &lineNumber, unsigned &charNumber)
  {
    const char *begin = stream;
    const char *lastAcceptedPosition = stream;
    DfaState *lastAcceptedState = nullptr;
    DfaState *currentState = startingState;
    unsigned lastCharNumber = charNumber;

    while (currentState != nullptr && *stream != 0)
    {
      auto it = currentState->edges.find(*stream);
      if (*stream == '\n')
      {
        lineNumber++;
        lastCharNumber = charNumber;
        charNumber = 0;
      }

      stream++;

      if (it != currentState->edges.end())
      {
        currentState = it->second;
      }
      else
      {
        //Prefer failure edge, then default edge
        if (currentState->failure_edge)
        {
          //Failure edge needs to undo the consumed character, and redirect to a new state
          stream--;

          if (*stream == '\n')
          {
            charNumber = lastCharNumber;
            lineNumber--;
          }

          currentState = currentState->failure_edge;
        }
        else
        {
          currentState = currentState->default_edge;
        }
      }

      if (currentState && currentState->acceptingToken != 0)
      {
        lastAcceptedPosition = stream;
        lastAcceptedState = currentState;
      }

      if (currentState != nullptr && *stream != 0)
        charNumber++;
    }

    if (stream != begin)
    {
      stream--;

      if (*stream == '\n')
      {
        lineNumber--;
        charNumber = lastCharNumber;
      }
    }

    if (lastAcceptedState != nullptr)
    {
      outToken.Text = begin;
      outToken.Length = lastAcceptedPosition - begin;
      outToken.TokenType = lastAcceptedState->acceptingToken;
      outToken.Position = DocumentPosition(lineNumber, charNumber - 1);
    }
    else
    {
      outToken.Text = begin;
      outToken.Length = stream - begin;
      outToken.TokenType = 0;
      outToken.Position = DocumentPosition(lineNumber, charNumber - 1);
    }
  }

  void TraverseState(DfaState *state, std::vector<DfaState *> &all_states)
  {
    if (state && InsertState(state, all_states))
    {
      TraverseState(state->default_edge, all_states);
      for (auto pair : state->edges)
      {
        TraverseState(pair.second, all_states);
      }
    }
  }

  void DeleteStateAndChildren(DfaState* root)
  {
    std::vector<DfaState *> all_states;

    TraverseState(root, all_states);

    for (DfaState *state : all_states)
    {
      delete state;
    }
  }

  void ReadToken(DfaState* startingState, const char* stream, Token& outToken, unsigned &lineNumber, unsigned &charNumber)
  {
    ParseToken(startingState, stream, outToken, lineNumber, charNumber);

    if (outToken.TokenType == Token::Type::Identifier)
    {
      std::string text(outToken.Text, outToken.Text + outToken.Length);
      auto it = startingState->keywords.find(text);
      if (it != startingState->keywords.end())
      {
        outToken.TokenType = it->second;
      }
    }
  }

  DfaState* CreateLanguageDfa()
  {
    DfaState *root = Lexer_CreateState(0);

    //Handle Keywords
    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i)
    {
      root->keywords.insert(std::make_pair(keywords[i], keyword_type[i]));
    }

    //Handle symbols
    std::unordered_map<char, DfaState *> symbol_states;
    for (int i = 0; i < sizeof(symbols) / sizeof(symbols[0]); ++i)
    {
      std::string symbol(symbols[i]);

      DfaState *state = nullptr;
      char first_char = symbol[0];
      auto it = symbol_states.find(first_char);
      if (it != symbol_states.end())
      {
        state = it->second;
      }
      else
      {
        state = Lexer_CreateState(0);
        AddEdge(root, state, first_char);
        symbol_states.insert(std::make_pair(first_char, state));
      }

      //Only 1 character
      if (symbol.size() == 1)
      {
        state->acceptingToken = symbol_type[i];
      }
      else
      {
        for (unsigned sym = 1; sym < symbol.size(); ++sym)
        {
          //Find the next character in the sequence
          auto symbol_it = state->edges.find(symbol[sym]);

          //If we find the character, traverse the tree. Otherwise create it
          if (symbol_it != state->edges.end())
          {
            state = symbol_it->second;
          }
          else
          {
            DfaState *new_state = Lexer_CreateState(0);
            AddEdge(state, new_state, symbol[sym]);
            state = new_state;
          }
        }

        state->acceptingToken = symbol_type[i];
      }
    }

    //Whitespace
    DfaState *whitespace = Lexer_CreateState(Token::Type::Whitespace);
    AddEdge(root, whitespace, ' ');
    AddEdge(root, whitespace, '\t');
    AddEdge(root, whitespace, '\r');
    AddEdge(root, whitespace, '\n');

    AddEdge(whitespace, whitespace, ' ');
    AddEdge(whitespace, whitespace, '\t');
    AddEdge(whitespace, whitespace, '\r');
    AddEdge(whitespace, whitespace, '\n');

    //Identifier
    DfaState *identifier = Lexer_CreateState(Token::Type::Identifier);
    AddEdge(root, identifier, '_');
    for (char c = 'a'; c <= 'z'; ++c)
      AddEdge(root, identifier, c);
    for (char c = 'A'; c <= 'Z'; ++c)
      AddEdge(root, identifier, c);


    AddEdge(identifier, identifier, '_');
    for (char c = 'a'; c <= 'z'; ++c)
      AddEdge(identifier, identifier, c);
    for (char c = 'A'; c <= 'Z'; ++c)
      AddEdge(identifier, identifier, c);
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(identifier, identifier, c);


    //Need a specific state for hex later, to get 0x...
    DfaState *int_literal_0 = Lexer_CreateState(Token::Type::IntegerLiteral);
    AddEdge(root, int_literal_0, '0');

    //Integer Literal
    DfaState *int_literal = Lexer_CreateState(Token::Type::IntegerLiteral);
    for (char c = '1'; c <= '9'; ++c)
      AddEdge(root, int_literal, c);

    for (char c = '0'; c <= '9'; ++c)
    {
      AddEdge(int_literal, int_literal, c);
      AddEdge(int_literal_0, int_literal, c);
    }

    //Float Literal
    DfaState *decimal = Lexer_CreateState(0);
    AddEdge(int_literal_0, decimal, '.');
    AddEdge(int_literal, decimal, '.');

    DfaState *double_literal = Lexer_CreateState(Token::Type::FloatLiteral);
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(decimal, double_literal, c);
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(double_literal, double_literal, c);

    //Add in from . operator to float
    auto dot_it = root->edges.find('.');
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(dot_it->second, double_literal, c);

    DfaState *exponential_start = Lexer_CreateState(0);
    AddEdge(int_literal, exponential_start, 'e');
    AddEdge(int_literal, exponential_start, 'E');
    AddEdge(double_literal, exponential_start, 'e');
    AddEdge(double_literal, exponential_start, 'E');

    DfaState *exponential_sign = Lexer_CreateState(0);
    AddEdge(exponential_start, exponential_sign, '-');

    DfaState *exponential = Lexer_CreateState(Token::Type::FloatLiteral);
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(exponential_start, exponential, c);
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(exponential_sign, exponential, c);
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(exponential, exponential, c);

    //Hex Literal
    DfaState *hex_start = Lexer_CreateState(0);
    AddEdge(int_literal_0, hex_start, 'x');

    DfaState *hex_firstchar = Lexer_CreateState(Token::Type::IntegerLiteral);
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(hex_start, hex_firstchar, c);
    for (char c = 'a'; c <= 'f'; ++c)
      AddEdge(hex_start, hex_firstchar, c);

    for (char c = '0'; c <= '9'; ++c)
      AddEdge(hex_firstchar, hex_firstchar, c);
    for (char c = 'a'; c <= 'f'; ++c)
      AddEdge(hex_firstchar, hex_firstchar, c);

    //-------- Strings --------//
    //String Literal
    DfaState *string_begin = Lexer_CreateState(0);
    DfaState *string_escape = Lexer_CreateState(0);
    DfaState *string_end = Lexer_CreateState(Token::Type::StringLiteral);

    AddEdge(root, string_begin, '\"');
    AddDefaultEdge(string_begin, string_begin);
    AddEdge(string_begin, string_end, '\"');
    AddEdge(string_begin, string_escape, '\\');

    //Normal escape
    AddEdge(string_escape, string_begin, 'b');
    AddEdge(string_escape, string_begin, 't');
    AddEdge(string_escape, string_begin, 'n');
    AddEdge(string_escape, string_begin, 'f');
    AddEdge(string_escape, string_begin, 'r');
    AddEdge(string_escape, string_begin, '\"');
    AddEdge(string_escape, string_begin, '\'');
    AddEdge(string_escape, string_begin, '\\');

    //Unicode escape
    DfaState *escape_unicode_start = Lexer_CreateState(0);
    DfaState *escape_unicode_hexdigit_array[1] = { Lexer_CreateState(0) };

    AddEdge(string_escape, escape_unicode_start, 'x');
    
    //Once the 'u' is found, we need 4 hex numbers, then it's valid
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(escape_unicode_start, escape_unicode_hexdigit_array[0], c);
    for (char c = 'a'; c <= 'f'; ++c)
      AddEdge(escape_unicode_start, escape_unicode_hexdigit_array[0], c);
    for (char c = 'A'; c <= 'F'; ++c)
      AddEdge(escape_unicode_start, escape_unicode_hexdigit_array[0], c);

    //Back to string from escape
    for (char c = '0'; c <= '9'; ++c)
      AddEdge(escape_unicode_hexdigit_array[0], string_begin, c);
    for (char c = 'a'; c <= 'f'; ++c)
      AddEdge(escape_unicode_hexdigit_array[0], string_begin, c);
    for (char c = 'A'; c <= 'F'; ++c)
      AddEdge(escape_unicode_hexdigit_array[0], string_begin, c);

    //Octal escape
    DfaState *escape_octal_1 = Lexer_CreateState(0);
    DfaState *escape_octal_2 = Lexer_CreateState(0);

    //Just 1 or 2 digits is considered complete for an octal number
    escape_octal_1->failure_edge = string_begin;
    escape_octal_2->failure_edge = string_begin;

    for (char c = '0'; c <= '7'; ++c)
      AddEdge(string_escape, escape_octal_1, c);

    for (char c = '0'; c <= '7'; ++c)
      AddEdge(escape_octal_1, escape_octal_2, c);

    for (char c = '0'; c <= '7'; ++c)
      AddEdge(escape_octal_2, string_begin, c);

    
    //Character strings
    DfaState *charstring_begin = DeepCopyWithReplacement(string_begin);
    DfaState *charstring_end = nullptr; //Filled in

    //Replace ending " with '
    auto char_it = charstring_begin->edges.find('\"');
    charstring_end = char_it->second;
    charstring_begin->edges.insert(std::make_pair('\'', charstring_end));
    charstring_begin->edges.erase('\"');

    //Beginning '
    AddEdge(root, charstring_begin, '\'');

    //Long strings
    //Need to get the OpenBracket token to start the long string
    auto bracket_it = root->edges.find('[');

    DfaState *longstring_begin = bracket_it->second;
    DfaState *longstring_begin_equals = Lexer_CreateState(0);
    DfaState *longstring_string = nullptr; //Filled in
    DfaState *longstring_end_equals = nullptr; //Filled in
    DfaState *longstring_end = Lexer_CreateState(Token::Type::StringLiteral);

    //First duplicate string_begin for the inner part of long string
    longstring_string = DeepCopyWithReplacement(string_begin);

    //Replace ending " with ]
    auto found_it = longstring_string->edges.find('\"');
    longstring_end_equals = found_it->second;
    longstring_string->edges.insert(std::make_pair(']', longstring_end_equals));
    longstring_string->edges.erase('\"');

    //Beginning [
    AddEdge(longstring_begin, longstring_string, '[');

    // 0 or more =
    AddEdge(longstring_begin, longstring_begin_equals, '=');
    AddEdge(longstring_begin_equals, longstring_begin_equals, '=');
    AddEdge(longstring_begin_equals, longstring_string, '[');

    //Ending ] with one or more =, but with no final ] return to normal string
    longstring_end_equals->failure_edge = longstring_string;

    AddEdge(longstring_end_equals, longstring_end_equals, '=');
    AddEdge(longstring_end_equals, longstring_end, ']');


    //-------- Comments --------//

    //Single line comments
    auto minus_it = root->edges.find('-');
    DfaState *single_comment_begin = minus_it->second;
    DfaState *single_comment_maybe_long = Lexer_CreateState(Token::Type::Comment);
    DfaState *single_comment = Lexer_CreateState(Token::Type::Comment);
    DfaState *single_comment_optinonal_r = Lexer_CreateState(Token::Type::Comment);
    DfaState *single_comment_end = Lexer_CreateState(Token::Type::Comment);

    //Add edge for --
    AddEdge(single_comment_begin, single_comment_maybe_long, '-');

    //Single line comments go until a newline
    AddDefaultEdge(single_comment, single_comment);
    AddEdge(single_comment, single_comment_optinonal_r, '\r');
    AddEdge(single_comment, single_comment_end, '\n');

    single_comment_maybe_long->failure_edge = single_comment;

    AddEdge(single_comment_optinonal_r, single_comment_end, '\n');

    //Long comments
    DfaState *long_comment_start = Lexer_CreateState(0);
    AddEdge(single_comment_maybe_long, long_comment_start, '[');

    //Second [
    DfaState *long_comment = Lexer_CreateState(0);
    AddEdge(long_comment_start, long_comment, '[');

    //Loop on self, eating characters until ]
    AddDefaultEdge(long_comment, long_comment);

    // First ] of end
    DfaState *long_comment_endBracket = Lexer_CreateState(0);
    long_comment_endBracket->failure_edge = long_comment;
    AddEdge(long_comment, long_comment_endBracket, ']');

    // ]] to end
    DfaState *long_comment_end = Lexer_CreateState(Token::Type::Comment);
    AddEdge(long_comment_endBracket, long_comment_end, ']');

    return root;
  }
}