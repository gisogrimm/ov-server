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
  set_timeout_usec(10000);
}

ovtcpsocket_t::~ovtcpsocket_t()
{
  close();
  if(mainthread.joinable())
    mainthread.join();
  for(auto& thr : handlethreads)
    if(thr.second.joinable())
      thr.second.join();
}

int ovtcpsocket_t::connect(endpoint_t ep)
{
  return ::connect(sockfd, (const struct sockaddr*)(&ep), sizeof(ep));
}

port_t ovtcpsocket_t::bind(port_t port, bool loopback)
{
  int optval = 1;
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
  targetport = ntohs(my_addr.sin_port);
  return targetport;
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
  std::cerr << "server ready to accept connections.\n";
  while(run_server) {
    endpoint_t ep;
    unsigned int len = sizeof(ep);
    int clientfd = accept(sockfd, (struct sockaddr*)(&ep), &len);
    if(clientfd >= 0) {
      if(handlethreads.count(clientfd)) {
        if(handlethreads[clientfd].joinable())
          handlethreads[clientfd].join();
      }
      handlethreads[clientfd] =
          std::thread(&ovtcpsocket_t::handleconnection, this, clientfd, ep);
    }
  }
  std::cerr << "server closed\n";
}

int ovtcpsocket_t::nbread(int fd, uint8_t* buf, size_t cnt)
{
  int rcnt = 0;
  while(run_server && (cnt > 0)) {
    int lrcnt = read(fd, buf, cnt);
    if(lrcnt < 0) {
      if(!((errno == EAGAIN) || (errno == EWOULDBLOCK)))
        return lrcnt;
    } else {
      if(lrcnt > 0) {
        cnt -= lrcnt;
        buf += lrcnt;
        rcnt += lrcnt;
      } else {
        return -2;
      }
    }
  }
  return rcnt;
}

int ovtcpsocket_t::nbwrite(int fd, uint8_t* buf, size_t cnt)
{
  int rcnt = 0;
  while(run_server && (cnt > 0)) {
    int lrcnt = write(fd, buf, cnt);
    if(lrcnt < 0) {
      if(!((errno == EAGAIN) || (errno == EWOULDBLOCK)))
        return lrcnt;
    } else {
      if(lrcnt > 0) {
        cnt -= lrcnt;
        buf += lrcnt;
        rcnt += lrcnt;
      } else {
        return -2;
      }
    }
  }
  return rcnt;
}

ssize_t ovtcpsocket_t::send(const char* buf, size_t len)
{
  if(len >= 1 << 16)
    return -3;
  char csize[2];
  csize[0] = len & 0xff;
  csize[1] = (len >> 8) & 0xff;
  auto wcnt = nbwrite(sockfd, (uint8_t*)csize, 2);
  if(wcnt < 2)
    return -4;
  return nbwrite(sockfd, (uint8_t*)buf, len);
}

void ovtcpsocket_t::handleconnection(int fd, endpoint_t ep)
{
  std::cerr << "connection from " << ep2str(ep) << " established\n";
  udpsocket_t udp;
  port_t udpport = udp.bind(0);
  DEBUG(udpport);
  udp.set_destination("localhost");
  port_t targetport = get_port();
  DEBUG(targetport);
  uint8_t buf[1 << 16];
  while(run_server) {
    char csize[2] = {0, 0};
    int cnt = nbread(fd, (uint8_t*)csize, 2);
    if(cnt < 0)
      break;
    uint16_t size = csize[0] + (csize[1] << 8);
    if(cnt == sizeof(size)) {
      // read package:
      DEBUG(size);
      cnt = nbread(fd, buf, size);
      DEBUG(cnt);
      buf[cnt] = 0;
      if(cnt == size) {
        udp.send( (char*)buf, cnt, targetport );
      }
    }
  }
  std::cerr << "closing connection from " << ep2str(ep) << "\n";
  ::close(fd);
}

void ovtcpsocket_t::set_timeout_usec(int usec, int fd)
{
  if(fd == -1)
    fd = sockfd;
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
