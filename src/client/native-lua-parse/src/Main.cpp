#include "Lexer_DFA.h"
#include "Descent_Parser.h"
#include "TypeSystem.h"
#include "AutoComplete.h"

#include <string>
#include <fstream>
#include <streambuf>

#include "dirent.h"

// When printing a character it can get pretty annoying if we print actual newlines (we want to print escaped versions instead)
const char* Escape(char c)
{
  unsigned char index = (unsigned char)c;

  // Make an entry for every character + null terminator
  static char characterStrings[256 * 2] = { 0 };
  static bool initialized = false;

  // Initilaize our table of character strings (this is so we don't have to allocate strings)
  if (initialized == false)
  {
    initialized = true;
    for (size_t i = 0; i < 256; ++i)
      characterStrings[i * 2] = (char)i;
  }

  // If we encounter these special characters, we want to print out the escaped version
  switch (c)
  {
  case '\0':
    return "\\0";
  case '\a':
    return "\\a";
  case '\b':
    return "\\b";
  case '\f':
    return "\\f";
  case '\n':
    return "\\n";
  case '\r':
    return "\\r";
  case '\t':
    return "\\t";
  case '\v':
    return "\\v";
  default:
    return characterStrings + index * 2;
  }
}

void Lexer_RunTest(int part, int test, Lexer::DfaState* root, const char *filename)
{
  //Read in file
  std::ifstream t(filename);

  std::string str((std::istreambuf_iterator<char>(t)),
    std::istreambuf_iterator<char>());

  my_log("************** PART %d TEST %d **************\n", part, test);

  const char *stream = str.c_str();
  unsigned lineNumber = 1;
  unsigned charNumber = 0;
  // Read until we exhaust the stream
  while (*stream != '\0')
  {
    Token token;
    Lexer::ReadToken(root, stream, token, lineNumber, charNumber);

    std::string escapedText;
    for (size_t i = 0; i < token.Length; ++i)
      escapedText += Escape(token.Text[i]);

    my_log("Token: '%s' of type %d (%s)\n", escapedText.c_str(), token.TokenType, TokenNames[token.TokenType]);
    stream += token.Length;

    if (token.Length == 0)
    {
      my_log("Skipping one character of input: '%s'\n", Escape(*stream));
      ++stream;
    }
  }

  my_log("*******************************************\n\n");
}

void ParseStream(const char *stream, Lexer::DfaState* root, std::vector<Token> &tokens)
{
  unsigned lineNumber = 0;
  unsigned charNumber = 0;
  while (*stream != '\0')
  {
    Token token;
    Lexer::ReadToken(root, stream, token, lineNumber, charNumber);
    stream += token.Length;

    if (token.Length != 0)
      tokens.push_back(token);
  }

  //Correct line numbers, then remove them
  //std::vector<int> line_indecies;
  //int line_num = 1;
  //
  //for (int i = 0; i < tokens.size(); ++i)
  //{
  //  tokens[i].Line = line_num;
  //  if (tokens[i].EnumTokenType == Token::Type::Newline)
  //  {
  //    tokens.erase(tokens.begin() + i);
  //    --i;
  //    line_num++;
  //  }
  //}
}

void Parser_RunTest(int part, int test, Lexer::DfaState* root, const char *filename, bool throwException)
{
  //Read in file
  std::ifstream t(filename);

  std::string str((std::istreambuf_iterator<char>(t)),
    std::istreambuf_iterator<char>());

  my_log("************** PART %d TEST %d **************\n", part, test);

  std::vector<Token> tokens;
  ParseStream(str.c_str(), root, tokens);

  my_log("**************      PARSER     **************\n");

  std::unique_ptr<AbstractNode> ast = nullptr;
  std::vector<ParsingException> errors;

  RemoveWhitespaceAndComments(tokens);
  ast = RecognizeTokens(tokens, &errors, throwException);

  if (errors.size() > 0)
  {
    my_log("Parsing Failed!\n");
    for (auto && e : errors)
    {
      my_log("Error (%s:%d: %s)\n", filename, e.position.Line, e.what());
    }
  }
  else
    my_log("Parsing Successful\n");

  PrintTree(ast.get());

  my_log("*******************************************\n\n");
}

