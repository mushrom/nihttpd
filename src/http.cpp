#include <nihttpd/http.h>
#include <iostream>
#include <string>
#include <sstream>
#include <ctype.h>

using namespace nihttpd;

static std::string strip_leading_spaces( std::string str ){
	auto it = str.begin();
	for ( ; it != str.end() && it[0] == ' '; it++ );

	return std::string( it, str.end() );
}

bool is_valid_prelude( std::string action, std::string location, std::string version ){
	// TODO: more extensive error checking
	return (action == "GET" || action == "POST" || action == "HEAD");
}

http_request::http_request( connection conn, size_t max_headers ){
	std::string temp = conn.recv_line();
	std::stringstream ss(temp);

	std::getline( ss, action,   ' ' );
	std::getline( ss, location, ' ' );
	std::getline( ss, version );

	std::cout << "[phead] > head: " << temp << std::endl;

	/*
	std::cout << "[parse] action:   \"" << action <<   "\"" << std::endl;
	std::cout << "[parse] location: \"" << location << "\"" << std::endl;
	std::cout << "[parse] version:  \"" << version <<  "\"" << std::endl;
	*/
	if ( !is_valid_prelude( action, location, version )){
		throw http_error( HTTP_400_BAD_REQUEST );
	}

	parse_headers( conn, max_headers );
	parse_body( conn );

	get_params = parse_fields( find_get_fields( location ));

	//get_params = parse_fields( find_get_fields( location ));
	location   = strip_get_fields( location );
	location   = url_decode( location );

	for ( const auto &x : get_params ){
		std::cout << "    GET > " << x.first << ": " << x.second << std::endl;
	}
}

void http_request::parse_headers( connection &conn, size_t max_headers ){
	std::string temp = conn.recv_line();

	for ( size_t n = 0; temp != "" && n <= max_headers; n++ ){
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

		if ( n == max_headers ){
			throw http_error( HTTP_413_ENTITY_TOO_LARGE );
		}

		headers[key] = value;
		temp = conn.recv_line();
		std::cout << "[phead] asdf: " << temp << std::endl;
	};
}

bool http_request::valid_post( void ){
	auto contains = [](auto m, auto key){ return m.find(key) != m.end(); };

	return action == "POST"
		&& contains(headers, "Content-Type")
		&& contains(headers, "Content-Length")
		&& headers["Content-Type"] == "application/x-www-form-urlencoded"
		;
}

void http_request::parse_body( connection &conn, size_t max_size ){
	std::cout << "[pbody] action: " << action << std::endl;
	std::cout << "[pbody] type: " << headers["Content-Type"] << std::endl;

	if ( action != "POST" ){
		return;
	}

	if ( !valid_post() ){
		throw http_error( HTTP_400_BAD_REQUEST );
	}

	size_t n = atoi( headers["Content-Length"].c_str());

	if ( n >= max_size ){
		throw http_error( HTTP_413_ENTITY_TOO_LARGE );
	}

	std::cout << "[pbody] asdf: " << std::to_string(n) << std::endl;
	std::string temp = conn.recieve(n);

	std::cout << "[pbody] body: " << temp << std::endl;

	body = url_decode( temp );
	post_params = parse_fields( body );
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
		case HTTP_200_OK:               return "200 OK";
		case HTTP_400_BAD_REQUEST:      return "400 Bad Request";
		case HTTP_403_FORBIDDEN:        return "403 Forbidden";
		case HTTP_404_NOT_FOUND:        return "404 Not Found";
		case HTTP_413_ENTITY_TOO_LARGE: return "413 Request Entity Too Large";
		default: return std::to_string( status ) + "TODO implement this";
	}
}

static inline char url_decode_char( char a, char b ){
	static std::string hextab = "0123456789abcdef";

	unsigned upper = hextab.find( tolower(a));
	unsigned lower = hextab.find( tolower(b));

	if ( upper == std::string::npos || lower == std::string::npos ){
		// TODO: handle this properly
		return '?';
	}

	return (upper << 4) | lower;
}

// copies a string, while decoding percent hex values
std::string nihttpd::url_decode( const std::string &str ){
	std::string ret = "";

	for ( auto c = str.begin(); c != str.end(); c++ ){
		if ( *c == '%' ){
			ret += url_decode_char( *(c + 1), *(c + 2));
			c += 2;

		} else {
			ret += *c;
		}
	}

	return ret;
}

static inline std::string hex_to_string( unsigned long n ){
	std::ostringstream ostr;
	ostr << std::hex << n;
	return ostr.str();
}

static inline char char_in( const char c, const std::string &xs ){
	for ( const auto k : xs ){
		if ( c == k ){
			return k;
		}
	}

	return false;
}

// URL encoding which doesn't encode the '/' character
std::string nihttpd::url_encode_path( const std::string &str ){
	std::string ret = "";

	for ( const auto c : str ){
		if ( isupper(c) || islower(c) || isdigit(c) || char_in(c, "-_.~/") ){
			ret += c;

		} else {
			ret += "%" + hex_to_string(c);
		}
	}

	return ret;
}

std::string nihttpd::sanitize( const std::string &str ){
	std::string ret = "";

	//for ( auto c = str.begin(); c != str.end(); c++ ){
	for ( const auto c : str ){
		switch (c) {
			case '<':  ret += "&lt;"; break;
			case '>':  ret += "&gt;"; break;
			case '&':  ret += "&amp;"; break;
			case '"':  ret += "&quot;"; break;
			case '\'': ret += "&apos;"; break;
			default:   ret += c; break;
		}
	}

	return ret;
}

bool nihttpd::path_below_root( const std::string &str ){
	int score = 0;

	size_t start = 0;
	size_t end = 0;
	size_t last_end = 0;

	while ( last_end != std::string::npos ){
		end = str.find("/", start);
		const std::string &token = str.substr(start, end - start);

		last_end = end;
		start    = end + 1;

		score += (token == "..")? -1 :
		         (token == ".")?   0 :
		         (token == "")?    0 :
		         /* otherwise */   1;

		if ( score < 0 ){
			return false;
		}
	}

	return true;
}

size_t nihttpd::get_fields_offset( const std::string &location ){
	return location.find("?");
}

std::string nihttpd::find_get_fields( const std::string &location ){
	size_t get_start = location.find("?");

	if ( get_start == std::string::npos ){
		return "";
	}

	return location.substr( get_start + 1, std::string::npos );
}

// NOTE: This must be called before the path is url decoded!
key_val_t nihttpd::parse_fields( const std::string &args ){
	// TODO: extract fields

	key_val_t ret = {};
	size_t field_start = 0;
	size_t field_end = 0;

	while ( field_end != std::string::npos ){
		field_end = args.find("&", field_start);

		std::string field = args.substr(field_start, field_end - field_start);
		std::stringstream ss(field);
		std::string key, value;

		std::getline( ss, key, '=' );
		std::getline( ss, value );

		ret[url_decode(key)] = url_decode(value);
		field_start = field_end + 1;
	}

	return ret;
}

std::string nihttpd::strip_get_fields( const std::string &location ){
	size_t get_start = location.find("?");

	if ( get_start == std::string::npos ){
		return location;
	}

	return location.substr(0, get_start);
}
