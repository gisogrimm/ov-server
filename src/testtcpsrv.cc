#include "ovtcpsocket.h"

int main(int argc,char**argv)
{
  ovtcpsocket_t tcp;
  tcp.set_timeout_usec(10000);
  tcp.bind(9877);
  sleep(20);
  return 0;
}
