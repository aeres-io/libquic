//
// Copyright 2017 - Filegear - All Rights Reserved
//

#ifndef MESH_QUIC_RAW_CONNECTION_H
#define MESH_QUIC_RAW_CONNECTION_H

#include <net/quic/core/quic_connection.h>

namespace net {

  class QuicConnectionVisitor {

  public:

    QuicConnectionVisitor(QuicConnection * connection)
      : connection(connection) {
    }

    QuicConnection * get()  {
      return this->connection;
    }

    QuicConnectionVisitorInterface * Visitor() {
      return this->connection->get_visitor();
    }

    EncryptionLevel LastDecryptedPacketLevel() {
      return this->connection->last_decrypted_packet_level_;
    }

    QuicPacketHeader & LastHeader() {
      return this->connection->last_header_;
    }

    QuicConnectionStats & Stats() {
      return this->connection->stats_;
    }

    void SetShouldLastPacketInstigateAcks(bool value) {
      this->connection->should_last_packet_instigate_acks_ = value;
    }

    bool MaybeConsiderAsMemoryCorruption(const QuicStreamFrame& frame) {
      return this->connection->MaybeConsiderAsMemoryCorruption(frame);
    }

  private:

    QuicConnection * connection;
  };


  class QuicRawConnection : public QuicConnection {

  public:
    QuicRawConnection(
      QuicConnectionId connection_id,
      IPEndPoint address,
      QuicConnectionHelper* helper,
      QuicAlarmFactory* alarm_factory,
      QuicPacketWriter* writer,
      bool owns_writer,
      Perspective perspective,
      const net::ParsedQuicVersionVector& supported_versions);

    ~QuicRawConnection() override;

    bool OnStreamFrame(const QuicStreamFrame& frame) override;

    bool OnConnectionCloseFrame(const QuicConnectionCloseFrame& frame) override {
      return this->QuicConnection::OnConnectionCloseFrame(frame);
    }
  };

}

#endif //MESH_QUIC_RAW_CONNECTION_H
