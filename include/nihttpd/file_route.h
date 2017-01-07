#ifndef _NIHTTPD_FILE_ROUTE_H
#define _NIHTTPD_FILE_ROUTE_H 1

#include <nihttpd/nihttpd.h>
#include <nihttpd/socket.h>
#include <nihttpd/http.h>

namespace nihttpd {
	class file_router : public router {
		public:
			file_router( std::string pref, std::string dest )
				: router( pref, dest ){ };

			virtual void dispatch( http_request req, connection conn );

		private:
			std::string translate_filename( std::string filename );
	};
}

#endif
