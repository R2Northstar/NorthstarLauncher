#include "pch.h"
#include "httprequesthandler.h"
#include "version.h"
#include "squirrel.h"
#include "tier0.h"


HttpRequestHandler* g_httpRequestHandler;

void HttpRequestHandler::StartHttpRequestHandler()
{
	if (IsRunning())
	{
		spdlog::warn("HttpRequestHandler::StartHttpRequestHandler was called while IsRunning() is true");
		return;
	}

	m_bIsHttpRequestHandlerRunning = true;
	spdlog::info("HttpRequestHandler started.");
}

void HttpRequestHandler::StopHttpRequestHandler()
{
	if (!IsRunning())
	{
		spdlog::warn("HttpRequestHandler::StopHttpRequestHandler was called while IsRunning() is false");
		return;
	}


	m_bIsHttpRequestHandlerRunning = false;
	spdlog::info("HttpRequestHandler stopped.");
}

size_t HttpCurlWriteToStringBufferCallback(char* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

template <ScriptContext context>
int HttpRequestHandler::MakeHttpRequest(const HttpRequest& requestParameters)
{
	if (!IsRunning())
	{
		spdlog::warn("HttpRequestHandler::MakeHttpRequest was called while IsRunning() is false");
		return -1;
	}

	// TODO: Check for localhost requests, block unless on server with -allowlocalhttp
	if (context != ScriptContext::SERVER || !Tier0::CommandLine()->FindParm("-allowlocalhttp"))
	{
		if (false)
		{
			spdlog::warn("HttpRequestHandler::MakeHttpRequest attempted to make a request to localhost. This is only allowed on servers "
						 "running with -allowlocalhttp.");
			return -1;
		}
	}

	// This handle will be returned to Squirrel so it can wait for the response and assign a callback for it.
	int handle = ++m_iLastRequestHandle;

	std::thread requestThread(
		[this, handle, requestParameters]()
		{
			CURL* curl = curl_easy_init();
			if (!curl)
			{
				// TODO: Send http error message back to Squirrel.
				spdlog::error("HttpRequestHandler::MakeHttpRequest failed to init libcurl for request.");
				g_pSquirrel<context>->createMessage(
					"NSHandleFailedHttpRequest", handle, static_cast<int>(CURLE_FAILED_INIT), curl_easy_strerror(CURLE_FAILED_INIT));				
				return;
			}

			CURLoption curlMethod = CURLOPT_HTTPGET;
			switch (requestParameters.method)
			{
			case HttpRequestMethod::GET:
				curlMethod = CURLOPT_HTTPGET;
				break;
			case HttpRequestMethod::POST:
				curlMethod = CURLOPT_HTTPPOST;
				break;
			}

			// Ensure we only allow HTTP or HTTPS.
			curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
			curl_easy_setopt(curl, curlMethod, 1L);

			// Allow redirects
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			// TODO: Maybe reduce this a little, that seems a bit too much.
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);

			std::string queryUrl = requestParameters.baseUrl;

			// GET requests, or POST requests with an empty body, can have query parameters.
			// Append them to the base url.
			if (requestParameters.method == HttpRequestMethod::GET || requestParameters.body.empty())
			{
				int idx = 0;
				for (const auto& kv : requestParameters.queryParameters)
				{
					if (idx == 0)
					{
						queryUrl.append(fmt::format("?{}={}", kv.first, kv.second));
					}
					else
					{
						queryUrl.append(fmt::format("&{}={}", kv.first, kv.second));
					}
				}
			}
			else
			{
				// Grab the body and set it as a POST field
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestParameters.body);
			}

			curl_easy_setopt(curl, CURLOPT_URL, queryUrl.c_str());

			// If we're using a GET method, setup the write function so it can write into the body.
			std::string bodyBuffer;
			std::string headerBuffer = "[header data not in yet]";

			if (requestParameters.method == HttpRequestMethod::GET)
			{
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpCurlWriteToStringBufferCallback);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bodyBuffer);
			}

			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HttpCurlWriteToStringBufferCallback);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerBuffer);

			// Add all the headers for the request.
			struct curl_slist* headers = nullptr;
			curl_slist_append(headers, fmt::format("Content-Type: {}", requestParameters.contentType).c_str());
			for (const auto& kv : requestParameters.headers)
			{
				curl_slist_append(headers, fmt::format("{}: {}", kv.first, kv.second).c_str());
			}

			curl_easy_setopt(curl, CURLOPT_HEADER, headers);
			// Enforce the Northstar user agent.
			curl_easy_setopt(curl, CURLOPT_USERAGENT, &NSUserAgent);

			CURLcode result = curl_easy_perform(curl);
			if (IsRunning())
			{
				if (result == CURLE_OK)
				{
					// While the curl request is OK, it could return a non success code.
					// Squirrel side will handle firing the correct callback.
					long httpCode = 0;
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
					// TODO: Send response over to Squirrel, yay.
					g_pSquirrel<context>->createMessage("NSHandleSuccessfulHttpRequest", handle, static_cast<int>(httpCode), bodyBuffer, headerBuffer);				
				}
				else
				{
					// Pass CURL result code & error.
					spdlog::error("curl_easy_perform() failed with code {}, error: {}", static_cast<int>(result), curl_easy_strerror(result));
					g_pSquirrel<context>->createMessage("NSHandleFailedHttpRequest", handle, static_cast<int>(result), curl_easy_strerror(result));			
				}
			}

			curl_easy_cleanup(curl);
		});

	requestThread.detach();
	return handle;
}

