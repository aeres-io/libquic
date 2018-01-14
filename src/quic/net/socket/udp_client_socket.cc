// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/udp_client_socket.h"

#include "net/base/net_errors.h"

namespace net {

UDPClientSocket::UDPClientSocket(DatagramSocket::BindType bind_type,
                                 const RandIntCallback& rand_int_cb)
    : socket_(bind_type, rand_int_cb) {}

UDPClientSocket::~UDPClientSocket() = default;

int UDPClientSocket::Connect(const IPEndPoint& address) {
  int rv = socket_.Open(address.GetFamily());
  if (rv != OK)
    return rv;
  return socket_.Connect(address);
}

void UDPClientSocket::ApplySocketTag(const SocketTag& tag) {
  socket_.ApplySocketTag(tag);
}

int UDPClientSocket::Read(IOBuffer* buf,
                          int buf_len,
                          const CompletionCallback& callback) {
  return socket_.Read(buf, buf_len, callback);
}

int UDPClientSocket::Write(
    IOBuffer* buf,
    int buf_len,
    const CompletionCallback& callback) {
  return socket_.Write(buf, buf_len, callback);
}

void UDPClientSocket::Close() {
  socket_.Close();
}

int UDPClientSocket::GetPeerAddress(IPEndPoint* address) const {
  return socket_.GetPeerAddress(address);
}

int UDPClientSocket::GetLocalAddress(IPEndPoint* address) const {
  return socket_.GetLocalAddress(address);
}

int UDPClientSocket::SetReceiveBufferSize(int32_t size) {
  return socket_.SetReceiveBufferSize(size);
}

int UDPClientSocket::SetSendBufferSize(int32_t size) {
  return socket_.SetSendBufferSize(size);
}

int UDPClientSocket::SetDoNotFragment() {
  return socket_.SetDoNotFragment();
}

void UDPClientSocket::UseNonBlockingIO() {
#if defined(OS_WIN)
  socket_.UseNonBlockingIO();
#endif
}

}  // namespace net
