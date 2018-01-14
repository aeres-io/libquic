// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/core/crypto/aead_base_decrypter.h"

#include <cstdint>

#include "net/quic/core/quic_utils.h"
#include "net/quic/platform/api/quic_arraysize.h"
#include "third_party/boringssl/src/include/openssl/err.h"
#include "third_party/boringssl/src/include/openssl/evp.h"

using std::string;

namespace net {

namespace {

// Clear OpenSSL error stack.
void ClearOpenSslErrors() {
  while (ERR_get_error()) {
  }
}

// In debug builds only, log OpenSSL error stack. Then clear OpenSSL error
// stack.
void DLogOpenSslErrors() {
#ifdef NDEBUG
  ClearOpenSslErrors();
#else
  while (uint32_t error = ERR_get_error()) {
    char buf[120];
    ERR_error_string_n(error, buf, QUIC_ARRAYSIZE(buf));
  }
#endif
}

}  // namespace

AeadBaseDecrypter::AeadBaseDecrypter(const EVP_AEAD* aead_alg,
                                     size_t key_size,
                                     size_t auth_tag_size,
                                     size_t nonce_size,
                                     bool use_ietf_nonce_construction)
    : aead_alg_(aead_alg),
      key_size_(key_size),
      auth_tag_size_(auth_tag_size),
      nonce_size_(nonce_size),
      use_ietf_nonce_construction_(use_ietf_nonce_construction),
      have_preliminary_key_(false) {
}

AeadBaseDecrypter::~AeadBaseDecrypter() {}

bool AeadBaseDecrypter::SetKey(base::StringPiece key) {
  if (key.size() != key_size_) {
    return false;
  }
  memcpy(key_, key.data(), key.size());

  EVP_AEAD_CTX_cleanup(ctx_.get());
  if (!EVP_AEAD_CTX_init(ctx_.get(), aead_alg_, key_, key_size_, auth_tag_size_,
                         nullptr)) {
    DLogOpenSslErrors();
    return false;
  }

  return true;
}

bool AeadBaseDecrypter::SetNoncePrefix(base::StringPiece nonce_prefix) {
  if (use_ietf_nonce_construction_) {
    return false;
  }
  if (nonce_prefix.size() != nonce_size_ - sizeof(QuicPacketNumber)) {
    return false;
  }
  memcpy(iv_, nonce_prefix.data(), nonce_prefix.size());
  return true;
}

bool AeadBaseDecrypter::SetIV(base::StringPiece iv) {
  if (!use_ietf_nonce_construction_) {
    return false;
  }
  if (iv.size() != nonce_size_) {
    return false;
  }
  memcpy(iv_, iv.data(), iv.size());
  return true;
}

bool AeadBaseDecrypter::SetPreliminaryKey(base::StringPiece key) {
  SetKey(key);
  have_preliminary_key_ = true;

  return true;
}

bool AeadBaseDecrypter::SetDiversificationNonce(
    const DiversificationNonce& nonce) {
  if (!have_preliminary_key_) {
    return true;
  }

  string key, nonce_prefix;
  size_t prefix_size = nonce_size_ - sizeof(QuicPacketNumber);
  DiversifyPreliminaryKey(
      base::StringPiece(reinterpret_cast<const char*>(key_), key_size_),
      base::StringPiece(reinterpret_cast<const char*>(iv_), prefix_size), nonce,
      key_size_, prefix_size, &key, &nonce_prefix);

  if (!SetKey(key) || !SetNoncePrefix(nonce_prefix)) {
    return false;
  }

  have_preliminary_key_ = false;
  return true;
}

bool AeadBaseDecrypter::DecryptPacket(QuicTransportVersion /*version*/,
                                      QuicPacketNumber packet_number,
                                      base::StringPiece associated_data,
                                      base::StringPiece ciphertext,
                                      char* output,
                                      size_t* output_length,
                                      size_t max_output_length) {
  if (ciphertext.length() < auth_tag_size_) {
    return false;
  }

  if (have_preliminary_key_) {
    return false;
  }

  uint8_t nonce[kMaxNonceSize];
  memcpy(nonce, iv_, nonce_size_);
  size_t prefix_len = nonce_size_ - sizeof(packet_number);
  if (use_ietf_nonce_construction_) {
    for (size_t i = 0; i < sizeof(packet_number); ++i) {
      nonce[prefix_len + i] ^=
          (packet_number >> ((sizeof(packet_number) - i - 1) * 8)) & 0xff;
    }
  } else {
    memcpy(nonce + prefix_len, &packet_number, sizeof(packet_number));
  }
  if (!EVP_AEAD_CTX_open(
          ctx_.get(), reinterpret_cast<uint8_t*>(output), output_length,
          max_output_length, reinterpret_cast<const uint8_t*>(nonce),
          nonce_size_, reinterpret_cast<const uint8_t*>(ciphertext.data()),
          ciphertext.size(),
          reinterpret_cast<const uint8_t*>(associated_data.data()),
          associated_data.size())) {
    // Because QuicFramer does trial decryption, decryption errors are expected
    // when encryption level changes. So we don't log decryption errors.
    ClearOpenSslErrors();
    return false;
  }
  return true;
}

size_t AeadBaseDecrypter::GetKeySize() const {
  return key_size_;
}

size_t AeadBaseDecrypter::GetIVSize() const {
  return nonce_size_;
}

base::StringPiece AeadBaseDecrypter::GetKey() const {
  return base::StringPiece(reinterpret_cast<const char*>(key_), key_size_);
}

base::StringPiece AeadBaseDecrypter::GetNoncePrefix() const {
  return base::StringPiece(reinterpret_cast<const char*>(iv_),
                         nonce_size_ - sizeof(QuicPacketNumber));
}

}  // namespace net
