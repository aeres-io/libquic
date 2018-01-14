//
// Copyright 2017 - Filegear - All Rights Reserved
//

#ifndef MESH_QUIC_CRYPTO_CLIENT_STREAM_VISITOR_H
#define MESH_QUIC_CRYPTO_CLIENT_STREAM_VISITOR_H

#include <net/quic/core/quic_crypto_client_stream.h>

namespace net {

  class QuicCryptoClientStreamVisitor {

  public:

    QuicCryptoClientStreamVisitor(QuicCryptoClientStream * stream)
        : stream(stream) {
    }


    QuicCryptoClientStream * get() {
      return this->stream;
    }


    void ResetHandshaker(QuicCryptoClientStream::HandshakerDelegate * handshaker) {
      this->stream->handshaker().reset(handshaker);
    }


  private:

    QuicCryptoClientStream * stream;

  };

}

#endif //MESH_QUIC_CRYPTO_CLIENT_STREAM_VISITOR_H
