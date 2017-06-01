#ifndef _NIHTTPD_HTTP_H
#define _NIHTTPD_HTTP_H 1
#include <nihttpd/socket.h>
#include <map>
#include <vector>
#include <string>
#include <exception>

namespace nihttpd {
	typedef std::map<std::string, std::string> key_val_t;

	enum http_status_nums {
		HTTP_200_OK          = 200,
		HTTP_400_BAD_REQUEST = 400,
		HTTP_403_FORBIDDEN   = 403,
		HTTP_404_NOT_FOUND   = 404,
	};

	class http_request {
		public:
			// TODO: add another constructor to construct arbitary headers
			http_request( connection conn );
			std::string action;
			std::string location;
			std::string version;
			key_val_t   headers;
			key_val_t   get_params;
	};

	class http_response {
		public:
			void send_to( connection conn );
			void send_headers( connection conn );
			void send_content( connection conn, const std::vector<char> &vec );
			void set_content( std::string &str );

			std::vector<char> content;
			key_val_t headers;
			unsigned  status = HTTP_200_OK;
	};

	class http_error : public std::exception {
		public:
			http_error( unsigned num ) : std::exception() { error_num = num; };
			unsigned error_num;
	};

	std::string status_string( unsigned status );
	std::string url_decode( const std::string &str );
	std::string url_encode_path( const std::string &str );
	std::string sanitize( const std::string &str );
	bool        path_below_root( const std::string &str );
}

#endif
