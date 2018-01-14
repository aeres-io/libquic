// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/core/crypto/null_decrypter.h"

#include <cstdint>

#include "net/base/int128.h"
#include "net/quic/core/quic_data_reader.h"
#include "net/quic/core/quic_utils.h"

using std::string;

namespace net {

NullDecrypter::NullDecrypter(Perspective perspective)
    : perspective_(perspective) {}

bool NullDecrypter::SetKey(base::StringPiece key) {
  return key.empty();
}

bool NullDecrypter::SetNoncePrefix(base::StringPiece nonce_prefix) {
  return nonce_prefix.empty();
}

bool NullDecrypter::SetIV(base::StringPiece iv) {
  return iv.empty();
}

bool NullDecrypter::SetPreliminaryKey(base::StringPiece key) {
  return false;
}

bool NullDecrypter::SetDiversificationNonce(const DiversificationNonce& nonce) {
  return true;
}

bool NullDecrypter::DecryptPacket(QuicTransportVersion version,
                                  QuicPacketNumber /*packet_number*/,
                                  base::StringPiece associated_data,
                                  base::StringPiece ciphertext,
                                  char* output,
                                  size_t* output_length,
                                  size_t max_output_length) {
  QuicDataReader reader(ciphertext.data(), ciphertext.length(),
                        HOST_BYTE_ORDER);
  uint128 hash;

  if (!ReadHash(&reader, &hash)) {
    return false;
  }

  base::StringPiece plaintext = reader.ReadRemainingPayload();
  if (plaintext.length() > max_output_length) {
    return false;
  }
  if (hash != ComputeHash(version, associated_data, plaintext)) {
    return false;
  }
  // Copy the plaintext to output.
  memcpy(output, plaintext.data(), plaintext.length());
  *output_length = plaintext.length();
  return true;
}

size_t NullDecrypter::GetKeySize() const {
  return 0;
}

size_t NullDecrypter::GetIVSize() const {
  return 0;
}

base::StringPiece NullDecrypter::GetKey() const {
  return base::StringPiece();
}

base::StringPiece NullDecrypter::GetNoncePrefix() const {
  return base::StringPiece();
}

uint32_t NullDecrypter::cipher_id() const {
  return 0;
}

bool NullDecrypter::ReadHash(QuicDataReader* reader, uint128* hash) {
  uint64_t lo;
  uint32_t hi;
  if (!reader->ReadUInt64(&lo) || !reader->ReadUInt32(&hi)) {
    return false;
  }
  *hash = MakeUint128(hi, lo);
  return true;
}

uint128 NullDecrypter::ComputeHash(QuicTransportVersion version,
                                   const base::StringPiece data1,
                                   const base::StringPiece data2) const {
  uint128 correct_hash;
  if (version > QUIC_VERSION_35) {
    if (perspective_ == Perspective::IS_CLIENT) {
      // Peer is a server.
      correct_hash = QuicUtils::FNV1a_128_Hash_Three(data1, data2, "Server");

    } else {
      // Peer is a client.
      correct_hash = QuicUtils::FNV1a_128_Hash_Three(data1, data2, "Client");
    }
  } else {
    correct_hash = QuicUtils::FNV1a_128_Hash_Two(data1, data2);
  }
  uint128 mask = MakeUint128(UINT64_C(0x0), UINT64_C(0xffffffff));
  mask <<= 96;
  correct_hash &= ~mask;
  return correct_hash;
}

}  // namespace net
