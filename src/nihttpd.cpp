#include <nihttpd/nihttpd.h>
#include <nihttpd/socket.h>
#include <nihttpd/http.h>
#include <iostream>

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

connection server::wait_for_connection(void){
	{
		// notify master thread, in case it's waiting because all workers are busy
		std::unique_lock<std::mutex> slock(listen_lock);
		available_threads++;
	}

	master.notify_one();

	{
		std::unique_lock<std::mutex> slock(listen_lock);
		waiter.wait(slock, [this]{ return pending_connects.size() > 0; });

		connection ret = pending_connects.front();
		pending_connects.pop_front();
		available_threads--;

		return ret;
	}
}

void server::worker(void){
	while (true){
		connection conn = wait_for_connection();

		try {
			http_request request( conn );
			router *route = find_route( request.location );

			log( urgency::normal,
					"  " + conn.client_ip() +
					": " + request.action + " " + request.location );

			if ( route == nullptr ){
				throw http_error( HTTP_404_NOT_FOUND );
			}

			route->dispatch( request, conn );

		} catch ( const http_error &err ){
			log( urgency::normal,
					"  " + conn.client_ip() + ": request aborted with error: "
					+ status_string( err.error_num ));

			http_response res = gen_error_page( err );
			res.send_to( conn );
		}

		log( urgency::debug, "  closing connection" );
		conn.disconnect();
	}
}

server::server( ){
}

void server::start( const std::string &host, const std::string &port ){
	log( urgency::info, "starting nihttpd" );

	try {
		sock = new listener( host, port );
		self = std::thread( &server::run, this );

		unsigned concurrency = 2 * (std::thread::hardware_concurrency() + 1);

		for (unsigned i = 0; i < concurrency; i++){
			workers.push_front(std::thread(&server::worker, this));
			log(urgency::debug, "started worker thread " + std::to_string(i));
		}

	} catch ( const std::string &msg ){
		log( urgency::high, "exception in listener: " + msg );
		throw;
	}

	running = true;
}

void server::wait( void ){
	if ( running ){
		self.join();
	}
}

void server::run( void ){
	while ( true ){
		connection meh = sock->accept_client();
		log( urgency::normal, "connection from " + meh.client_ip() );

		{
			log( urgency::debug, "trying to queue " + meh.client_ip() );
			std::lock_guard<std::mutex> slock(listen_lock);
			pending_connects.push_back(meh);
			log( urgency::debug, "queued " + meh.client_ip() );
		}

		log( urgency::debug, "notifying " + meh.client_ip() );

		{
			std::unique_lock<std::mutex> slock(listen_lock);

			if (available_threads == 0) {
				log(urgency::debug, "no available worker threads, waiting...");
				master.wait(slock, [this]{ return available_threads > 0; });
				log(urgency::debug, "woke");
			}
		}

		waiter.notify_one();
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
