#include "ovtcpsocket.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>

static bool quit_app = false;

static void sighandler(int sig)
{
  quit_app = true;
}

int main(int argc, char** argv)
{
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  ovtcpsocket_t tcp(9869);
  endpoint_t ep;
  ep.sin_family = AF_INET;
  ep.sin_addr.s_addr = inet_addr("127.0.0.1");
  ep.sin_port = htons(9877);
  tcp.connect(ep, 9999);
  while(!quit_app)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
