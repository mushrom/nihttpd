#ifndef _NIHTTPD_H
#define _NIHTTPD_H 1
#include <string>
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
}

#endif
