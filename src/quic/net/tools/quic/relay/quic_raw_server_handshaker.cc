// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/relay/quic_raw_server_handshaker.h"

#include <memory>

#include "net/quic/platform/api/quic_arraysize.h"
#include "net/quic/platform/api/quic_text_utils.h"
#include "third_party/boringssl/src/include/openssl/sha.h"
#include "net/quic/core/crypto/null_decrypter.h"
#include "net/quic/core/crypto/null_encrypter.h"

using std::string;

namespace net {

class QuicRawServerHandshaker::ProcessClientHelloCallback
    : public ProcessClientHelloResultCallback {
 public:
  ProcessClientHelloCallback(
      QuicRawServerHandshaker* parent,
      const QuicReferenceCountedPointer<
          ValidateClientHelloResultCallback::Result>& result)
      : parent_(parent), result_(result) {}

  void Run(QuicErrorCode error,
           const string& error_details,
           std::unique_ptr<CryptoHandshakeMessage> message,
           std::unique_ptr<DiversificationNonce> diversification_nonce,
           std::unique_ptr<net::ProofSource::Details> proof_source_details)
      override {
    if (parent_ == nullptr) {
      return;
    }

    parent_->FinishProcessingHandshakeMessageAfterProcessClientHello(
        *result_, error, error_details, std::move(message),
        std::move(diversification_nonce), std::move(proof_source_details));
  }

  void Cancel() { parent_ = nullptr; }

 private:
  QuicRawServerHandshaker* parent_;
  QuicReferenceCountedPointer<ValidateClientHelloResultCallback::Result>
      result_;
};

QuicRawServerHandshaker::QuicRawServerHandshaker(
    const QuicCryptoServerConfig* crypto_config,
    QuicCryptoServerStream* stream,
    QuicCompressedCertsCache* compressed_certs_cache,
    bool use_stateless_rejects_if_peer_supported,
    QuicSession* session,
    QuicCryptoServerStream::Helper* helper)
    : QuicCryptoHandshaker(stream, session),
      stream_(stream),
      session_(session),
      crypto_config_(crypto_config),
      compressed_certs_cache_(compressed_certs_cache),
      signed_config_(new QuicSignedServerConfig),
      helper_(helper),
      num_handshake_messages_(0),
      num_handshake_messages_with_server_nonces_(0),
      send_server_config_update_cb_(nullptr),
      num_server_config_update_messages_sent_(0),
      use_stateless_rejects_if_peer_supported_(
          use_stateless_rejects_if_peer_supported),
      peer_supports_stateless_rejects_(false),
      zero_rtt_attempted_(false),
      chlo_packet_size_(0),
      validate_client_hello_cb_(nullptr),
      process_client_hello_cb_(nullptr),
      encryption_established_(false),
      handshake_confirmed_(false),
      crypto_negotiated_params_(new QuicCryptoNegotiatedParameters) {}

QuicRawServerHandshaker::~QuicRawServerHandshaker() {
  CancelOutstandingCallbacks();
}

void QuicRawServerHandshaker::CancelOutstandingCallbacks() {
  // Detach from the validation callback.  Calling this multiple times is safe.
  if (validate_client_hello_cb_ != nullptr) {
    validate_client_hello_cb_->Cancel();
    validate_client_hello_cb_ = nullptr;
  }
  if (send_server_config_update_cb_ != nullptr) {
    send_server_config_update_cb_->Cancel();
    send_server_config_update_cb_ = nullptr;
  }
  if (process_client_hello_cb_ != nullptr) {
    process_client_hello_cb_->Cancel();
    process_client_hello_cb_ = nullptr;
  }
}

void QuicRawServerHandshaker::OnHandshakeMessage(
    const CryptoHandshakeMessage& message) {
  QuicCryptoHandshaker::OnHandshakeMessage(message);
  ++num_handshake_messages_;
  chlo_packet_size_ = session()->connection()->GetCurrentPacket().length();

  // Do not process handshake messages after the handshake is confirmed.
  if (handshake_confirmed_) {
    stream_->CloseConnectionWithDetails(
        QUIC_CRYPTO_MESSAGE_AFTER_HANDSHAKE_COMPLETE,
        "Unexpected handshake message from client");
    return;
  }

  if (message.tag() != kCHLO) {
    stream_->CloseConnectionWithDetails(QUIC_INVALID_CRYPTO_MESSAGE_TYPE,
                                        "Handshake packet not CHLO");
    return;
  }

  if (validate_client_hello_cb_ != nullptr ||
      process_client_hello_cb_ != nullptr) {
    // Already processing some other handshake message.  The protocol
    // does not allow for clients to send multiple handshake messages
    // before the server has a chance to respond.
    stream_->CloseConnectionWithDetails(
        QUIC_CRYPTO_MESSAGE_WHILE_VALIDATING_CLIENT_HELLO,
        "Unexpected handshake message while processing CHLO");
    return;
  }

  CryptoUtils::HashHandshakeMessage(message, &chlo_hash_,
                                    Perspective::IS_SERVER);

  std::unique_ptr<ValidateCallback> cb(new ValidateCallback(this));
  validate_client_hello_cb_ = cb.get();
  crypto_config_->ValidateClientHello(
      message, GetClientAddress().address(),
      session()->connection()->self_address(), transport_version(),
      session()->connection()->clock(), signed_config_, std::move(cb));
}

void QuicRawServerHandshaker::FinishProcessingHandshakeMessage(
    QuicReferenceCountedPointer<ValidateClientHelloResultCallback::Result>
        result,
    std::unique_ptr<ProofSource::Details> details) {
  const CryptoHandshakeMessage& message = result->client_hello;

  // Clear the callback that got us here.
  validate_client_hello_cb_ = nullptr;

  if (use_stateless_rejects_if_peer_supported_) {
    peer_supports_stateless_rejects_ =
        QuicCryptoServerStreamBase::DoesPeerSupportStatelessRejects(message);
  }

  std::unique_ptr<ProcessClientHelloCallback> cb(
      new ProcessClientHelloCallback(this, result));
  process_client_hello_cb_ = cb.get();
  ProcessClientHello(result, std::move(details), std::move(cb));
}

void QuicRawServerHandshaker::
    FinishProcessingHandshakeMessageAfterProcessClientHello(
        const ValidateClientHelloResultCallback::Result& result,
        QuicErrorCode error,
        const string& error_details,
        std::unique_ptr<CryptoHandshakeMessage> reply,
        std::unique_ptr<DiversificationNonce> diversification_nonce,
        std::unique_ptr<ProofSource::Details> proof_source_details) {
  // Clear the callback that got us here.
  process_client_hello_cb_ = nullptr;

  const CryptoHandshakeMessage& message = result.client_hello;
  if (error != QUIC_NO_ERROR) {
    stream_->CloseConnectionWithDetails(error, error_details);
    return;
  }

  if (reply->tag() != kSHLO) {
    if (reply->tag() == kSREJ) {
      // Before sending the SREJ, cause the connection to save crypto packets
      // so that they can be added to the time wait list manager and
      // retransmitted.
      session()->connection()->EnableSavingCryptoPackets();
    }
    SendHandshakeMessage(*reply);

    if (reply->tag() == kSREJ) {
      session()->connection()->CloseConnection(
          QUIC_CRYPTO_HANDSHAKE_STATELESS_REJECT, "stateless reject",
          ConnectionCloseBehavior::SILENT_CLOSE);
    }
    return;
  }

  // If we are returning a SHLO then we accepted the handshake.  Now
  // process the negotiated configuration options as part of the
  // session config.
  QuicConfig* config = session()->config();
  OverrideQuicConfigDefaults(config);
  string process_error_details;
  const QuicErrorCode process_error =
      config->ProcessPeerHello(message, CLIENT, &process_error_details);
  if (process_error != QUIC_NO_ERROR) {
    stream_->CloseConnectionWithDetails(process_error, process_error_details);
    return;
  }

  session()->OnConfigNegotiated();

  config->ToHandshakeMessage(reply.get());

  // Receiving a full CHLO implies the client is prepared to decrypt with
  // the new server write key.  We can start to encrypt with the new server
  // write key.
  //
  // NOTE: the SHLO will be encrypted with the new server write key.
#ifdef USE_QUIC_ENCRYPTION
  session()->connection()->SetEncrypter(
      ENCRYPTION_INITIAL,
      crypto_negotiated_params_->initial_crypters.encrypter.release());
  session()->connection()->SetDefaultEncryptionLevel(ENCRYPTION_INITIAL);
  // Set the decrypter immediately so that we no longer accept unencrypted
  // packets.
  session()->connection()->SetDecrypter(
      ENCRYPTION_INITIAL,
      crypto_negotiated_params_->initial_crypters.decrypter.release());
#else
  session()->connection()->SetEncrypter(
      ENCRYPTION_INITIAL,
      new NullEncrypter(Perspective::IS_SERVER));
  session()->connection()->SetDefaultEncryptionLevel(ENCRYPTION_INITIAL);
  // Set the decrypter immediately so that we no longer accept unencrypted
  // packets.
  session()->connection()->SetDecrypter(
      ENCRYPTION_INITIAL,
      new NullDecrypter(Perspective::IS_SERVER));
#endif

  session()->connection()->SetDiversificationNonce(*diversification_nonce);

  SendHandshakeMessage(*reply);

#ifdef USE_QUIC_ENCRYPTION
  session()->connection()->SetEncrypter(
      ENCRYPTION_FORWARD_SECURE,
      crypto_negotiated_params_->forward_secure_crypters.encrypter.release());
  session()->connection()->SetDefaultEncryptionLevel(ENCRYPTION_FORWARD_SECURE);

  session()->connection()->SetAlternativeDecrypter(
      ENCRYPTION_FORWARD_SECURE,
      crypto_negotiated_params_->forward_secure_crypters.decrypter.release(),
      false /* don't latch */);
#else
  session()->connection()->SetEncrypter(
      ENCRYPTION_FORWARD_SECURE,
      new NullEncrypter(Perspective::IS_SERVER));
  session()->connection()->SetDefaultEncryptionLevel(ENCRYPTION_FORWARD_SECURE);

  session()->connection()->SetAlternativeDecrypter(
      ENCRYPTION_FORWARD_SECURE,
      new NullDecrypter(Perspective::IS_SERVER),
      false /* don't latch */);
#endif

  encryption_established_ = true;
  handshake_confirmed_ = true;
  session()->OnCryptoHandshakeEvent(QuicSession::HANDSHAKE_CONFIRMED);
}

void QuicRawServerHandshaker::SendServerConfigUpdate(
    const CachedNetworkParameters* cached_network_params) {
  if (!handshake_confirmed_) {
    return;
  }

  if (send_server_config_update_cb_ != nullptr) {
    return;
  }

  std::unique_ptr<SendServerConfigUpdateCallback> cb(
      new SendServerConfigUpdateCallback(this));
  send_server_config_update_cb_ = cb.get();

  crypto_config_->BuildServerConfigUpdateMessage(
      session()->connection()->transport_version(), chlo_hash_,
      previous_source_address_tokens_, session()->connection()->self_address(),
      GetClientAddress().address(), session()->connection()->clock(),
      session()->connection()->random_generator(), compressed_certs_cache_,
      *crypto_negotiated_params_, cached_network_params,
      std::move(cb));
}

QuicRawServerHandshaker::SendServerConfigUpdateCallback::
    SendServerConfigUpdateCallback(QuicRawServerHandshaker* parent)
    : parent_(parent) {}

void QuicRawServerHandshaker::SendServerConfigUpdateCallback::Cancel() {
  parent_ = nullptr;
}

// From BuildServerConfigUpdateMessageResultCallback
void QuicRawServerHandshaker::SendServerConfigUpdateCallback::Run(
    bool ok,
    const CryptoHandshakeMessage& message) {
  if (parent_ == nullptr) {
    return;
  }
  parent_->FinishSendServerConfigUpdate(ok, message);
}

void QuicRawServerHandshaker::FinishSendServerConfigUpdate(
    bool ok,
    const CryptoHandshakeMessage& message) {
  // Clear the callback that got us here.
  send_server_config_update_cb_ = nullptr;

  if (!ok) {
    return;
  }

  const QuicData& data = message.GetSerialized(Perspective::IS_SERVER);
  stream_->WriteOrBufferData(base::StringPiece(data.data(), data.length()), false,
                             nullptr);

  ++num_server_config_update_messages_sent_;
}

uint8_t QuicRawServerHandshaker::NumHandshakeMessages() const {
  return num_handshake_messages_;
}

uint8_t QuicRawServerHandshaker::NumHandshakeMessagesWithServerNonces()
    const {
  return num_handshake_messages_with_server_nonces_;
}

int QuicRawServerHandshaker::NumServerConfigUpdateMessagesSent() const {
  return num_server_config_update_messages_sent_;
}

const CachedNetworkParameters*
QuicRawServerHandshaker::PreviousCachedNetworkParams() const {
  return previous_cached_network_params_.get();
}

bool QuicRawServerHandshaker::UseStatelessRejectsIfPeerSupported() const {
  return use_stateless_rejects_if_peer_supported_;
}

bool QuicRawServerHandshaker::PeerSupportsStatelessRejects() const {
  return peer_supports_stateless_rejects_;
}

bool QuicRawServerHandshaker::ZeroRttAttempted() const {
  return zero_rtt_attempted_;
}

void QuicRawServerHandshaker::SetPeerSupportsStatelessRejects(
    bool peer_supports_stateless_rejects) {
  peer_supports_stateless_rejects_ = peer_supports_stateless_rejects;
}

void QuicRawServerHandshaker::SetPreviousCachedNetworkParams(
    CachedNetworkParameters cached_network_params) {
  previous_cached_network_params_.reset(
      new CachedNetworkParameters(cached_network_params));
}

bool QuicRawServerHandshaker::ShouldSendExpectCTHeader() const {
  return signed_config_->proof.send_expect_ct_header;
}

bool QuicRawServerHandshaker::GetBase64SHA256ClientChannelID(
    string* output) const {
  if (!encryption_established() ||
      crypto_negotiated_params_->channel_id.empty()) {
    return false;
  }

  const string& channel_id(crypto_negotiated_params_->channel_id);
  uint8_t digest[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const uint8_t*>(channel_id.data()), channel_id.size(),
         digest);

  QuicTextUtils::Base64Encode(digest, QUIC_ARRAYSIZE(digest), output);
  return true;
}

bool QuicRawServerHandshaker::encryption_established() const {
  return encryption_established_;
}

bool QuicRawServerHandshaker::handshake_confirmed() const {
  return handshake_confirmed_;
}

const QuicCryptoNegotiatedParameters&
QuicRawServerHandshaker::crypto_negotiated_params() const {
  return *crypto_negotiated_params_;
}

CryptoMessageParser* QuicRawServerHandshaker::crypto_message_parser() {
  return QuicCryptoHandshaker::crypto_message_parser();
}

void QuicRawServerHandshaker::ProcessClientHello(
    QuicReferenceCountedPointer<ValidateClientHelloResultCallback::Result>
        result,
    std::unique_ptr<ProofSource::Details> proof_source_details,
    std::unique_ptr<ProcessClientHelloResultCallback> done_cb) {
  const CryptoHandshakeMessage& message = result->client_hello;
  string error_details;
  if (!helper_->CanAcceptClientHello(
          message, session()->connection()->self_address(), &error_details)) {
    done_cb->Run(QUIC_HANDSHAKE_FAILED, error_details, nullptr, nullptr,
                 nullptr);
    return;
  }
  if (!result->info.server_nonce.empty()) {
    ++num_handshake_messages_with_server_nonces_;
  }

  if (num_handshake_messages_ == 1) {
    // Client attempts zero RTT handshake by sending a non-inchoate CHLO.
    base::StringPiece public_value;
    zero_rtt_attempted_ = message.GetStringPiece(kPUBS, &public_value);
  }

  // Store the bandwidth estimate from the client.
  if (result->cached_network_params.bandwidth_estimate_bytes_per_second() > 0) {
    previous_cached_network_params_.reset(
        new CachedNetworkParameters(result->cached_network_params));
  }
  previous_source_address_tokens_ = result->info.source_address_tokens;

  const bool use_stateless_rejects_in_crypto_config =
      use_stateless_rejects_if_peer_supported_ &&
      peer_supports_stateless_rejects_;
  QuicConnection* connection = session()->connection();
  const QuicConnectionId server_designated_connection_id =
      GenerateConnectionIdForReject(use_stateless_rejects_in_crypto_config);
  crypto_config_->ProcessClientHello(
      result, /*reject_only=*/false, connection->connection_id(),
      connection->self_address(), GetClientAddress(), transport_version(),
      ParsedVersionsToTransportVersions(connection->supported_versions()),
      use_stateless_rejects_in_crypto_config, server_designated_connection_id,
      connection->clock(), connection->random_generator(),
      compressed_certs_cache_, crypto_negotiated_params_, signed_config_,
      QuicCryptoStream::CryptoMessageFramingOverhead(transport_version()),
      chlo_packet_size_, std::move(done_cb));
}

void QuicRawServerHandshaker::OverrideQuicConfigDefaults(
    QuicConfig* config) {}

QuicRawServerHandshaker::ValidateCallback::ValidateCallback(
    QuicRawServerHandshaker* parent)
    : parent_(parent) {}

void QuicRawServerHandshaker::ValidateCallback::Cancel() {
  parent_ = nullptr;
}

void QuicRawServerHandshaker::ValidateCallback::Run(
    QuicReferenceCountedPointer<Result> result,
    std::unique_ptr<ProofSource::Details> details) {
  if (parent_ != nullptr) {
    parent_->FinishProcessingHandshakeMessage(std::move(result),
                                              std::move(details));
  }
}

QuicConnectionId QuicRawServerHandshaker::GenerateConnectionIdForReject(
    bool use_stateless_rejects) {
  if (!use_stateless_rejects) {
    return 0;
  }
  return helper_->GenerateConnectionIdForReject(
      session()->connection()->connection_id());
}

const IPEndPoint QuicRawServerHandshaker::GetClientAddress() {
  return session()->connection()->peer_address();
}

}  // namespace net
