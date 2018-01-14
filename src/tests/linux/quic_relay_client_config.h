#pragma once

#include <stdint.h>
#include <string>
#include "net/base/ip_address.h"

namespace net
{

struct QuicRelayClientConfig
{
  static int kTcpServicePort;
  static IPAddress kIpAddress;
};

}
