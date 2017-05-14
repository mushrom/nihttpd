#ifndef _NIHTTPD_H
#define _NIHTTPD_H 1
#include <nihttpd/socket.h>
#include <nihttpd/http.h>
#include <string>
#include <thread>
#include <list>
#include <memory>

namespace nihttpd {
	enum class urgency {
		debug,
		low,
		normal,
		info,
		high,
		critical,
	};

	class router;

	class server {
		public:
			server( );
			void start( const std::string &host, const std::string &port );
			void wait( void );
			void log( urgency level, const std::string &msg );
			void add_route( std::unique_ptr<router> rut );

		private:
			void run( void );
			void worker( connection conn );
			router *find_route( std::string location );

			bool running;
			listener *sock;
			std::list<std::unique_ptr<router>> routes;
			std::thread self;
	};

	class router {
		public:
			typedef void (*handler)( http_request req, connection conn );
			router( std::string pref, std::string dest );
			virtual void dispatch( http_request req, connection conn );

			std::string prefix;
			std::string destination;
	};
}

#endif
