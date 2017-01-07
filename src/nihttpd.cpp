#include <nihttpd/nihttpd.h>
#include <nihttpd/socket.h>
#include <nihttpd/http.h>
#include <iostream>

#include <nihttpd/test_route.h>
#include <nihttpd/file_route.h>

using namespace nihttpd;

static http_response gen_error_page( http_error err ){
	http_response ret;
	std::string str = status_string( err.error_num );

	std::string cont =
		"<!doctype html><html>"
		"<head><title>" + str + "</title></head>"
		"<body><h2>" + str + "</h2>"
		"</body></html>";

	ret.set_content( cont );

	ret.status = err.error_num;
	ret.headers["Connection"]     = "close";
	ret.headers["Content-Type"]   = "text/html";
	ret.headers["Content-Length"] = std::to_string( ret.content.size() );
	ret.headers["Server"]         = "nihttpd/0.0.1";

	return ret;
}

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

	serv->log( urgency::debug, "  closing connection" );
	conn.disconnect();
}

server::server( ){
	log( urgency::info, "starting nihttpd" );

	listener *foo;

	try {
		 foo = new listener( "127.0.0.1", "8082" );

		 typedef std::unique_ptr<router> routerptr;
		 add_route( routerptr( new file_router("/", "/tmp/www/") ));
		 add_route( routerptr( new test_router("/test/") ));
		 add_route( routerptr( new test_router("/blarg") ));

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

router::router( std::string pref, std::string dest ){
	prefix      = pref;
	destination = dest;
}

void router::dispatch( http_request req, connection conn ){
	// do nothing
	;
}
