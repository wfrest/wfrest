#include "workflow/WFFacilities.h"
#include <csignal>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main(int argc, char **argv)
{
    // in cert file 
    // sudo ./gen.sh to generate crt / key files
	if (argc != 3)
	{
		fprintf(stderr, "%s [cert file] [key file]\n",
				argv[0]);
		exit(1);
	}

    signal(SIGINT, sig_handler);

    HttpServer svr;

    // curl -v https://ip:port/https
    svr.GET("/https", [](const HttpReq *req, HttpResp *resp)
    {
        resp->String("Test Https\n");
    });

    if (svr.start(8888, argv[1], argv[2]) == 0)
    {
        wait_group.wait();
        svr.stop();
    } else
    {
        fprintf(stderr, "Cannot start server\n");
        exit(1);
    }
    return 0;
}
