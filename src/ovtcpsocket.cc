#include "ovtcpsocket.h"
#include "errmsg.h"
#include <cstring>

ovtcpsocket_t::ovtcpsocket_t()
{
  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    throw ErrMsg("Opening socket failed: ", errno);
}

ovtcpsocket_t::~ovtcpsocket_t()
{
  close();
}

port_t ovtcpsocket_t::bind(port_t port, bool loopback)
{
  int optval = 1;
  // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval,
  //           sizeof(int));
  // windows (cast):
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval,
             sizeof(int));

  endpoint_t my_addr;
  memset(&my_addr, 0, sizeof(endpoint_t));
  /* Clear structure */
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons((unsigned short)port);
  if(loopback) {
    my_addr.sin_addr.s_addr = 0x0100007f;
  }
  if(::bind(sockfd, (struct sockaddr*)&my_addr, sizeof(endpoint_t)) == -1)
    throw ErrMsg("Binding the socket to port " + std::to_string(port) +
                     " failed: ",
                 errno);
  if(listen(sockfd, 256) < 0)
    throw ErrMsg("Unable to listen to socket: ", errno);
  socklen_t addrlen(sizeof(endpoint_t));
  getsockname(sockfd, (struct sockaddr*)&my_addr, &addrlen);
  return ntohs(my_addr.sin_port);
}

void ovtcpsocket_t::close()
{
  if(isopen)
#if defined(WIN32) || defined(UNDER_CE)
    ::closesocket(sockfd);
#else
    ::close(sockfd);
#endif
  isopen = false;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
