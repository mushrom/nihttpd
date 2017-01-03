#include <nihttpd/socket.h>
#include <chrono>
#include <thread>

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
