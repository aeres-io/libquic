//
// Copyright 2017 - Filegear - All Rights Reserved
//

#include "net/tools/quic/relay/quic_raw_connection.h"


namespace net {

#define ENDPOINT \
  (perspective() == Perspective::IS_SERVER ? "Server: " : "Client: ")


  QuicRawConnection::QuicRawConnection(
      QuicConnectionId connection_id,
      IPEndPoint address,
      QuicConnectionHelper *helper,
      QuicAlarmFactory *alarm_factory,
      QuicPacketWriter *writer,
      bool owns_writer,
      Perspective perspective,
      const ParsedQuicVersionVector &supported_versions)
      : QuicConnection(
          connection_id,
          address,
          helper,
          alarm_factory,
          writer,
          owns_writer,
          perspective,
          supported_versions
        )
  {
  }


  QuicRawConnection::~QuicRawConnection() {
  }


  bool QuicRawConnection::OnStreamFrame(const QuicStreamFrame &frame) {
    QuicConnectionVisitor parent(this);


#ifdef USE_QUIC_ENCRYPTION
    if (frame.stream_id != kCryptoStreamId &&
        parent.LastDecryptedPacketLevel() == ENCRYPTION_NONE) {
      if (parent.MaybeConsiderAsMemoryCorruption(frame)) {
        CloseConnection(QUIC_MAYBE_CORRUPTED_MEMORY,
                        "Received crypto frame on non crypto stream.",
                        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
        return false;
      }

      CloseConnection(QUIC_UNENCRYPTED_STREAM_DATA,
                      "Unencrypted stream data seen.",
                      ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
      return false;
    }
#endif
    parent.Visitor()->OnStreamFrame(frame);
    parent.Visitor()->PostProcessAfterData();
    parent.Stats().stream_bytes_received += frame.data_length;
    parent.SetShouldLastPacketInstigateAcks(true);
    return this->connected();
  }
}