#ifndef _NIHTTPD_SOCKET_H
#define _NIHTTPD_SOCKET_H 1
#include <string>
#include <vector>
#include <stdint.h>

namespace nihttpd {
	class connection {
		public:
			connection( int s );
			void disconnect( void );

			char recv_char( void );
			std::string recv_line( size_t max_length = 0x10000 );

			void send_char( char c );
			void send_line( const std::string &line );
			void send_str( const std::string &line );
			void send_data( const std::vector<char> &vec );

			std::string client_ip( void );

		private:
			int sock;
	};

	class listener {
		public:
			 listener( const std::string &host, const std::string &port );
			~listener( );
			connection accept_client( void );

		private:
			int sock;
	};
}

#endif
