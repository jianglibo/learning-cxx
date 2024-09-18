#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFServer.h"
#include "workflow/WFHttpServer.h"
#include "workflow/WFFacilities.h"
#include <utility>
#include "workflow/Workflow.h"
#include <fcntl.h>
#include <unistd.h>
#include "workflow/WFTaskFactory.h"

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

// main
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "%s <port> [root path] [cert file] [key file]\n",
                argv[0]);
        exit(1);
    }

    signal(SIGINT, sig_handler);

    unsigned short port = atoi(argv[1]);
    std::string scheme = "http://";

    /* Test the server. */
    auto &&create = [&scheme, port](WFRepeaterTask *) -> SubTask *
    {
        char buf[1024];
        *buf = '\0';
        printf("Input file name: (Ctrl-D to exit): ");
        scanf("%1023s", buf);
        if (*buf == '\0')
        {
            printf("\n");
            return NULL;
        }

        std::string url = scheme + "127.0.0.1:" + std::to_string(port) + "/" + buf;
        WFHttpTask *task = WFTaskFactory::create_http_task(url, 0, 0,
                                                           [](WFHttpTask *task)
                                                           {
                                                               auto *resp = task->get_resp();
                                                               if (strcmp(resp->get_status_code(), "200") == 0)
                                                               {
                                                                   std::string body = protocol::HttpUtil::decode_chunked_body(resp);
                                                                   fwrite(body.c_str(), body.size(), 1, stdout);
                                                                   printf("\n");
                                                               }
                                                               else
                                                               {
                                                                   printf("%s %s\n", resp->get_status_code(), resp->get_reason_phrase());
                                                               }
                                                           });

        return task;
    };

    WFFacilities::WaitGroup wg(1);
    WFRepeaterTask *repeater;
    repeater = WFTaskFactory::create_repeater_task(create, [&wg](WFRepeaterTask *)
                                                   { wg.done(); });

    repeater->start();
    wg.wait();
}