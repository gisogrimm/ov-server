#include "ovtcpsocket.h"
#include <signal.h>

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
  tcp.bind(9877);
  while(!quit_app)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return 0;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
