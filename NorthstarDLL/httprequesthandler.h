#pragma once

#include <string>
#include <unordered_map>

enum class ScriptContext;

// These definitions below should match on the Squirrel side so we can easily pass them along through a function.

/**
 * Allowed methods for an HttpRequest.
 */
namespace HttpRequestMethod
{
	enum Type
	{
		HRM_GET = 0,
		HRM_POST = 1,
		HRM_HEAD = 2,
		HRM_PUT = 3,
		HRM_DELETE = 4,
		HRM_PATCH = 5,
	};

	/** Returns the HTTP string representation of the given method. */
	inline std::string ToString(HttpRequestMethod::Type method)
	{
		switch (method)
		{
		case HttpRequestMethod::HRM_GET:
			return "GET";
		case HttpRequestMethod::HRM_POST:
			return "POST";
		case HttpRequestMethod::HRM_HEAD:
			return "HEAD";
		case HttpRequestMethod::HRM_PUT:
			return "PUT";
		case HttpRequestMethod::HRM_DELETE:
			return "DELETE";
		case HttpRequestMethod::HRM_PATCH:
			return "PATCH";
		default:
			return "INVALID";
		}
	}

	/** Whether or not the given method should be treated like a POST for curlopts. */
	bool UsesCurlPostOptions(HttpRequestMethod::Type method)
	{
		switch (method)
		{
		case HttpRequestMethod::HRM_POST:
		case HttpRequestMethod::HRM_PUT:
		case HttpRequestMethod::HRM_DELETE:
		case HttpRequestMethod::HRM_PATCH:
			return true;
		default:
			return false;
		}
	}
};

/** Contains data about an http request that has been queued. */
struct HttpRequest
{
	/** Method used for this http request. */
	HttpRequestMethod::Type method;

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

	/** The timeout for the http request, in seconds. Must be between 1 and 60. */
	int timeout;

	/** If set, the override to use for the User-Agent header. */
	std::string userAgent;
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

	template <ScriptContext context>
	int MakeHttpRequest(const HttpRequest& requestParameters);
	template <ScriptContext context> void RegisterSQFuncs();

  private:

	int m_iLastRequestHandle = 0;
	std::atomic_bool m_bIsHttpRequestHandlerRunning = false;
};

extern HttpRequestHandler* g_httpRequestHandler;
