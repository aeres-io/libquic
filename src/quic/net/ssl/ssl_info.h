// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SSL_SSL_INFO_H_
#define NET_SSL_SSL_INFO_H_

#include <stdint.h>

#include <vector>

#include "base/memory/ref_counted.h"
#include "net/base/net_export.h"
#include "net/ssl/ssl_config.h"

namespace net {

namespace ct {

enum class CTPolicyCompliance;

}  // namespace ct


// SSL connection info.
// This is really a struct.  All members are public.
class NET_EXPORT SSLInfo {
 public:
  // HandshakeType enumerates the possible resumption cases after an SSL
  // handshake.
  enum HandshakeType {
    HANDSHAKE_UNKNOWN = 0,
    HANDSHAKE_RESUME,  // we resumed a previous session.
    HANDSHAKE_FULL,  // we negotiated a new session.
  };

  SSLInfo();
  SSLInfo(const SSLInfo& info);
  ~SSLInfo();
  SSLInfo& operator=(const SSLInfo& info);

  void Reset();

  
  // Adds the specified |error| to the cert status.
  void SetCertError(int error);

  

 

  // The security strength, in bits, of the SSL cipher suite.
  // 0 means the connection is not encrypted.
  // -1 means the security strength is unknown.
  int security_bits;

  // The ID of the (EC)DH group used by the key exchange or zero if unknown
  // (older cache entries may not store the value) or not applicable.
  uint16_t key_exchange_group;

  // Information about the SSL connection itself. See
  // ssl_connection_status_flags.h for values. The protocol version,
  // ciphersuite, and compression in use are encoded within.
  int connection_status;

  // If the certificate is valid, then this is true iff it was rooted at a
  // standard CA root. (As opposed to a user-installed root.)
  bool is_issued_by_known_root;

  // True if pinning was bypassed on this connection.
  bool pkp_bypassed;

  // True if a client certificate was sent to the server.  Note that sending
  // a Certificate message with no client certificate in it does not count.
  bool client_cert_sent;

  // True if a channel ID was sent to the server.
  bool channel_id_sent;

  // True if Token Binding was negotiated with the server and we agreed on a
  // version and key params.
  bool token_binding_negotiated;

  // Only valid if |token_binding_negotiated| is true. Contains the key param
  // negotiated by the client and server in the Token Binding Negotiation TLS
  // extension.
  TokenBindingParam token_binding_key_param;

  HandshakeType handshake_type;

  // TransportSecurityState::PKPState::CheckPublicKeyPins in the event of a
  // pinning failure. It is a (somewhat) human-readable string.
  std::string pinning_failure_log;

  
  // Whether the connection complied with the CT cert policy, and if
  // not, why not.
  ct::CTPolicyCompliance ct_policy_compliance;

  // True if the connection was required to comply with the CT cert policy. Only
  // meaningful if |ct_policy_compliance| is not
  // COMPLIANCE_DETAILS_NOT_AVAILABLE.
  bool ct_policy_compliance_required;

  
  // True if there was a certificate error which should be treated as fatal,
  // and false otherwise.
  bool is_fatal_cert_error;
};

}  // namespace net

#endif  // NET_SSL_SSL_INFO_H_
