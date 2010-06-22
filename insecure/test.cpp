#include <ucommon/secure.h>

using namespace UCOMMON_NAMESPACE;

int main(int argc, char **argv)
{
	secure::init();

	ssl_t ssl("http", NULL);
	ssl.open("www.google.com");

	ssl.writes("GET /\r\n\r\n");
	ssl.flush();

	char buf[256];

	while(!ssl.eof()) {
		ssl.getline(buf, sizeof(buf));
		printf("%s\n", buf);
	}	

    digest_t md5 = "md5";

    md5.puts("this is some text");
    printf("result %d %s\n", md5.size(), *md5);

	md5 = "sha1";
	md5.puts("this is some text");
	 printf("result %d %s\n", md5.size(), *md5);

	random::seed();

	printf("random %d %d %d\n", random::get(), random::get(), random::get());

}

