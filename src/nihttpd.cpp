#include <nihttpd/nihttpd.h>
#include <nihttpd/socket.h>
#include <nihttpd/http.h>
#include <iostream>

#include <chrono>

using namespace nihttpd;

std::string gen_page( http_request request ){
	using namespace std::chrono;

	time_t tt = system_clock::to_time_t( system_clock::now( ));

	std::string heads = "";

	for ( const auto &s : request.headers ){
		heads += s.first + ": " + s.second + "<br />";
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
		"<h4>headers</h4>" + heads +
		"</body></html>"
		;
}

void worker( server *serv, connection conn ){
	http_request  request( conn );
	http_response response;

	response.content = gen_page( request );

	response.headers["Connection"]     = "close";
	response.headers["Content-Type"]   = "text/html";
	response.headers["Content-Length"] = std::to_string( response.content.size() );
	response.headers["Server"]         = "nihttpd/0.0.1";

	response.send_to( conn );

	serv->log( urgency::debug, "closing connection with " + conn.client_ip() );
	conn.disconnect();
}

server::server( ){
	log( urgency::info, "starting nihttpd" );

	listener *foo;

	try {
		 foo = new listener( "127.0.0.2", "8081" );

	} catch ( const std::string &msg ){
		log( urgency::high, "exception in listener: " + msg );
		throw;
	}

	while ( true ){
		connection meh = foo->accept_client();
		log( urgency::normal, "connection at " + meh.client_ip() );

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

router::router( std::string pref, std::string dest, handler hand ){
	prefix      = pref;
	destination = dest;

}
