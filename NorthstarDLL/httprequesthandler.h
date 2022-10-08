#pragma once

#include <string>
#include <unordered_map>

enum class ScriptContext;

// These definitions below should match on the Squirrel side so we can easily pass them along through a function.

/**
 * Method for an HttpRequest. For now, we'll only support GET & POST.
 * If the need arises, we can add others.
 */
enum class HttpRequestMethod : int
{
	GET = 0,
	POST = 1
};

/** Contains data about an http request that has been queued. */
struct HttpRequest
{
	/** Method used for this http request. */
	HttpRequestMethod method;

	/** Base URL of this http request. */
	std::string baseUrl;

	/** Headers used for this http request. Some may get overridden or ignored. */
	std::unordered_map<std::string, std::string> headers;

	/** Query parameters for this http request. */
	// TODO: Check if we should keep the order on those. *Most* APIs shouldn't really care but you never know.
	std::unordered_map<std::string, std::string> queryParameters;

	/** The content type of this http request. Defaults to text/plain & UTF-8 charset. */
	std::string contentType = "text/plain; charset=utf-8";

	/** The body of this http request. If set, will override queryParameters.*/
	std::string body;
};

/**
 * Handles making HTTP requests and sending the responses back to Squirrel.
 */
class HttpRequestHandler
{
  public:

	void StartHttpRequestHandler();
	void StopHttpRequestHandler();
	bool IsRunning() const { return m_bIsHttpRequestHandlerRunning; }
	bool IsDestinationHostAllowed(const std::string& host, std::string& outResolvedHost, std::string& outHostHeader);

	template <ScriptContext context>
	int MakeHttpRequest(const HttpRequest& requestParameters);
	template <ScriptContext context> void RegisterSQFuncs();

  private:

	int m_iLastRequestHandle = 0;
	std::atomic_bool m_bIsHttpRequestHandlerRunning = false;
};

extern HttpRequestHandler* g_httpRequestHandler;
