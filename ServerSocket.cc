/*
 * Copyright ©2022 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 595 for use solely during Spring Semester 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <cstdio>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <cstring>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

namespace searchserver {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

  // This function causes the ServerSocket to attempt to create a
  // listening socket and to bind it to the given port number on
  // whatever IP address the host OS recommends for us.  The caller
  // provides:
  //
  // - ai_family: whether to create an IPv4, IPv6, or "either" listening
  //   socket.  To specify IPv4, customers pass in AF_INET.  To specify
  //   IPv6, customers pass in AF_INET6.  To specify "either" (which
  //   leaves it up to BindAndListen() to pick, in which case it will
  //   typically try IPv4 first), customers pass in AF_UNSPEC.  AF_INET6
  //   can handle IPv6 and IPv4 clients on POSIX systems, while AF_UNSPEC
  //   might pick IPv4 and not be able to accept IPv6 connections.
  //
  // On failure this function returns false.  On success, it returns
  // true, sets listen_sock_fd_ to be the file descriptor for the
  // listening socket, and also returns (via an output parameter):
  //
  // - listen_fd: the file descriptor for the listening socket.
  //              which should be the same value as listen_fd_

bool ServerSocket::bind_and_listen(int ai_family, int *listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"

  // TODO: implement

  // Populate the "hints" addrinfo structure for getaddrinfo().
  // ("man addrinfo")
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = ai_family;       // input
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use an address we can bind to a socket and accept client connections on
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;


  // Use this->port_ as the string representation of our portnumber to
  // pass in to getaddrinfo().  getaddrinfo() returns a list of
  // address structures via the output parameter "result".
  
  // cast unit to string
  char portnum[6];
  sprintf(portnum, "%u", port_);

  struct addrinfo *result;
  int res = getaddrinfo(nullptr, portnum, &hints, &result);
  if (res != 0) {
    std::cerr << "getaddrinfo() failed: ";
    std::cerr << gai_strerror(res) << std::endl;
    return -1;
  }
  
  // Loop through the returned address structures until we are able
  // to create a socket and bind to one.  The address structures are
  // linked in a list through the "ai_next" field of result.
  for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
    listen_sock_fd_ = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (listen_sock_fd_ == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      std::cerr << "socket() failed: " << strerror(errno) << std::endl;
      listen_sock_fd_ = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(listen_sock_fd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(listen_sock_fd_, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
      sock_family_ = rp->ai_family;
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(listen_sock_fd_);
    listen_sock_fd_ = -1;
  }
  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // Did we succeed in binding to any addresses?
  if (listen_sock_fd_ == -1) {
    // No.  Quit with failure.
    std::cerr << "Couldn't bind to any addresses." << std::endl;
    return false;
  }

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(listen_sock_fd_, SOMAXCONN) != 0) {
    std::cerr << "Failed to mark socket as listening: ";
    std::cerr << strerror(errno) << std::endl;
    close(listen_sock_fd_);
    listen_sock_fd_ = -1;
    return false;
  }

  // success!
  *listen_fd = listen_sock_fd_;
  return true;

}

  // This function causes the ServerSocket to attempt to accept
  // an incoming connection from a client.  On failure, returns false.
  // On success, it returns true, and also returns (via output
  // parameters) the following:
  //
  // - accepted_fd: the file descriptor for the new client connection.
  //   The customer is responsible for close()'ing this socket when it
  //   is done with it.
  //
  // - client_addr: a C++ string object containing a printable
  //   representation of the IP address the client connected from.
  //
  // - client_port: a uint16_t containing the port number the client
  //   connected from.
  //
  // - client_dnsname: a C++ string object containing the DNS name
  //   of the client.
  //
  // - server_addr: a C++ string object containing a printable
  //   representation of the server IP address for the connection.
  //
  // - server_dnsname: a C++ string object containing the DNS name
  //   of the server.
bool ServerSocket::accept_client(int *accepted_fd,
                          std::string *client_addr,
                          uint16_t *client_port,
                          std::string *client_dns_name,
                          std::string *server_addr,
                          std::string *server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // TODO: implement
    // Loop forever, accepting a connection from a client and doing
  // an echo trick to it.
  while (1) {
    // typedef struct sockaddr_storage {
    //  short   ss_family;
    //  char    __ss_pad1[_SS_PAD1SIZE];
    //  __int64 __ss_align;
    //  char    __ss_pad2[_SS_PAD2SIZE];
    // } SOCKADDR_STORAGE, *PSOCKADDR_STORAGE;

    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    int client_fd = accept(listen_sock_fd_,
                           reinterpret_cast<struct sockaddr *>(&caddr),
                           &caddr_len);
    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
      close(listen_sock_fd_);
      return false;
    } else {
      
      *accepted_fd = client_fd;

      // connection accepted
      if (caddr.ss_family == AF_INET) {
      // Print out the IPV4 address and port

      char astring[INET_ADDRSTRLEN];
      struct sockaddr_in *in4 = reinterpret_cast<struct sockaddr_in *>(&caddr);
      inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
      // std::cout << " IPv4 address " << astring;
      // std::cout << " and port " << ntohs(in4->sin_port) << std::endl;
      *client_port = ntohs(in4->sin_port);
      *client_addr = astring;
      }
      else if (caddr.ss_family == AF_INET6) {
      // Print out the IPV6 address and port

      char astring[INET6_ADDRSTRLEN];
      struct sockaddr_in6 *in6 = reinterpret_cast<struct sockaddr_in6 *>(&caddr);
      inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
      // std::cout << " IPv6 address " << astring;
      // std::cout << " and port " << ntohs(in6->sin6_port) << std::endl;
      *client_port = ntohs(in6->sin6_port);
      *client_addr = astring;
    } 
    else {
      std::cout << " ???? address and port ????" << std::endl;
    }


    // client dns
    char hostname[1024];  // ought to be big enough.

    int dns = getnameinfo(reinterpret_cast<struct sockaddr*>(&caddr), caddr_len, hostname, 1024, nullptr, 0, 0);
    if (dns != 0) {
      sprintf(hostname, "[reverse DNS failed]");
    }
    *client_dns_name = hostname;    


    // addr and port
    char hname[1024];
    hname[0] = '\0';

    if (sock_family_ == AF_INET) {
      // The server is using an IPv4 address.
      struct sockaddr_in srvr;
      socklen_t srvrlen = sizeof(srvr);
      char addrbuf[INET_ADDRSTRLEN];
      getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
      inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);

      
      // std::cout << addrbuf;
      *server_addr = std::string(addrbuf);

      // Get the server's dns name, or return it's IP address as
      // a substitute if the dns lookup fails.
      getnameinfo((const struct sockaddr *) &srvr,
                  srvrlen, hname, 1024, nullptr, 0, 0);
      // std::cout << " [" << hname << "]" << std::endl;
      *server_dns_name = std::string(hname);
      
    } else {
      // The server is using an IPv6 address.
      struct sockaddr_in6 srvr;
      socklen_t srvrlen = sizeof(srvr);
      char addrbuf[INET6_ADDRSTRLEN];
      getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
      inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
      
      // std::cout << addrbuf;
      *server_addr = std::string(addrbuf);

      // Get the server's dns name, or return it's IP address as
      // a substitute if the dns lookup fails.
      getnameinfo((const struct sockaddr *) &srvr,
                  srvrlen, hname, 1024, nullptr, 0, 0);
      
      // std::cout << " [" << hname << "]" << std::endl;
      *server_dns_name = std::string(hname);
    }

    break;
  }
  }
  return true;
}
  
}  // namespace searchserver
