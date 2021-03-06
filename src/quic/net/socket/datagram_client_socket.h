// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_DATAGRAM_CLIENT_SOCKET_H_
#define NET_SOCKET_DATAGRAM_CLIENT_SOCKET_H_

#include "net/base/net_export.h"
#include "net/socket/datagram_socket.h"
#include "net/socket/socket.h"

namespace net {

class IPEndPoint;
class SocketTag;

class NET_EXPORT_PRIVATE DatagramClientSocket : public DatagramSocket,
                                                public Socket {
 public:
  ~DatagramClientSocket() override {}

  // Initialize this socket as a client socket to server at |address|.
  // Returns a network error code.
  virtual int Connect(const IPEndPoint& address) = 0;

  // Apply |tag| to this socket.
  virtual void ApplySocketTag(const SocketTag& tag) = 0;
};

}  // namespace net

#endif  // NET_SOCKET_DATAGRAM_CLIENT_SOCKET_H_
