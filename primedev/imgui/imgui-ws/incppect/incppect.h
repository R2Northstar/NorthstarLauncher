#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

class Incppect
{
	public:
		enum EventType
		{
			Connect,
			Disconnect,
			Custom,
		};

		using TUrl = std::string;
		using TResourceContent = std::string;
		using TPath = std::string;
		using TIdxs = std::vector<int>;
		using TGetter = std::function<std::string_view(const TIdxs & idxs)>;
		using THandler = std::function<void(int clientId, EventType etype, std::string_view)>;

		// service parameters
		struct Parameters
		{
			int32_t portListen = 3000;
			int32_t maxPayloadLength_bytes = 256*1024;
			int64_t tLastRequestTimeout_ms = 3000;
			int32_t tIdleTimeout_s = 120;

			std::string httpRoot = ".";
			std::vector<std::string> resources;

			std::string sslKey = "key.pem";
			std::string sslCert = "cert.pem";

			// todo:
			// max clients
			// max buffered amount
			// etc.
		};

		Incppect();
		~Incppect();

		// run the incppect service main loop in the current thread
		// blocking call
		void run(Parameters parameters);

		// terminate the server instance
		void stop();

		// set a resource. useful for serving html/js files from within the application
		void setResource(const TUrl & url, const TResourceContent & content);

		// number of connected clients
		int32_t nConnected() const;

		// run the incppect service main loop in dedicated thread
		// non-blocking call, returns the created std::thread
		std::thread runAsync(Parameters parameters);

		// define variable/memory to inspect
		//
		// examples:
		//
		//   var("path0", [](auto ) { ... });
		//   var("path1[%d]", [](auto idxs) { ... idxs[0] ... });
		//   var("path2[%d].foo[%d]", [](auto idxs) { ... idxs[0], idxs[1] ... });
		//
		bool var(const TPath & path, TGetter && getter);

		// handle input from the clients
		void handler(THandler && handler);

		// shorthand for string_view from var
		template <typename T>
			static std::string_view view(T & v) {
				if constexpr (std::is_same<T, std::string>::value) {
					return std::string_view { v.data(), v.size() };
				}
				return std::string_view { (char *)(&v), sizeof(v) };
			}

		template <typename T>
			static std::string_view view(T && v) {
				static T t;
				t = std::move(v);
				return std::string_view { (char *)(&t), sizeof(t) };
			}

		// get global instance
		static Incppect & getInstance() {
			static Incppect instance;
			return instance;
		}

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
};

