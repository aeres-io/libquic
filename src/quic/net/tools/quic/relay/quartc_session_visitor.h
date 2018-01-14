//
// Copyright 2017 - Filegear - All Rights Reserved
//

#ifndef MESH_QUARTC_SESSION_VISITOR_H_H
#define MESH_QUARTC_SESSION_VISITOR_H_H

#include <net/quic/quartc/quartc_session.h>

namespace net {

  class QuartcSessionVisitor {

  public:

    QuartcSessionVisitor(QuartcSession * session)
      : session(session) {
    }

    const std::string & ServerId() const {
      return this->session->unique_remote_server_id_;
    }

    Perspective GetPerspective() const {
      return this->session->perspective_;
    }

    QuicCryptoClientConfig * ClientConfig() {
      return this->session->quic_crypto_client_config_.get();
    }

    QuicCryptoServerConfig * ServerConfig() {
      return this->session->quic_crypto_server_config_.get();
    }

    QuartcCryptoServerStreamHelper * ServerStreamHelper() {
      return &this->session->stream_helper_;
    }

    void ResetCryptoStream(QuicCryptoStream * stream) {
      this->session->crypto_stream_.reset(stream);
    }

    void ResetQuicCompressedCertsCache(QuicCompressedCertsCache * val) {
      this->session->quic_compressed_certs_cache_.reset(val);
    }

  private:

    QuartcSession * session;

  };

}

#endif //MESH_QUARTC_SESSION_VISITOR_H_H
