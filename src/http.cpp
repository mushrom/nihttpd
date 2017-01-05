#include <nihttpd/http.h>
#include <iostream>
#include <string>
#include <sstream>

using namespace nihttpd;

http_request::http_request( connection conn ){
	std::string temp = conn.recv_line();
	std::stringstream ss(temp);

	std::getline( ss, action,   ' ' );
	std::getline( ss, location, ' ' );
	std::getline( ss, version );

	std::cout << "[parse] action:   \"" << action <<   "\"" << std::endl;
	std::cout << "[parse] location: \"" << location << "\"" << std::endl;
	std::cout << "[parse] version:  \"" << version <<  "\"" << std::endl;

	temp = conn.recv_line();

	while ( temp != "" ) {
		std::stringstream line(temp);
		std::string key, value;

		std::getline( line, key, ':' );
		std::getline( line, value );

		if ( key.empty() || value.empty() ){
			std::cout << "[phead] invalid header recieved, TODO: error" << std::endl;
			std::cout << "[phead] key: " << key << ", value: " << value << std::endl;
		}

		headers[key] = value;
		temp = conn.recv_line();
	};
}

void http_response::send_to( connection conn ){
	conn.send_line( "HTTP/1.1 200 OK" );

	for ( const auto &x : headers ){
		std::string temp = x.first + ": " + x.second;
		conn.send_line( temp );
	}

	conn.send_line( "" );
	conn.send_str( content );
}
