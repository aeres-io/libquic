#pragma once

#include <stdio.h>
#include <string>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"


int ResolveIPAddress(std::string host, net::IPAddress& address) {
  uint32_t dwRetval;

  int i = 1;

  struct addrinfo *result = NULL;
  struct addrinfo *ptr = NULL;
  struct addrinfo hints;

  struct sockaddr_in  *sockaddr_ipv4;
  struct sockaddr_in6 *sockaddr_ipv6;

  char ipstringbuffer[46];
  

  //--------------------------------
  // Setup the hints address info structure
  // which is passed to the getaddrinfo() function
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  //--------------------------------
  // Call getaddrinfo(). If the call succeeds,
  // the result variable will hold a linked list
  // of addrinfo structures containing response
  // information
  dwRetval = getaddrinfo(host.c_str(), nullptr, &hints, &result);
  if (dwRetval != 0) {
    printf("getaddrinfo failed with error: %d\n", dwRetval);
    return 1;
  }

  printf("getaddrinfo returned success\n");
  
  // Retrieve each address and print out the hex bytes
  for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
    printf("getaddrinfo response %d\n", i++);
    printf("\tFlags: 0x%x\n", ptr->ai_flags);
    printf("\tFamily: ");
    switch (ptr->ai_family) {
      case AF_INET:
        printf("AF_INET (IPv4)\n");
        sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
        printf("\tIPv4 address %s\n", inet_ntoa(sockaddr_ipv4->sin_addr));
        address = net::IPAddress((const uint8_t*)(&sockaddr_ipv4->sin_addr.s_addr), sizeof(sockaddr_ipv4->sin_addr.s_addr));
        
        break;
      
      case AF_INET6:
        printf("AF_INET6 (IPv6)\n");
        sockaddr_ipv6 = (struct sockaddr_in6 *) ptr->ai_addr;
        printf("\tIPv6 address %s\n",
            inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr, ipstringbuffer, 46) );

        address = net::IPAddress((const uint8_t*)(&sockaddr_ipv6->sin6_addr.s6_addr), 
          sizeof(sockaddr_ipv6->sin6_addr.s6_addr));
        break;

    default:
      printf("Other %d\n", ptr->ai_family);
      break;
    }

    if ((ptr->ai_family == AF_INET || ptr->ai_family == AF_INET6))
      break;
  }

  freeaddrinfo(result);  
  return 0;
}
