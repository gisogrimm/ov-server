#ifndef OVTCPSOCKET_H
#define OVTCPSOCKET_H

#include "udpsocket.h"

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
  /**
   * Close the socket.
   */
  void close();

private:
  int sockfd = -1;
  endpoint_t serv_addr;
  bool isopen = false;

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
