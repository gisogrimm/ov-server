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
  ovtcpsocket_t tcp;
  endpoint_t ep;
  udpsocket_t udp;
  ep.sin_family = AF_INET;
  ep.sin_addr.s_addr = inet_addr("127.0.0.1");
  ep.sin_port = htons(9877);
  tcp.connect(ep);
  udp.bind(9869);
  udp.set_timeout_usec(10000);
  char buf[1024];
  endpoint_t eptmp;
  while(!quit_app) {
    ssize_t len = 0;
    if((len = udp.recvfrom(buf, 1024, eptmp)) > 0) {
      tcp.send(buf, len);
    }
  }
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
