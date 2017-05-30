#include <nihttpd/nihttpd.h>
#include <nihttpd/test_route.h>
#include <nihttpd/file_route.h>
#include <cstdio>

using namespace nihttpd;

int main( int argc, char *argv[] ){
	typedef std::unique_ptr<router> routerptr;
	server serv;

	for ( int x = 0; x < argc; x++ ){
		printf( "arg[%d]: %s\n", x, argv[x] );
	}

	serv.add_route( routerptr( new file_router("/", "/tmp/www/") ));
	serv.add_route( routerptr( new test_router("/test/") ));
	serv.add_route( routerptr( new test_router("/blarg") ));

	serv.start( "localhost", "8082" );
	serv.wait();

	return 0;
}
