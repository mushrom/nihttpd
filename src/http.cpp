#include <nihttpd/http.h>
#include <iostream>
#include <string>
#include <sstream>

using namespace nihttpd;

static std::string strip_leading_spaces( std::string str ){
	auto it = str.begin();
	for ( ; it != str.end() && it[0] == ' '; it++ );

	return std::string( it, str.end() );
}

bool is_valid_prelude( std::string action, std::string location, std::string version ){
	// TODO: more extensive error checking
	return (action == "GET" || action == "POST");
}

http_request::http_request( connection conn ){
	std::string temp = conn.recv_line();
	std::stringstream ss(temp);

	std::getline( ss, action,   ' ' );
	std::getline( ss, location, ' ' );
	std::getline( ss, version );

	/*
	std::cout << "[parse] action:   \"" << action <<   "\"" << std::endl;
	std::cout << "[parse] location: \"" << location << "\"" << std::endl;
	std::cout << "[parse] version:  \"" << version <<  "\"" << std::endl;
	*/
	if ( !is_valid_prelude( action, location, version )){
		throw http_error( HTTP_400_BAD_REQUEST );
	}

	temp = conn.recv_line();

	while ( temp != "" ) {
		std::stringstream line(temp);
		std::string key, value;

		std::getline( line, key, ':' );
		std::getline( line, value );

		value = strip_leading_spaces( value );

		if ( key.empty() || value.empty() ){
			std::cout << "[phead] invalid header recieved, TODO: error" << std::endl;
			std::cout << "[phead] key: " << key << ", value: " << value << std::endl;
			throw http_error( HTTP_400_BAD_REQUEST );
		}

		headers[key] = value;
		temp = conn.recv_line();
	};
}

void http_response::send_to( connection conn ){
	conn.send_line( "HTTP/1.1 " + status_string( status ));

	for ( const auto &x : headers ){
		std::string temp = x.first + ": " + x.second;
		conn.send_line( temp );
	}

	conn.send_line( "" );
	conn.send_data( content );
}

void http_response::send_headers( connection conn ){
	conn.send_line( "HTTP/1.1 " + status_string( status ));

	for ( const auto &x : headers ){
		std::string temp = x.first + ": " + x.second;
		conn.send_line( temp );
	}

	conn.send_line( "" );
}

void http_response::send_content( connection conn, const std::vector<char> &vec ){
	conn.send_data( vec );
}

void http_response::set_content( std::string &str ){
	content.assign( str.begin(), str.end() );
}

std::string nihttpd::status_string( unsigned status ){
	std::string str;

	// TODO: find a better way to do this
	switch ( status ){
		case HTTP_200_OK:          return "200 OK";
		case HTTP_400_BAD_REQUEST: return "400 Bad Request";
		case HTTP_404_NOT_FOUND:   return "404 Not Found";
		default: return std::to_string( status ) + "TODO implement this";
	}
}