void Type_RunTest(int part, int test, Lexer::DfaState* root, const char *filename, bool throwException)
{
  //Read in file
  std::ifstream t(filename);

  std::string str((std::istreambuf_iterator<char>(t)),
    std::istreambuf_iterator<char>());


  my_log("************** PART %d TEST %d **************\n", part, test);

  std::vector<Token> tokens;
  ParseStream(str.c_str(), root, tokens);

  std::unique_ptr<AbstractNode> ast;
  std::vector<ParsingException> errors;

  RemoveWhitespaceAndComments(tokens);
  ast = RecognizeTokens(tokens, &errors, throwException);

  if (errors.size() > 0)
  {
    my_log("Parsing Failed!\n");
    for (auto && e : errors)
    {
      my_log("Error (%s:%d: %s)\n", filename, e.position.Line, e.what());
    }
  }
  else
    my_log("Parsing Successful\n");

  my_log("**************       TYPE      **************\n");
  LibraryReference libRef;
  ResolveTypes(ast.get(), CreateCoreLibrary(), &libRef);
  PrintTypes(ast.get());

  my_log("*******************************************\n\n");
}

void Autocomplete_RunTest(int part, int test, Lexer::DfaState* root, const char *filename, int lineNumber, int charNumber, bool throwException)
{
  //Read in file
  std::ifstream t(filename);

  std::string str((std::istreambuf_iterator<char>(t)),
    std::istreambuf_iterator<char>());


  my_log("************** PART %d TEST %d **************\n", part, test);

  std::vector<Token> tokens;
  ParseStream(str.c_str(), root, tokens);

  std::unique_ptr<AbstractNode> ast;
  std::vector<ParsingException> errors;

  RemoveWhitespaceAndComments(tokens);
  ast = RecognizeTokens(tokens, &errors, throwException);

  if (errors.size() > 0)
  {
    my_log("Parsing Failed!\n");
    for (auto && e : errors)
    {
      my_log("Error (%s:%d: %s)\n", filename, e.position.Line, e.what());
    }
  }
  else
    my_log("Parsing Successful\n");

  my_log("**************       AUTOCOMPLETE      **************\n");
  Library *lib = CreateCoreLibrary();
  LibraryReference libRef;
  ResolveTypes(ast.get(), lib, &libRef);
  PrintTypes(ast.get());

  std::vector<AutoCompleteEntry> entries;
  ResolveAutocomplete(ast.get(), lineNumber, charNumber, entries, lib, tokens);

  my_log("\n");
  my_log("Found Entries:\n");
  for (auto &&entry : entries)
  {
    my_log("  %s\n", entry.name.c_str());
  }
  my_log("\n");

  my_log("*******************************************\n\n");
}

