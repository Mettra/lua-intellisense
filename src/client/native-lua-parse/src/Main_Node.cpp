#include <node.h>

#include "Lexer_DFA.h"
#include "Descent_Parser.h"
#include "TypeSystem.h"
#include "AutoComplete.h"
#include "Minidump.h"

#include <memory>
#include <string>
#include <fstream>
#include <future>
#include <thread>
#include <chrono>
#include <streambuf>

#include "Main_Node.hpp"

extern std::string internal_parse_stdout;

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
}

namespace demo {
  Lexer::DfaState *lexer;
  Library *masterLibrary;

#ifdef _WIN32
  Windows::LONG my_handler(Windows::EXCEPTION_POINTERS *ptr)
  {
    WriteDump(ptr);

    return EXCEPTION_CONTINUE_SEARCH;
  }
#endif

  using namespace v8;

  struct Document
  {
    std::string text;
    std::vector<Token> tokens;
    std::unique_ptr<AbstractNode> ast;
    std::vector<ParsingException> lastErrors;
    LibraryReference libRefs;

    void Parse(std::string documentText)
    {
      text = documentText;
      ParseStream(text.c_str(), lexer, tokens);

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

      ResolveTypes(ast.get(), masterLibrary, &libRefs);
    }
  };

  std::unordered_map<std::string, std::unique_ptr<Document>> Documents;


  void ParseDocument(std::string uri, std::string text) {
    std::unique_ptr<Document> doc = std::make_unique<Document>();

    auto it = Documents.find(uri);
    if (it != Documents.end())
    {
      it->second = std::move(doc);
      it->second->Parse(text);
    }
    else
    {
      doc->Parse(text);
      Documents.insert(std::make_pair(uri, std::move(doc)));
    }
  }

  void AutoComplete(std::string uri, unsigned lineNumber, unsigned charNumber, std::vector<AutoCompleteEntry> &entries) {
    auto it = Documents.find(uri);
    if (it != Documents.end())
    {
      Document &doc = *it->second;

      my_log("**************       AUTOCOMPLETE      **************\n");
      ResolveAutocomplete(doc.ast.get(), lineNumber, charNumber, entries, masterLibrary, doc.tokens);
    }
  }

  void WatchFunction_AutoComplete(const FunctionCallbackInfo<Value> &args)
  {
    internal_parse_stdout = "";

    Isolate* isolate = args.GetIsolate();
    String::Utf8Value uriValue(args[0]);

    unsigned lineNumber = args[1]->ToUint32()->Int32Value();
    unsigned charNumber = args[2]->ToUint32()->Int32Value();

    std::string uri(*uriValue);
    std::vector<AutoCompleteEntry> entries;

#ifdef _WIN32
    __try {
#endif
        AutoComplete(uri, lineNumber, charNumber, entries);

#ifdef _WIN32
      }
      __except (my_handler((struct Windows::_EXCEPTION_POINTERS*)Windows::_exception_info())) {}
#endif

	  // We will be creating temporary handles so we use a handle scope.
	  v8::HandleScope handle_scope(isolate);

	  // Create a new empty array.
	  v8::Handle<v8::Array> array = v8::Array::New(isolate, entries.size());
	  unsigned currentIndex = 0;

	  my_log("\n");
	  my_log("Found Entries (%i):\n", entries.size());
	  for (auto &&entry : entries)
	  {
		my_log("  %s\n", entry.name.c_str());

		Local<Object> js_entry = Object::New(isolate);
		js_entry->Set(String::NewFromUtf8(isolate, "label"), String::NewFromUtf8(isolate, entry.name.c_str()));
		js_entry->Set(String::NewFromUtf8(isolate, "data"), v8::Number::New(isolate, currentIndex));
		js_entry->Set(String::NewFromUtf8(isolate, "kind"), v8::Uint32::New(isolate, (unsigned)entry.entryKind));
		array->Set(currentIndex, js_entry);
		currentIndex++;
	  }
	  //my_log("\n");

	  my_log("*******************************************\n\n");

	  Local<Object> obj = Object::New(isolate);
	  obj->Set(String::NewFromUtf8(isolate, "completion_items"), array);
	  obj->Set(String::NewFromUtf8(isolate, "output"), String::NewFromUtf8(isolate, internal_parse_stdout.c_str()));
	  args.GetReturnValue().Set(obj);

  }

  void WatchFunction_ParseDocument(const FunctionCallbackInfo<Value> &args)
  {
    internal_parse_stdout = "";

    String::Utf8Value uriValue(args[0]);
    String::Utf8Value documentText(args[1]);
    std::string uri(*uriValue);
    std::string text(*documentText);

#ifdef _WIN32
      __try {
#endif
        ParseDocument(uri, text);

#ifdef _WIN32
      }
      __except (my_handler((struct Windows::_EXCEPTION_POINTERS*)Windows::_exception_info())) {}
#endif
 
  }

  void init(Local<Object> exports) {
    lexer = Lexer::CreateLanguageDfa();
    masterLibrary = CreateCoreLibrary();

#ifdef _WIN32
    SetupMinidump();
#endif

    NODE_SET_METHOD(exports, "AutoComplete", WatchFunction_AutoComplete);
    NODE_SET_METHOD(exports, "ParseDocument", WatchFunction_ParseDocument);
  }

  NODE_MODULE(addon, init)

}  // namespace demo
