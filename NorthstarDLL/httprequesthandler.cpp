#include "pch.h"
#include "httprequesthandler.h"
#include "version.h"

void HttpRequestHandler::StartHttpRequestHandler()
{
	if (IsRunning())
	{
		spdlog::warn("HttpRequestHandler::StartHttpRequestHandler was called while IsRunning() is true");
		return;
	}

	m_bIsHttpRequestHandlerRunning = true;
}

void HttpRequestHandler::StopHttpRequestHandler()
{
	if (!IsRunning())
	{
		spdlog::warn("HttpRequestHandler::StopHttpRequestHandler was called while IsRunning() is false");
		return;
	}


	m_bIsHttpRequestHandlerRunning = false;
}

size_t CurlWriteToStringBufferCallback(char* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

int HttpRequestHandler::MakeHttpRequest(const HttpRequest& requestParameters)
{
	if (!IsRunning())
	{
		spdlog::warn("HttpRequestHandler::MakeHttpRequest was called while IsRunning() is false");
		return -1;
	}

	// TODO: Check for localhost requests, block unless on server with -allowlocalhttp


	// This handle will be returned to Squirrel so it can wait for the response and assign a callback for it.
	int handle = ++m_iLastRequestHandle;

	std::thread requestThread(
		[this, handle, requestParameters]()
		{
			CURL* curl = curl_easy_init();
			if (!curl)
			{
				// TODO: Send http error message back to Squirrel.
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

			// Escape the query URL into a valid one.
			char* sanitizedUrl = curl_easy_escape(curl, queryUrl.c_str(), queryUrl.length());
			curl_easy_setopt(curl, CURLOPT_URL, sanitizedUrl);

			// If we're using a GET method, setup the write function so it can write into the body.
			std::string bodyBuffer;
			std::string headerBuffer;

			if (requestParameters.method == HttpRequestMethod::GET)
			{
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteToStringBufferCallback);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bodyBuffer);
			}

			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlWriteToStringBufferCallback);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerBuffer);

			// Add all the headers for the request.
			struct curl_slist* headers = nullptr;
			curl_slist_append(headers, fmt::format("Content-Type: {}", requestParameters.contentType).c_str());
			for (const auto& kv : requestParameters.headers)
			{
				curl_slist_append(headers, fmt::format("{}: {}", kv.first, kv.second).c_str());
			}

			// Enforce the Northstar user agent.
			curl_easy_setopt(curl, CURLOPT_USERAGENT, &NSUserAgent);

			CURLcode result = curl_easy_perform(curl);
			if (result == CURLE_OK)
			{
				// While the curl request is OK, it could return a non success code.
				// Squirrel side will handle firing the correct callback.
				// TODO: Send response over to Squirrel, yay.
			}
			else
			{
				// TODO: Send http request failed message.
				// Pass CURL result code & error.
				spdlog::error("curl_easy_perform() failed with code {}, error: ", result, curl_easy_strerror(result));
			}

			curl_free(sanitizedUrl);
			curl_easy_cleanup(curl);
		});


	return handle;
}