void AutocompleteMultiFile_RunTest(int part, int test, Lexer::DfaState* root, std::initializer_list<const char *> files, const char *ac_filename, int lineNumber, int charNumber, bool throwException)
{
  Library *masterLibrary = CreateCoreLibrary();

  struct Document
  {
    std::string text;
    std::vector<Token> tokens;
    std::unique_ptr<AbstractNode> ast;
    std::vector<ParsingException> lastErrors;
    Library *masterLibrary;
    LibraryReference ref;
    Lexer::DfaState *root;

    void Parse(std::string documentText)
    {
      text = documentText;
      ParseStream(text.c_str(), root, tokens);

      RemoveWhitespaceAndComments(tokens);
      ast = std::move(RecognizeTokens(tokens, &lastErrors, false));

      if (lastErrors.size() > 0)
      {
        my_log("Parsing Failed!\n");
        for (auto && e : lastErrors)
        {
          my_log("Error (%s:%d: %s)\n", "Test", e.position.Line, e.what());
        }
      }
      else
        my_log("Parsing Successful\n");

      ResolveTypes(ast.get(), masterLibrary, &ref);
      PrintTypes(ast.get());
    }
  };

  std::unordered_map<std::string, Document *> Documents;

  auto parseFile = [&](const char *file) {
    //Read in file
    std::ifstream t(file);

    std::string str((std::istreambuf_iterator<char>(t)),
      std::istreambuf_iterator<char>());

    Document *doc = new Document();
    doc->root = root;
    doc->masterLibrary = masterLibrary;
    doc->Parse(str);

    auto it = Documents.find(file);
    if (it != Documents.end())
    {
      delete it->second;
      it->second = doc;
    }
    else
    {
      Documents.insert(std::make_pair(file, doc));
    }
  };

  for (const char *file : files)
  {
    parseFile(file);
  }

  my_log("**************       AUTOCOMPLETE      **************\n");
  auto it = Documents.find(ac_filename);
  if (it != Documents.end())
  {
    std::vector<AutoCompleteEntry> entries;
    ResolveAutocomplete(it->second->ast.get(), lineNumber, charNumber, entries, masterLibrary, it->second->tokens);

    my_log("\n");
    my_log("Found Entries:\n");
    for (auto &&entry : entries)
    {
      my_log("  %s\n", entry.name.c_str());
    }
    my_log("\n");
  }
  else
  {
    my_log("ERROR! Could not find file with name %s!\n", ac_filename);
  }

  my_log("*******************************************\n\n");
}

