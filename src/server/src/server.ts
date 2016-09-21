/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';

let fs = require('fs');
const lua_parser = require("lua_parser/lua_parser.node")
import {
    IPCMessageReader, IPCMessageWriter,
	createConnection, IConnection,
	TextDocuments, TextDocument, Diagnostic, DiagnosticSeverity, 
	InitializeParams, InitializeResult,
	TextDocumentPositionParams, CompletionItem, CompletionItemKind, CompletionOptions, RequestType
} from 'vscode-languageserver';

namespace InitializeRequest {
	export const type: RequestType<void, void, any> = { get method() { return 'lua_intellisense/initializeRequest'; } };
}

namespace PushDocumentsRequest {
	export const type: RequestType<void, void, any> = { get method() { return 'lua_intellisense/pushDocumentRequest'; } };
}

interface RecievedDocument
{
    text : string;
    name : string;
}

namespace ReceiveDocumentRequest {
	export const type: RequestType<RecievedDocument, void, any> = { get method() { return 'lua_intellisense/receiveDocumentRequest'; } };
}

namespace ParseDocuments {
	export const type: RequestType<number, void, any> = { get method() { return 'lua_intellisense/parseDocuments'; } };
}

let newDocs : RecievedDocument[] = []
let allFileNum : number = -1;
let parsedAllFiles = false;

let print_con: IConnection = createConnection(new IPCMessageReader(process), new IPCMessageWriter(process));

// Create a connection for the server. The connection uses Node's IPC as a transport
let connection: IConnection = createConnection(new IPCMessageReader(process), new IPCMessageWriter(process));

// Create a simple text document manager. The text document manager
// supports full document sync only
let documents: TextDocuments = new TextDocuments();
// Make the text document manager listen on the connection
// for open, change and close text document events
documents.listen(connection);

// After the server has started the client sends an initialize request. The server receives
// in the passed params the rootPath of the workspace plus the client capabilities. 
let workspaceRoot: string;
connection.onInitialize((params): InitializeResult => {
	workspaceRoot = params.rootPath;

    print_con.console.log("Server initialized!");
    
	return {
		capabilities: {
				// Tell the client that the server works in FULL text document sync mode
				textDocumentSync: documents.syncKind,

				// Tell the client that the server support code complete
				completionProvider: {
					resolveProvider: true,
                    triggerCharacters: ['.',':']
				}
			}
		}
});

// This handler provides the initial list of the completion items.
connection.onCompletion((textDocumentPosition: TextDocumentPositionParams): CompletionItem[] => 
{
    if(!parsedAllFiles)
        return [];
    
    // The pass parameter contains the position of the text document in 
    // which code complete got requested. For the example we ignore this
    // info and always provide the same completion items.
	var currentDoc = documents.get(textDocumentPosition.textDocument.uri);
    lua_parser.ParseDocument(currentDoc.uri, currentDoc.getText());

	var ret = lua_parser.AutoComplete(currentDoc.uri, textDocumentPosition.position.line, textDocumentPosition.position.character);
    
    return ret.completion_items;
});

// This handler resolve additional information for the item selected in
// the completion list.
connection.onCompletionResolve((item: CompletionItem): CompletionItem => {
    return item;
});

connection.onDidChangeConfiguration(() =>
{
});

connection.onRequest(InitializeRequest.type, () =>
{
    connection.sendRequest(PushDocumentsRequest.type).then();
})

connection.onRequest(ReceiveDocumentRequest.type, (doc : RecievedDocument) =>
{
    newDocs.push(doc);

    if(newDocs.length == allFileNum)
    {
        newDocs.forEach((v : RecievedDocument) => {
            lua_parser.ParseDocument(v.name, v.text);
        })
        print_con.console.log("Done parsing all files in the workspace.");
        parsedAllFiles = true
    }
})

connection.onRequest(ParseDocuments.type, (v : number) =>
{
    allFileNum = v;
})

// Listen on the connection
connection.listen();