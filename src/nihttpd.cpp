#include <nihttpd/nihttpd.h>
#include <iostream>

using namespace nihttpd;

server::server( ){
	log( urgency::normal, "starting nihttpd" );
}

void server::log( urgency level, const std::string &msg ){
	std::cout << "> " << msg << std::endl;
}
