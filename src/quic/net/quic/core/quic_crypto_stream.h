// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_CORE_QUIC_CRYPTO_STREAM_H_
#define NET_QUIC_CORE_QUIC_CRYPTO_STREAM_H_

#include <cstddef>

#include "base/macros.h"
#include "net/quic/core/crypto/crypto_framer.h"
#include "net/quic/core/crypto/crypto_utils.h"
#include "net/quic/core/quic_config.h"
#include "net/quic/core/quic_packets.h"
#include "net/quic/core/quic_stream.h"
#include "net/quic/platform/api/quic_export.h"
#include "base/strings/string_piece.h"

namespace net {

class QuicSession;

// Crypto handshake messages in QUIC take place over a reserved stream with the
// id 1.  Each endpoint (client and server) will allocate an instance of a
// subclass of QuicCryptoStream to send and receive handshake messages.  (In the
// normal 1-RTT handshake, the client will send a client hello, CHLO, message.
// The server will receive this message and respond with a server hello message,
// SHLO.  At this point both sides will have established a crypto context they
// can use to send encrypted messages.
//
// For more details:
// https://docs.google.com/document/d/1g5nIXAIkN_Y-7XJW5K45IblHd_L2f5LTaDUDwvZ5L6g/edit?usp=sharing
class QUIC_EXPORT_PRIVATE QuicCryptoStream : public QuicStream {
 public:
  explicit QuicCryptoStream(QuicSession* session);

  ~QuicCryptoStream() override;

  // Returns the per-packet framing overhead associated with sending a
  // handshake message for |version|.
  static QuicByteCount CryptoMessageFramingOverhead(
      QuicTransportVersion version);

  // QuicStream implementation
  void OnDataAvailable() override;

  // Performs key extraction to derive a new secret of |result_len| bytes
  // dependent on |label|, |context|, and the stream's negotiated subkey secret.
  // Returns false if the handshake has not been confirmed or the parameters are
  // invalid (e.g. |label| contains null bytes); returns true on success.
  bool ExportKeyingMaterial(base::StringPiece label,
                            base::StringPiece context,
                            size_t result_len,
                            std::string* result) const;

  // Performs key extraction for Token Binding. Unlike ExportKeyingMaterial,
  // this function can be called before forward-secure encryption is
  // established. Returns false if initial encryption has not been established,
  // and true on success.
  //
  // Since this depends only on the initial keys, a signature over it can be
  // repurposed by an attacker who obtains the client's or server's DH private
  // value.
  bool ExportTokenBindingKeyingMaterial(std::string* result) const;

  // Writes |data| to the QuicStream.
  virtual void WriteCryptoData(const base::StringPiece& data);

  // Returns true once an encrypter has been set for the connection.
  virtual bool encryption_established() const = 0;

  // Returns true once the crypto handshake has completed.
  virtual bool handshake_confirmed() const = 0;

  // Returns the parameters negotiated in the crypto handshake.
  virtual const QuicCryptoNegotiatedParameters& crypto_negotiated_params()
      const = 0;

  // Provides the message parser to use when data is received on this stream.
  virtual CryptoMessageParser* crypto_message_parser() = 0;

  // Called to cancel retransmission of unencrypted crypto stream data.
  void NeuterUnencryptedStreamData();

  // Override to record the encryption level of consumed data.
  void OnStreamDataConsumed(size_t bytes_consumed) override;

  // Override to retransmit lost crypto data with the appropriate encryption
  // level.
  void WritePendingRetransmission() override;

 private:
  // Consumed data according to encryption levels.
  // TODO(fayang): This is not needed once switching from QUIC crypto to
  // TLS 1.3, which never encrypts crypto data.
  QuicIntervalSet<QuicStreamOffset> bytes_consumed_[NUM_ENCRYPTION_LEVELS];

  DISALLOW_COPY_AND_ASSIGN(QuicCryptoStream);
};

}  // namespace net

#endif  // NET_QUIC_CORE_QUIC_CRYPTO_STREAM_H_
