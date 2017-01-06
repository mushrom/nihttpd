#include <nihttpd/test_route.h>
#include <nihttpd/nihttpd.h>
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
		"<html><head><title>nihttpd/0.0.1 test page</title>"
		"<style>"
		"body { max-width: 800px; margin-left: auto; margin-right: auto;"
		"       font-family: sans;"
		"}"
		"</style>"
		"</head><body>"
		"<b>nihttpd test</b><br/>" + (std::string)ctime(&tt) + "<br />"
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

void test_router::dispatch(http_request req, connection conn){
	http_response response;

	response.content = gen_page( req );

	response.headers["Connection"]     = "close";
	response.headers["Content-Type"]   = "text/html";
	response.headers["Content-Length"] = std::to_string( response.content.size() );
	response.headers["Server"]         = "nihttpd/0.0.1";

	response.send_to( conn );
};
