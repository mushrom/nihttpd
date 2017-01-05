#ifndef _NIHTTPD_H
#define _NIHTTPD_H 1
#include <nihttpd/socket.h>
#include <nihttpd/http.h>
#include <string>
#include <thread>
#include <stdint.h>

namespace nihttpd {
	enum class urgency {
		debug,
		low,
		normal,
		info,
		high,
		critical,
	};

	class server {
		public:
			server( );
			void log( urgency level, const std::string &msg );

		private:
			void listen( const std::string &host, uint16_t port );
	};

	class router {
		public:
			typedef void (*handler)( http_request req, connection conn );
			router( std::string pref, std::string dest, handler hand );
			void dispatch( http_request req, connection conn );

		private:
			std::string prefix;
			std::string destination;
			handler handle;
	};
}

#endif
