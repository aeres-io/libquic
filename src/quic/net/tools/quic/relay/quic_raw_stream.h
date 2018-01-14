#pragma once

#include "net/quic/core/quic_session.h"
#include "net/quic/core/quic_stream.h"
#include "net/quic/platform/api/quic_export.h"
#include "net/quic/quartc/quartc_stream_interface.h"

namespace net {

class QuicRawStreamDelegate : public QuartcStreamInterface::Delegate {
public:

  void OnReceived(QuartcStreamInterface * stream,
      const char * data, size_t size) override;
  void OnClose(QuartcStreamInterface * stream) override;
  void OnCanWrite(QuartcStreamInterface* stream) override;

};

typedef QuicRawStreamDelegate QuicRawServerStreamDelegate;

}
