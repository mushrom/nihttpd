#include <nihttpd/file_route.h>
#include <nihttpd/nihttpd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <map>

using namespace nihttpd;

static std::map<std::string, std::string> content_types = {
	{ ".html", "text/html" },
	{ ".css",  "text/css"  },
	{ ".txt",  "text/plain" },
	{ ".jpg",  "image/jpeg" },
	{ ".png",  "image/png" },
	{ ".mp3",  "audio/mpeg" },
	{ ".ogg",  "audio/ogg" },
	{ ".mkv",  "video/x-matroska" },
};

static unsigned get_file_size( std::ifstream &stream ){
	stream.seekg( 0, stream.end );
	unsigned ret = stream.tellg();
	stream.seekg( 0, stream.beg );

	return ret;
}

static std::string get_content_type( std::string filename ){
	std::string ret = "text/plain";
	std::size_t location = filename.rfind( '.' );

	if ( location != std::string::npos ){
		std::string suffix = filename.substr( location, filename.length() );
		auto x = content_types.find( suffix );

		if ( x != content_types.end() ){
			ret = (*x).second;
		}
	}

	return ret;
}

void file_router::dispatch( http_request req, connection conn ){
	http_response response;

	std::string translated = translate_filename( req.location );
	std::ifstream document( translated, std::ifstream::in );

	if ( !document.good() ){
		throw http_error( HTTP_404_NOT_FOUND );
	}

	unsigned length = get_file_size( document );

	response.headers["Connection"]     = "close";
	response.headers["Content-Type"]   = get_content_type( translated );
	response.headers["Content-Length"] = std::to_string( length );
	response.headers["Server"]         = "nihttpd/0.0.1";
	response.send_headers( conn );

	std::vector<char> foo(0x1000);

	while ( document.good() ){
		document.read( foo.data(), 0x1000 );
		response.send_content( conn, foo );
	}
}

std::string file_router::translate_filename( std::string filename ){
	std::string temp   = destination + filename.substr( prefix.length() );
	std::string rm_str = "..";

	std::size_t loc = temp.find( rm_str );

	while ( loc != std::string::npos ){
		auto pos = temp.begin();
		temp.erase( pos + loc, pos + loc + rm_str.length() );
		loc = temp.find( rm_str );
	}

	if ( *temp.rbegin() == '/' ){
		temp += "index.html";
	}

	return temp;
}
