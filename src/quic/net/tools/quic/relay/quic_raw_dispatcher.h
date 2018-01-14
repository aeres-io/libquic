// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_QUIC_P2P_QUIC_RAW_DISPATCHER_H_
#define NET_TOOLS_QUIC_P2P_QUIC_RAW_DISPATCHER_H_

#include <memory>
#include <string>
#include "net/tools/quic/quic_dispatcher.h"
#include "net/tools/quic/relay/quic_raw_session.h"

namespace net {

// server component
// used to dispatch connections from multiple clients
class QuicRawDispatcher : public QuicDispatcher {
 public:
   class DispatcherDelegate {
    public:
     virtual ~DispatcherDelegate() = default;
     virtual void OnNewSession(QuicRawSession * session) = 0;
   };
 
 public:
  QuicRawDispatcher(
      const QuicConfig& config,
      const QuicCryptoServerConfig* crypto_config,
      QuicVersionManager* version_manager,
      std::unique_ptr<QuicConnectionHelper> helper,
      std::unique_ptr<QuicCryptoServerStream::Helper> session_helper,
      std::unique_ptr<QuicAlarmFactory> alarm_factory,
      const std::string & unique_server_id
      );

  ~QuicRawDispatcher() override;

  DispatcherDelegate * GetDispatcherDelegate() {
    return dispatcher_delegate_.get();
  }

  void ResetDispatcherDelegate(DispatcherDelegate * val) {
    dispatcher_delegate_.reset(val);
  }

 protected:
  QuicRawSession* CreateQuicSession(
      QuicConnectionId connection_id,
      const IPEndPoint& client_address,
      base::StringPiece alpn) override;

 private:
  std::string unique_server_id_;

  std::unique_ptr<DispatcherDelegate> dispatcher_delegate_;
};

}  // namespace net

#endif  // NET_TOOLS_QUIC_P2P_QUIC_RAW_DISPATCHER_H_
