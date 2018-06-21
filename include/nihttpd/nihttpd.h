#ifndef _NIHTTPD_H
#define _NIHTTPD_H 1
#include <nihttpd/socket.h>
#include <nihttpd/http.h>
#include <string>
#include <thread>
#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>

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
			void run(void);
			void worker(void);
			connection wait_for_connection(void);
			router *find_route( std::string location );

			bool running;
			listener *sock;
			std::list<std::unique_ptr<router>> routes;
			std::list<std::thread> workers;
			std::thread self;

			std::mutex listen_lock;
			std::condition_variable waiter;
			std::condition_variable master;
			std::list<connection> pending_connects;
			unsigned available_threads = 0;
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
