#include <nihttpd/socket.h>
#include <chrono>
#include <thread>
#include <vector>

// network stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

using namespace nihttpd;

static bool die( const std::string &msg ){
	throw( msg + ": " + strerror(errno) );
	return false;
}

listener::listener( const std::string &host, const std::string &port ){
	struct addrinfo hints, *re;
	int temp = 0;

	memset( &hints, 0, sizeof( hints ));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;

	getaddrinfo( host.c_str(), port.c_str(), &hints, &re );

	(sock = socket(re->ai_family, re->ai_socktype, re->ai_protocol)) >= 0
	  || die( "socket()" );

	setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) >= 0
	  || die( "setsockopt()" );

	bind( sock, re->ai_addr, re->ai_addrlen ) >= 0
	  || die( "bind()" );

	listen( sock, 10 ) >= 0
	  || die( "listen()" );
}

listener::~listener( ){
	close( sock );
}

connection listener::accept_client( void ){
	struct sockaddr_in client;
	socklen_t sin_size  = sizeof(struct sockaddr_in);
	int client_sock = accept( sock, (struct sockaddr *)&client, &sin_size );

	return connection( client_sock );
}

connection::connection( int s ){
	sock = s;
}

void connection::disconnect( void ){
	close( sock );
}

// TODO: more efficient implementation of the following functions,
//       will be a performance bottleneck
char connection::recv_char( void ){
	char ret = 0;
	recv( sock, &ret, 1, 0 );

	return ret;
}

void connection::send_char( char c ){
	while ( send( sock, &c, 1, MSG_NOSIGNAL ) == 0 );
}

void connection::send_line( const std::string &line ){
	send_str( line );
	send_str( "\r\n" );
}

void connection::send_str( const std::string &str ){
	unsigned total_sent = 0;

	while ( total_sent < str.length() ){
		unsigned to_send = str.length() - total_sent;
		int ret = send( sock, str.c_str() + total_sent, to_send, MSG_NOSIGNAL );

		if ( ret >= 0 ){
			total_sent += ret;

		} else {
			break;
			// TODO; throw exception
			;
		}
	}
}

void connection::send_data( const std::vector<char> &vec ){
	unsigned total_sent = 0;

	while ( total_sent < vec.size() ){
		unsigned to_send = vec.size() - total_sent;
		int ret = send( sock, vec.data() + total_sent, to_send, MSG_NOSIGNAL );

		if ( ret >= 0 ){
			total_sent += ret;

		} else {
			break;
			// TODO; throw exception
			;
		}
	}
}

std::string connection::recv_line( size_t max_length ){
	std::string ret = "";
	char c = recv_char();

	for ( size_t n = 0; n < max_length && c != '\r' && c != '\n'; n++ ){
		ret += c;
		c = recv_char();
	}

	if ( c == '\r' ){
		// TODO: check that the next character is a newline maybe,
		//       or possibly have a 'strict' config setting that toggles
		//       loose parsing behavior
		recv_char();
	}

	return ret;
}

std::string connection::recieve( size_t max_length ){
	std::string ret = "";
	char *buf = new char[max_length + 1];

	int n = recv( sock, buf, max_length, 0 );

	if ( n > 0 ){
		buf[n] = 0;
		std::string ret(buf);
		delete buf;
		return ret;
	}

	delete buf;
	return "";
}


std::string connection::client_ip( void ){
	char buf[INET6_ADDRSTRLEN];
	struct sockaddr_in info;
	socklen_t len = sizeof( info );

	getpeername( sock, (struct sockaddr *)&info, &len );
	inet_ntop( info.sin_family, &info.sin_addr, buf, sizeof buf );

	return buf;
}