void ReferenceCount_RunTest(int part, int test, Lexer::DfaState* root, bool throwException)
{
  Library *masterLibrary = CreateCoreLibrary();

  struct Document
  {
    ~Document()
    {
      my_log("GOODBYE!\n");
    }

    std::string text;
    std::vector<Token> tokens;
    std::unique_ptr<AbstractNode> ast;
    std::vector<ParsingException> lastErrors;
    Library *masterLibrary;
    LibraryReference ref;
    Lexer::DfaState *root;

    void Parse(std::string documentText)
    {
      text = documentText;
      ParseStream(text.c_str(), root, tokens);

      RemoveWhitespaceAndComments(tokens);
      ast = std::move(RecognizeTokens(tokens, &lastErrors, false));

      if (lastErrors.size() > 0)
      {
        my_log("Parsing Failed!\n");
        for (auto && e : lastErrors)
        {
          my_log("Error (%s:%d: %s)\n", "Test", e.position.Line, e.what());
        }
      }
      else
        my_log("Parsing Successful\n");

      ResolveTypes(ast.get(), masterLibrary, &ref);
      //PrintTypes(ast.get());
    }
  };

  std::unordered_map<std::string, std::unique_ptr<Document>> Documents;

  auto parseFile = [&](const char *file) {
    //Read in file
    std::ifstream t(file);

    std::string str((std::istreambuf_iterator<char>(t)),
      std::istreambuf_iterator<char>());

    std::unique_ptr<Document> doc = std::make_unique<Document>();
    doc->root = root;
    doc->masterLibrary = masterLibrary;
    doc->Parse(str);

    auto it = Documents.find(file);
    if (it != Documents.end())
    {
      it->second = std::move(doc);
    }
    else
    {
      Documents.insert(std::make_pair(file, std::move(doc)));
    }
  };

  auto parseString = [&](const char *lua_str, const char *name) {
    std::string str(lua_str);

    std::unique_ptr<Document> doc = std::make_unique<Document>();
    doc->root = root;
    doc->masterLibrary = masterLibrary;
    doc->Parse(str);

    auto it = Documents.find(name);
    if (it != Documents.end())
    {
      it->second = std::move(doc);
    }
    else
    {
      Documents.insert(std::make_pair(name, std::move(doc)));
    }
  };

  auto runAutocomplete = [&](const char *filename, int lineNumber, int charNumber) {
    auto it = Documents.find(filename);
    if (it != Documents.end())
    {
      std::vector<AutoCompleteEntry> entries;
      ResolveAutocomplete(it->second->ast.get(), lineNumber, charNumber, entries, masterLibrary, it->second->tokens);

      //my_log("\n");
      //my_log("Found Entries:\n");
      //for (auto &&entry : entries)
      //{
      //  my_log("  %s\n", entry.name.c_str());
      //}
      //my_log("\n");
    }
    else
    {
      my_log("ERROR! Could not find file with name %s!\n", filename);
    }
  };

  my_log("**************       REFERENCE COUNTING      **************\n");
  parseString("", "f4");
  parseString("GameObject.", "f1");
  parseString("function GameObject.third() self.t_var = 11 end function love.draw() end function love.graphics.one.two.drawmeathing:wattup() end", "f3");
  parseString("GameObject = {} GameObject.position = {} GameObject.position.x = 5 GameObject.position.y = 10 function GameObject:load() self.new_var = 15 end", "f2");
  masterLibrary->Clean();

  runAutocomplete("f1", 0, 20);
  runAutocomplete("f4", 0, 20);
  Documents.erase("f3");
  runAutocomplete("f1", 0, 20);
  Documents.erase("f2");
  runAutocomplete("f1", 0, 20);
  runAutocomplete("f4", 0, 20);
  Documents.erase("f1");
  runAutocomplete("f4", 0, 20);
  Documents.erase("f4");

  //Part 2
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir("TestFolder")) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != NULL) {
      std::string name = ent->d_name;
      if (name != "." && name != "..")
      {
        std::string fullName = "TestFolder/" + name;
        parseFile(fullName.c_str());
      }
    }
    closedir(dir);
  }
  else {
  }

  std::ifstream f1("main_h1.lua");
  std::string h1((std::istreambuf_iterator<char>(f1)),
    std::istreambuf_iterator<char>());

  std::ifstream f2("main_h2.lua");
  std::string h2((std::istreambuf_iterator<char>(f2)),
    std::istreambuf_iterator<char>());

  std::string testString = "TickManager.BlockedMaterialPositions";

  for (int i = 0; i < 2; ++i)
  {
    for (int s = 0; s < testString.size(); ++s)
    {
      std::string newStr = testString.substr(0, s);
      my_log("%s\n", newStr.c_str());

      std::string full = h1 + newStr + h2;
      parseString(full.c_str(), "f1");
      runAutocomplete("f1", 139, newStr.size() - 2);
    }

    runAutocomplete("f1", 139, testString.size() - 2);

    for (int s = testString.size() - 1; s >= 0; --s)
    {
      std::string newStr = testString.substr(0, s);
      my_log("%s\n", newStr.c_str());

      std::string full = h1 + newStr + h2;
      parseString(full.c_str(), "f1");
      runAutocomplete("f1", 139, newStr.size() - 2);
    }

    runAutocomplete("f1", 139, 0);
  }

  my_log("*******************************************\n\n");
}

int main()
{
  Lexer::DfaState *lexer = Lexer::CreateLanguageDfa();

  //Lexer tests
  Lexer_RunTest(1, 1, lexer, "test_names.lua");
  Lexer_RunTest(1, 2, lexer, "test_numbers.lua");
  Lexer_RunTest(1, 3, lexer, "test_strings.lua");
  Lexer_RunTest(1, 3, lexer, "test_comment.lua");
  Lexer_RunTest(1, 4, lexer, "test_keywords.lua");
  
  //Parser tests
  //Parser_RunTest(2, 1, lexer, "Grammer_Test_1.lua", true);

  //Type tests
  //Type_RunTest(3, 1, lexer, "test.lua", true);

  //Autocomplete Tests
  //Autocomplete_RunTest(4, 1, lexer, "main.lua", 140,17, false);

  //Multi-file 436
  AutocompleteMultiFile_RunTest(5, 1, lexer, { "AutocompleteTest.lua", "AutoComplete_GameObject.lua" }, "AutocompleteTest.lua", 9,16, false);

  //Referance counting test
  //ReferenceCount_RunTest(6, 1, lexer, false);

  //@TODO: Blank assignment shouldn't create globals

  Lexer::DeleteStateAndChildren(lexer);
  system("pause");
}