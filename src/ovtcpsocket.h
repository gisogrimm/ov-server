#ifndef OVTCPSOCKET_H
#define OVTCPSOCKET_H

#include "udpsocket.h"
#include <thread>

class ovtcpsocket_t {
public:
  /**
   * Default constructor.
   *
   * This call will open a TCP socket of AF_INET domain. If the socket
   * creation failed, an exception of type ErrMsg with an appropriate
   * error message is thrown.
   */
  ovtcpsocket_t();
  /**
   * Deconstructor. This will close the socket.
   */
  ~ovtcpsocket_t();
  /**
   * Set the receive timeout.
   *
   * @param usec Timeout in Microseconds, or zero to set to blocking
   * mode (default)
   * @param fd File descriptor, or -1 to use primary socket
   */
  void set_timeout_usec(int usec, int fd = -1);
  /**
   * Set priority for all packets (SO_PRIORITY)
   *
   * @param priority Priority value, 0 to 6.
   */
  void set_netpriority(int priority);
  /**
   * Set flags for low loss, low latency, low jitter, assured
   * bandwidth, end-to-end service according to RFC2598
   */
  void set_expedited_forwarding_PHB();
  /**
   * Bind the socket to a port.
   *
   * @param port Port number to bind the port to.
   * @param loopback Use loopback device (otherwise use 0.0.0.0)
   * @return The actual port used.
   *
   * Upon error (the underlying system call returned -1), an exception
   * of type ErrMsg with an appropriate error message is thrown.
   */
  port_t bind(port_t port, bool loopback = false);
  int connect(endpoint_t ep);
  /**
   * Close the socket.
   */
  void close();
  int nbread(int fd, uint8_t* buf, size_t cnt);
  ssize_t send(const char* buf, size_t len);


  virtual void handleconnection(int fd, endpoint_t ep);

private:
  void acceptor();
  int sockfd = -1;
  endpoint_t serv_addr;
  bool isopen = false;
  std::atomic_bool run_server = true;
  std::thread mainthread;
  std::map<int, std::thread> handlethreads;

public:
  /**
   * Number of bytes sent through this socket.
   */
  std::atomic_size_t tx_bytes = 0u;
  /**
   * Number of bytes received through this socket.
   */
  std::atomic_size_t rx_bytes = 0u;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
