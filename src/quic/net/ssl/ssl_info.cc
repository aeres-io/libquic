// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/ssl/ssl_info.h"

#include "base/pickle.h"
#include "base/stl_util.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"

namespace net {

SSLInfo::SSLInfo() {
  Reset();
}

SSLInfo::SSLInfo(const SSLInfo& info) {
  *this = info;
}

SSLInfo::~SSLInfo() = default;

SSLInfo& SSLInfo::operator=(const SSLInfo& info) = default;

void SSLInfo::Reset() {
  security_bits = -1;
  key_exchange_group = 0;
  connection_status = 0;
  is_issued_by_known_root = false;
  pkp_bypassed = false;
  client_cert_sent = false;
  channel_id_sent = false;
  token_binding_negotiated = false;
  token_binding_key_param = TB_PARAM_ECDSAP256;
  handshake_type = HANDSHAKE_UNKNOWN;
  base::STLClearObject(&pinning_failure_log);
  ct_policy_compliance_required = false;
  is_fatal_cert_error = false;
}


}  // namespace net
