//
// Copyright 2017 - Filegear - All Rights Reserved
//

#ifndef MESH_QUIC_CRYPTO_SERVER_STREAM_VISITOR_H
#define MESH_QUIC_CRYPTO_SERVER_STREAM_VISITOR_H

#include <net/quic/core/quic_crypto_server_stream.h>

namespace net {

  class QuicCryptoServerStreamVisitor {

  public:

    QuicCryptoServerStreamVisitor(QuicCryptoServerStream * stream)
        : stream(stream) {
    }


    QuicCryptoServerStream * get() {
      return this->stream;
    }


    void ResetHandshaker(QuicCryptoServerStream::HandshakerDelegate * handshaker) {
      this->stream->handshaker_.reset(handshaker);
    }


  private:

    QuicCryptoServerStream * stream;

  };

}

#endif //MESH_QUIC_CRYPTO_SERVER_STREAM_VISITOR_H
