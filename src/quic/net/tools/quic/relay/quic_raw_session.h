#pragma once

#include <stdint.h>

#include <deque>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "net/quic/core/quic_crypto_server_stream.h"
#include "net/quic/core/quic_packets.h"


#include "net/quic/quartc/quartc_session.h"
#include "net/quic/quartc/quartc_stream.h"

#include "net/tools/quic/relay/quic_raw_stream.h"

namespace net {

class QuicRawSession : public QuartcSession {

public:

  QuicRawSession(std::unique_ptr<QuicConnection> connection,
      const QuicConfig & config,
      const std::string & remote_fingerprint_value,
      Perspective perspective,
      QuicConnectionHelper * helper);

  ~QuicRawSession() override;

  int GetNumSentClientHellos();
  int GetNumReceivedServerConfigUpdates();

  void StartCryptoHandshake() override;
};

}
