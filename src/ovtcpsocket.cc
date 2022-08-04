#include "ovtcpsocket.h"
#include "errmsg.h"
#include <cstring>

ovtcpsocket_t::ovtcpsocket_t()
{
  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    throw ErrMsg("Opening socket failed: ", errno);
  set_netpriority(6);
}

ovtcpsocket_t::~ovtcpsocket_t()
{
  close();
  if(mainthread.joinable())
    mainthread.join();
  for(auto& thr : handlethreads)
    if(thr.joinable())
      thr.join();
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
  run_server = true;
  mainthread = std::thread(&ovtcpsocket_t::acceptor, this);
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
  run_server = false;
}

void ovtcpsocket_t::acceptor()
{
  DEBUG("waiting");
  while(run_server) {
    int clientfd = accept(sockfd, NULL, NULL);
    if(clientfd >= 0) {
      handlethreads.push_back(
          std::thread(&ovtcpsocket_t::handleconnection, this, clientfd));
    }
  }
  DEBUG("server closed");
}

void ovtcpsocket_t::handleconnection(int fd)
{
  DEBUG(fd);
  DEBUG("connection established");
  char c = '\0';
  while(run_server) {
    int cnt = read(fd, &c, 1);
    if(cnt == -1) {
      if(!((errno == EAGAIN) || (errno == EWOULDBLOCK)))
        break;
    } else {
      if(cnt > 0) {
        std::cerr << c;
      }else{
        break;
      }
    }
  }
  DEBUG("closing");
  DEBUG(fd);
  ::close(fd);
}

void ovtcpsocket_t::set_timeout_usec(int usec)
{
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = usec;
#if defined(WIN32) || defined(UNDER_CE)
  // windows (cast):
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#else
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

void ovtcpsocket_t::set_netpriority(int priority)
{
#ifndef __APPLE__
  // gnu SO_PRIORITY
  // IP_TOS defined in ws2tcpip.h
#if defined(WIN32) || defined(UNDER_CE)
  // no documentation on what SO_PRIORITY does, optname, level in GNU socket
  setsockopt(sockfd, SOL_SOCKET, SO_GROUP_PRIORITY,
             reinterpret_cast<const char*>(&priority), sizeof(priority));
#else
  // on linux:
  setsockopt(sockfd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));
#endif
#endif
}

void ovtcpsocket_t::set_expedited_forwarding_PHB()
{
#ifndef __APPLE__
  int iptos = IPTOS_DSCP_EF;
  setsockopt(sockfd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
#endif
  set_netpriority(6);
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
