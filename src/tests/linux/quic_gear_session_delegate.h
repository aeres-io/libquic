#pragma once

#include "base/macros.h"
#include "net/quic/core/quic_crypto_server_stream.h"
#include "net/quic/core/quic_packets.h"

#include "net/quic/quartc/quartc_session.h"
#include "net/quic/quartc/quartc_stream.h"

#include <map>
#include <vector>

namespace net {

class QuicRawSession;

class QuicGearSessionDelegate :
  public QuartcSessionInterface::Delegate {

public:

  QuicGearSessionDelegate(QuicRawSession * target);

  ~QuicGearSessionDelegate() override;

  void OnCryptoHandshakeComplete() override;

  void OnIncomingStream(QuartcStreamInterface * stream) override;

  void OnConnectionClosed(int error_code, bool from_remote) override;

private:

  std::unique_ptr<QuartcStreamInterface::Delegate> stream_delegate_;
  std::map<int, QuartcStreamInterface *> relayed_tcp_connections_map_;

  QuicRawSession * session_;       // unowned

  std::set<QuartcStreamInterface::Delegate *> stream_delegates_;

};

}
