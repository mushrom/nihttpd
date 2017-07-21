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
	{ ".pdf",  "application/pdf" },
	{ ".zip",  "application/zip" },
	{ ".rar",  "application/x-rar-compressed" },
	{ ".gz",   "application/octet-stream" },
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
		if ( type == "image/jpeg" || type == "image/png" ){
			// framed picture glyph
			return "\xf0\x9f\x96\xbc";
		}

		if ( type == "audio/mpeg" || type == "audio/ogg" ){
			// single musical note glyph
			return "\xf0\x9f\x8e\xb5";
		}

		if ( type == "application/pdf" ){
			// open book glyph
			return "\xf0\x9f\x93\x96";
		}
	}

	// default symbol, a 'face-up page' glyph
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

static bool trailing( std::string str, const char c ){
	return !str.empty() && *(str.rbegin()) == c;
}

static std::string dirpath( std::string str ){
	return str + (trailing(str, '/')? "" : "/");
}

static std::string gen_dir_listing( http_request req, std::string path ){
	if ( !path_is_directory( path )){
		return "<html><body><h1>wait what</h1></body></html>";
	}

	// TODO: add string sanitation once that's implemented
	//       to avoid dumb security holes and page bugs
	std::string ret =
		"<!doctype html><head>"
			"<title>Index of " + sanitize(req.location) + "</title>"
			"<meta name=\"viewport\" content=\"width=device-width, intial-scale=1.0\">"
			"<meta charset=\"UTF-8\">"
			"<style>"
				"body {"
					"margin: 0;"
					"margin-left: auto;"
					"margin-right: auto;"
					"max-width: 800px;"
					"font-family: sans;"
				"}"
				"a { color: #222; text-decoration: none; }"
				".diritem { width: 100%; }"
				".diritem:hover { background-color: #eee; }"
				".diritem:active { background-color: lightorange; }"
				".diritem b { font-weight: 500; }"
				".diricon {"
					"margin-left: auto;"
					"margin-right: auto;"
					"min-width: 25px;"
					"text-align: center;"
					"padding-left: 10px;"
					"padding-right: 10px;"
				"}"
				"table { width: 100%; margin: 0; padding: 0; border: none; }"
				"tr, td { border-bottom: 1px dotted lightgrey; }"
				"td:first-child { width: 50px; }"
				".table_head { background-color: #fee; }"
				".list_head {"
					"background-color: #eee;"
					"border-bottom: 2px solid darkorange;"
					"padding: 15px;"
				"}"
				".footer {"
					"background-color: #eee;"
					"padding: 5px;"
					"padding-left: 15px;"
					"border-top: 2px solid darkorange;"
				"}"
			"</style>"
		"</head>"
		"<body>"
			"<div class=\"list_head\">"
			"<b>Index of " + sanitize(req.location) + "</b>"
			"</div>"
			"<table cellspacing=\"0\" cellpadding=\"0\">"
		;


	DIR *dir;
	struct dirent *ent;

	if (( dir = opendir( path.c_str( ))) == NULL ){
		throw http_error( HTTP_404_NOT_FOUND );
	}

	ret += "<tr class=diritem>"
				"<td><div class=\"diricon\">" + get_file_icon("/") + "</div></td>"
				"<td>"
					"<a href=\"" + url_encode_path(dirpath(req.location)) + ".." + "\"><div>"
							"<b>..</b>"
							"<br />"
							"<small>directory</small>"
					"</div></a>"
				"</td>"
				"<td></td>"
			"</tr>";

	while (( ent = readdir( dir ))){
		std::string name( ent->d_name );
		std::string temp = dirpath(req.location);
		std::string local_path = dirpath(path) + name;
		std::string sizestr = gen_human_size( get_file_stat_size( local_path ));
		std::string symbol = get_file_icon( local_path );

		// don't generate another ".." entry, or a pointless "." entry
		if ( name == "." || name == ".." ){
			continue;
		}

		bool is_dir = path_is_directory( local_path );
		std::string type = is_dir? "directory" : get_content_type( local_path );

		// append a trailing slash if the entry is a directory
		if ( is_dir ){
			name += "/";
		}

		ret +=
			"<tr class=diritem>"
				"<td><div class=\"diricon\">" + symbol + "</div></td>"
				"<td>"
					"<a href=\"" + url_encode_path(temp) + url_encode_path(name) + "\"><div>"
							"<b>" + sanitize(name) + "</b>"
							"<br />"
							"<small>" + type + "</small>"
					"</div></a>"
				"</td>"
				"<td><code>" + sizestr + "</code></td>"
			"</tr>"
		;
	}

	closedir( dir );

	ret += "</table>";
	ret += "<div class=\"footer\">"
				"<small>nihttpd 0.0.1</small>"
			"</div></body></html>";

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
	if ( !path_below_root( req.location )){
		throw http_error( HTTP_403_FORBIDDEN );
	}

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
