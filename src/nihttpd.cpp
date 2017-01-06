#include <nihttpd/nihttpd.h>
#include <nihttpd/socket.h>
#include <nihttpd/http.h>
#include <iostream>

#include <chrono>

using namespace nihttpd;

class client_info {
	public:
		client_info( std::string agent, std::string atime ){
			user_agent  = agent;
			access_time = atime;
		}

		std::string user_agent;
		std::string access_time;
};

static std::string gen_page( http_request request ){
	using namespace std::chrono;

	time_t tt = system_clock::to_time_t( system_clock::now( ));

	static unsigned rendered = 0;
	static std::list<client_info> accessed_list;

	client_info info( request.headers["User-Agent"], (std::string)ctime(&tt) );
	accessed_list.push_back( info );

	std::string heads = "";
	for ( const auto &s : request.headers ){
		heads += "<code>" + s.first + ":" + s.second + "</code><br />";
	}

	std::string clients = "";
	for ( const auto &client : accessed_list ){
		clients += "<code>" + client.access_time + ", "
			+ client.user_agent + "</code><br />";
	}

	return
		"<!doctype html>"
		"<html><head><title>nihttpd test page</title></head><body>"
		"<b>sup, sent at " + (std::string)ctime(&tt) + "</b><br />"
		"<hr />"
		"action: "   + request.action + "<br/>"
		"location: " + request.location + "<br/>"
		"version: "  + request.version + "<br/>"
		"<hr />"
		"<h4>headers</h4>" + heads + "<br />"
		"<hr />"
		"<h4>misc. info</h4>"
		"this page rendered: " + std::to_string( ++rendered ) + " times<br />"
		"accessed by: <br />" + clients +
		"</body></html>"
		;
}

static http_response gen_error_page( http_error err ){
	http_response ret;
	std::string str = status_string( err.error_num );

	ret.status = err.error_num;
	ret.content =
		"<!doctype html><html>"
		"<head><title>" + str + "</title></head>"
		"<body><h2>" + str + "</h2>"
		"</body></html>";

	ret.headers["Connection"]     = "close";
	ret.headers["Content-Type"]   = "text/html";
	ret.headers["Content-Length"] = std::to_string( ret.content.size() );
	ret.headers["Server"]         = "nihttpd/0.0.1";

	return ret;
}

class test_router : public router {
	public:
		test_router( )                  : router( "/",  "/srv/http", func ){ };
		test_router( std::string pref ) : router( pref, "/srv/http", func ){ };

	private:
		static void func(http_request req, connection conn){
			http_response response;

			response.content = gen_page( req );

			response.headers["Connection"]     = "close";
			response.headers["Content-Type"]   = "text/html";
			response.headers["Content-Length"] = std::to_string( response.content.size() );
			response.headers["Server"]         = "nihttpd/0.0.1";

			response.send_to( conn );
		};
};

void worker( server *serv, connection conn ){
	try {
		http_request request( conn );
		router *route = serv->find_route( request.location );

		serv->log( urgency::normal,
		           "  " + conn.client_ip() +
		           ": " + request.action + " " + request.location );

		if ( route == nullptr ){
			throw http_error( HTTP_404_NOT_FOUND );
		}

		route->dispatch( request, conn );

	} catch ( const http_error &err ){
		serv->log( urgency::normal,
		           "  " + conn.client_ip() + ": request aborted with error: "
		           + status_string( err.error_num ));

		http_response res = gen_error_page( err );
		res.send_to( conn );
	}

	serv->log( urgency::debug, "  closing connection with " + conn.client_ip() );
	conn.disconnect();
}

server::server( ){
	log( urgency::info, "starting nihttpd" );

	listener *foo;

	try {
		 foo = new listener( "127.0.0.1", "8082" );

		 add_route( std::unique_ptr<router>( new test_router("/test/") ));

	} catch ( const std::string &msg ){
		log( urgency::high, "exception in listener: " + msg );
		throw;
	}

	while ( true ){
		connection meh = foo->accept_client();
		log( urgency::normal, "connection from " + meh.client_ip() );

		std::thread thr( worker, this, meh );
		thr.detach();
	}
}

void server::log( urgency level, const std::string &msg ){
	switch ( level ){
		case urgency::debug:    std::cout << "[debug] "; break;
		case urgency::low:      std::cout << "[low]   "; break;
		case urgency::normal:   std::cout << "        "; break;
		case urgency::info:     std::cout << "[info]  "; break;
		case urgency::high:     std::cout << "[high]  "; break;
		case urgency::critical: std::cout << "[!!!]   "; break;
	}

	std::cout << "> " << msg << std::endl;
}

void server::add_route( std::unique_ptr<router> rut ){
	routes.push_front( std::move( rut ));
}

router *server::find_route( std::string location ){
	for ( const auto &x : routes ){
		unsigned len = x->prefix.length();

		if ( x->prefix == location.substr(0, len) ){
			return x.get();
		}
	}

	return nullptr;
}

router::router( std::string pref, std::string dest, handler hand ){
	prefix      = pref;
	destination = dest;
	handle      = hand;
}

void router::dispatch( http_request req, connection conn ){
	handle( req, conn );
}
