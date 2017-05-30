#include <nihttpd/file_route.h>
#include <nihttpd/nihttpd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <cstdio>

#include <sys/stat.h>
#include <dirent.h>

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

static size_t get_file_stat_size( std::string path ){
	struct stat s;

	if ( stat(path.c_str(), &s) != 0 ){
		return 0;
	}

	return s.st_size;
}

static bool path_exists( std::string path ){
	struct stat s;

	return stat(path.c_str(), &s) == 0;
}

static bool path_is_directory( std::string path ){
	// TODO: add windows support code, or wait for c++17 to become ubiquitous
	struct stat s;

	if ( stat(path.c_str(), &s) == 0 ){
		return !!(s.st_mode & S_IFDIR);
	}

	return false;
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

static void send_file( http_request req, connection conn, std::string path ){
	http_response response;
	std::ifstream document( path, std::ifstream::in );

	if ( !document.good() ){
		throw http_error( HTTP_404_NOT_FOUND );
	}

	unsigned length = get_file_size( document );
	response.headers["Connection"]     = "close";
	response.headers["Content-Type"]   = get_content_type( path );
	response.headers["Content-Length"] = std::to_string( length );
	response.headers["Server"]         = "nihttpd/0.0.1";
	response.send_headers( conn );

	if ( req.action == "HEAD" ){
		return;
	}

	std::vector<char> foo(0x1000);

	while ( document.good() ){
		document.read( foo.data(), 0x1000 );
		response.send_content( conn, foo );
	}
}

static std::string get_file_icon( std::string path ){
	std::string type = get_content_type( path );

	if ( path_is_directory( path )){
		return "\xf0\x9f\x93\x81";

	} else {
		// default symbol, a 'face-up page' glyph

		if ( type == "image/jpeg" || type == "image/png" ){
			// framed picture glyph
			return "\xf0\x9f\x96\xbc";
		}

		if ( type == "audio/mpeg" || type == "audio/ogg" ){
			// single musical note glyph
			return "\xf0\x9f\x8e\xb5";
		}
	}

	return "\xf0\x9f\x93\x84";
}

static std::string gen_human_size( size_t bytes ){
	if ( bytes > 1024 * 1024 ){
		return std::to_string(bytes / 1024 / 1024) + "MiB";
	}

	if ( bytes > 1024 ){
		return std::to_string(bytes / 1024) + "KiB";
	}

	return std::to_string(bytes) + "B";
}

static std::string gen_dir_listing( http_request req, std::string path ){
	if ( !path_is_directory( path )){
		return "<html><body><h1>wait what</h1></body></html>";
	}

	std::string ret =
		"<!doctype html><head>"
			"<title>Index of " + req.location + "</title>"
			"<meta name=\"viewport\" content=\"width=device-width, intial-scale=1.0\">"
			"<meta charset=\"UTF-8\">"
			"<style>"
				"body {"
					"margin-left: auto;"
					"margin-right: auto;"
				"}"
				"table { width: 100%; margin: 0; padding: 0; }"
				"tr, td { border-top: 1px dotted lightgrey; }"
				".table_head { background-color: #fee; }"
			"</style>"
		"</head>"
		"<body>"
			"<div class=\"list_head\"><h3>Index of " + req.location + "</h3></div>"
			//"<hr />"
			"<table>"
				"<tr class=\"table_head\">"
					"<td></td>"
					"<td><strong>name</strong></td>"
					"<td><strong>type</strong></td>"
					"<td><strong>size</strong></td>"
				"</tr>"
		;


	DIR *dir;
	struct dirent *ent;

	if (( dir = opendir( path.c_str( ))) == NULL ){
		throw http_error( HTTP_404_NOT_FOUND );
	}

	while (( ent = readdir( dir ))){
		std::string name( ent->d_name );
		std::string temp = (req.location == "/")? req.location : req.location + "/";
		std::string local_path = path + "/" + name;
		std::string sizestr = gen_human_size( get_file_stat_size( local_path ));
		std::string type = get_content_type( local_path );
		std::string symbol = get_file_icon( local_path );

		ret +=
			"<tr>"
				"<td>" + symbol + "</td>"
				"<td><a href=\"" + temp + name + "\">" + name + "</a></td>"
				"<td><code>" + type + "</code></td>"
				"<td><code>" + sizestr + "</code></td>"
			"</tr>"
		;
	}

	ret += "</table>";
	ret += "<hr /><small>nihttpd 0.0.1</small>";
	ret += "</body></html>";

	return ret;
}

static void send_dir_listing( http_request req, connection conn, std::string path ){
	http_response response;

	std::string page = gen_dir_listing( req, path );
	response.set_content( page );

	response.headers["Connection"]     = "close";
	response.headers["Content-Type"]   = "text/html";
	response.headers["Content-Length"] = std::to_string( response.content.size() );
	response.headers["Server"]         = "nihttpd/0.0.1";

	if ( req.action == "HEAD" ){
		response.send_headers( conn );

	} else {
		response.send_to( conn );
	}
}

void file_router::dispatch( http_request req, connection conn ){
	std::string translated = translate_filename( req.location );

	if ( !path_exists(translated) ){
		throw http_error( HTTP_404_NOT_FOUND );
	}

	// TODO: config option to enable/disable directory listings
	bool do_dir_listing = false;

	if ( path_is_directory(translated) ){
		if ( path_exists( translated + "/index.html" )){
			translated += "/index.html";
			do_dir_listing = false;

		} else {
			do_dir_listing = true;
		}
	}

	if ( do_dir_listing ){
		printf( "this is a directory!\n" );
		send_dir_listing( req, conn, translated );

	} else {
		send_file( req, conn, translated );
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

	return temp;
}