template <ScriptContext context>
void HttpRequestHandler::RegisterSQFuncs()
{
	g_pSquirrel<context>->AddFuncRegistration
	(
		"int",
		"NS_InternalMakeHttpRequest",
		"int method, string baseUrl, table<string, string> headers, table<string, string> queryParams, string contentType, string body",
		"[Internal use only] Passes the HttpRequest struct fields to be reconstructed in native and used for an http request",
		SQ_InternalMakeHttpRequest<context>
	);
}

// int NS_InternalMakeHttpRequest(int method, string baseUrl, table<string, string> headers, table<string, string> queryParams, string contentType, string body)
template<ScriptContext context>
SQRESULT SQ_InternalMakeHttpRequest(HSquirrelVM* sqvm)
{
	if (!g_httpRequestHandler || !g_httpRequestHandler->IsRunning())
	{
		spdlog::warn("NS_InternalMakeHttpRequest called while the http request handler isn't running.");
		g_pSquirrel<context>->pushinteger(sqvm, -1);
		return SQRESULT_NOTNULL;
	}

	HttpRequest request;
	request.method = static_cast<HttpRequestMethod>(g_pSquirrel<context>->getinteger(sqvm, 1));
	request.baseUrl = g_pSquirrel<context>->getstring(sqvm, 2);

	SQTable* headerTable = sqvm->_stackOfCurrentFunction[3]._VAL.asTable;
	for (int idx = 0; idx < headerTable->_numOfNodes; ++idx)
	{
		tableNode* node = &headerTable->_nodes[idx];

		// TODO: Figure out why the first three nodes of this table are OT_NULL with a single value.
		// Then re-enable this if possible.
		//if (node->key._Type != OT_STRING || node->val._Type != OT_STRING)
		//{
		//	g_pSquirrel<context>->raiseerror(
		//		sqvm, fmt::format("Invalid header type or value (expected string, got {} = {})", SQTypeNameFromID(node->key._Type), SQTypeNameFromID(node->val._Type)).c_str());
		//	return SQRESULT_ERROR;
		//}

		if (node->key._Type == OT_STRING && node->val._Type == OT_STRING)
		{
			request.headers[node->key._VAL.asString->_val] = node->val._VAL.asString->_val;
		}
	}

	SQTable* queryTable = sqvm->_stackOfCurrentFunction[4]._VAL.asTable;
	for (int idx = 0; idx < queryTable->_numOfNodes; ++idx)
	{
		tableNode* node = &queryTable->_nodes[idx];
		// Same as above.
		//if (node->key._Type != OT_STRING || node->val._Type != OT_STRING)
		//{
		//	g_pSquirrel<context>->raiseerror(
		//		sqvm, fmt::format("Invalid query parameter type or value (expected string, got {} = {})", SQTypeNameFromID(node->key._Type), SQTypeNameFromID(node->val._Type)).c_str());
		//	return SQRESULT_ERROR;
		//}

		if (node->key._Type == OT_STRING && node->val._Type == OT_STRING)
		{
			request.queryParameters[node->key._VAL.asString->_val] = node->val._VAL.asString->_val;

		}
	}

	request.contentType = g_pSquirrel<context>->getstring(sqvm, 5);
	request.body = g_pSquirrel<context>->getstring(sqvm, 6);

	int handle = g_httpRequestHandler->MakeHttpRequest<context>(request);
	g_pSquirrel<context>->pushinteger(sqvm, handle);
	return SQRESULT_NOTNULL;
}

ON_DLL_LOAD_RELIESON("client.dll", HttpRequestHandler_ClientInit, ClientSquirrel, (CModule module))
{
	g_httpRequestHandler->RegisterSQFuncs<ScriptContext::CLIENT>();
}

ON_DLL_LOAD_RELIESON("server.dll", HttpRequestHandler_ServerInit, ServerSquirrel, (CModule module))
{
	g_httpRequestHandler->RegisterSQFuncs<ScriptContext::SERVER>();
}

ON_DLL_LOAD("engine.dll", HttpRequestHandler_Init, (CModule module))
{
	g_httpRequestHandler = new HttpRequestHandler;
	g_httpRequestHandler->StartHttpRequestHandler();
}
