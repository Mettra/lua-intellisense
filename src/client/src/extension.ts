/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';

let fs = require('fs');
import * as path from 'path';
import * as vscode from 'vscode';
import { workspace, Disposable, ExtensionContext,TextDocument, Uri } from 'vscode';
import { LanguageClient, LanguageClientOptions, SettingMonitor, ServerOptions, TransportKind, RequestType } from 'vscode-languageclient';

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


export function activate(context: ExtensionContext) {

	// The server is implemented in node
	let serverModule = context.asAbsolutePath(path.join('server', 'server.js'));
	// The debug options for the server
	let debugOptions = { execArgv: ["--nolazy", "--debug=6004"] };

	// If the extension is launched in debug mode then the debug server options are used
	// Otherwise the run options are used
	let serverOptions: ServerOptions = {
		run : { module: serverModule, transport: TransportKind.ipc },
		debug: { module: serverModule, transport: TransportKind.ipc, options: debugOptions }
	}
	
	// Options to control the language client
	let clientOptions: LanguageClientOptions = {
		// Register the server for lua documents
		documentSelector: ['lua'],
		synchronize: {
			// Synchronize the setting section 'languageServerExample' to the server
			configurationSection: 'languageServerExample',
			// Notify the server about file changes to '.clientrc files contain in the workspace
			fileEvents: workspace.createFileSystemWatcher('**/.clientrc')
		}
	}
	
	let client = new LanguageClient("ID", 'Language Server Example', serverOptions, clientOptions);
	let v : void;

	client.onReady().then(() =>
	{
		client.onRequest(PushDocumentsRequest.type, () => {
			workspace.findFiles("**/*.lua", "").then((value: Uri[]) => {
				console.log("It will have theese many" + value.length);
				client.sendRequest(ParseDocuments.type,value.length).then();

				value.forEach((uri : Uri) => {
					fs.readFile(uri.fsPath, (err : any, data : string) => {
						let d : RecievedDocument = {name : uri.toString(), text : data.toString()};
						client.sendRequest(ReceiveDocumentRequest.type, d).then();
					});
				});
			});

			
		});

		

		console.log("Registered!");
		client.sendRequest(InitializeRequest.type, v).then();
	});

	// Create the language client and start the client.
	let disposable = client.start();

	// Push the disposable to the context's subscriptions so that the 
	// client can be deactivated on extension deactivation
	context.subscriptions.push(disposable);
}

//Error handler
process.on('uncaughtException', function (exception) {
  // handle or ignore error
  console.log(exception);
});