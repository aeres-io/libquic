// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/ssl/ssl_config.h"


namespace net {

const uint16_t kDefaultSSLVersionMin = SSL_PROTOCOL_VERSION_TLS1;

const uint16_t kDefaultSSLVersionMax = SSL_PROTOCOL_VERSION_TLS1_2;

const TLS13Variant kDefaultTLS13Variant = kTLS13VariantDraft22;

SSLConfig::CertAndStatus::CertAndStatus() = default;
SSLConfig::CertAndStatus::CertAndStatus(const CertAndStatus& other) = default;
SSLConfig::CertAndStatus::~CertAndStatus() = default;

SSLConfig::SSLConfig()
    : rev_checking_enabled(false),
      rev_checking_required_local_anchors(false),
      sha1_local_anchors_enabled(true),
      common_name_fallback_local_anchors_enabled(true),
      symantec_enforcement_disabled(false),
      version_min(kDefaultSSLVersionMin),
      version_max(kDefaultSSLVersionMax),
      tls13_variant(kDefaultTLS13Variant),
      version_interference_probe(false),
      channel_id_enabled(true),
      false_start_enabled(true),
      signed_cert_timestamps_enabled(true),
      require_ecdhe(false),
      send_client_cert(false),
      verify_ev_cert(false),
      cert_io_enabled(true),
      renego_allowed_default(false) {}

SSLConfig::SSLConfig(const SSLConfig& other) = default;

SSLConfig::~SSLConfig() = default;


}  // namespace net
