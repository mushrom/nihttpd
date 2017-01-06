#ifndef _NIHTTPD_TEST_ROUTE_H
#define _NIHTTPD_TEST_ROUTE_H 1

#include <nihttpd/nihttpd.h>
#include <nihttpd/socket.h>
#include <nihttpd/http.h>

namespace nihttpd {
	class test_router : public router {
		public:
			test_router( )                  : router( "/",  "/srv/http" ){ };
			test_router( std::string pref ) : router( pref, "/srv/http" ){ };

			virtual void dispatch(http_request req, connection conn);
	};
}

#endif
