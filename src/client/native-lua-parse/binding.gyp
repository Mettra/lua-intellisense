{
  "targets": [
    {
      "target_name": "lua_parser",
      "sources": [ 
        "src/Main_Node.cpp",
        "src/AST_Nodes.cpp",
        "src/AutoComplete.cpp",
        "src/Descent_Parser.cpp",
        "src/Lexer_DFA.cpp",
        "src/Token.cpp",
        "src/TypeSystem.cpp",
        "src/Minidump.cpp"
      ],
      "defines": [
        "NODE_PRINT"
      ],
      'conditions': [
        ['OS=="win"', {
          'configurations': {
            'Debug': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeTypeInfo': 'true',
                },
              }
            }, # Debug
            'Release': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeTypeInfo': 'true',
                },
              },
            }, # Release
          }, # configurations

          'libraries':[
          '-ldbghelp.lib'
        ],
        }], # OS=="win"
 	     ['OS!="win"', {
		      'cflags_cc!': [ '-fno-rtti' ],
		      'cflags_cc+': [ '-std=c++14' ],
        }],
		    ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_RTTI': 'YES'
		      }
        }],
      ], # conditions
      'include_dirs': [
        '../LuaParser/LuaParser/'
      ],
      
    },
  ]
}
#node-gyp configure rebuild --nodedir="c:\Sandbox\node_source" --debug --arch=ia32
#node-gyp build --nodedir="c:\Sandbox\node_source" --debug --arch=ia32
# C:\Sandbox\node_source\Debug\node.exe
# C:\Sandbox\lua-parser-node\
#
#
#node-gyp rebuild --verbose --arch=ia32 --target=0.37.6 --dist-url=https://atom.io/download/atom-shell --debug
#node-gyp build --verbose --arch=ia32 --target=0.37.6 --dist-url=https://atom.io/download/atom-shell --debug
