#include <nihttpd/nihttpd.h>
#include <nihttpd/socket.h>
#include <iostream>

using namespace nihttpd;

server::server( ){
	log( urgency::normal, "starting nihttpd" );

	try {
		listener foo( "127.0.0.1", "8081" );
	} catch ( const std::string &msg ){
		log( urgency::high, "exception in listener: " + msg );
		throw;
	}
}

void server::log( urgency level, const std::string &msg ){
	std::cout << "> " << msg << std::endl;
}
