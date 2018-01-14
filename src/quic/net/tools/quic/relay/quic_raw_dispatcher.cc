// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/relay/quic_raw_dispatcher.h"
#include "net/tools/quic/relay/quic_raw_session.h"

namespace net {

QuicRawDispatcher::QuicRawDispatcher(
    const QuicConfig& config,
    const QuicCryptoServerConfig* crypto_config,
    QuicVersionManager* version_manager,
    std::unique_ptr<QuicConnectionHelper> helper,
    std::unique_ptr<QuicCryptoServerStream::Helper> session_helper,
    std::unique_ptr<QuicAlarmFactory> alarm_factory,
    const std::string & unique_server_id
    )
    : QuicDispatcher(config,
                     crypto_config,
                     version_manager,
                     std::move(helper),
                     std::move(session_helper),
                     std::move(alarm_factory))
    , unique_server_id_(unique_server_id)
      {}

QuicRawDispatcher::~QuicRawDispatcher() {}


QuicRawSession* QuicRawDispatcher::CreateQuicSession(
    QuicConnectionId connection_id,
    const IPEndPoint& client_address,
    base::StringPiece alpn) {
  QuicConnection* connection = new QuicConnection(
      connection_id, client_address, helper(), alarm_factory(),
      CreatePerConnectionWriter(),
      /* owns_writer= */ true, Perspective::IS_SERVER, GetSupportedVersions());

  QuicRawSession* session = new QuicRawSession(
      std::unique_ptr<QuicConnection>(connection),
      config(),
      unique_server_id_,
      Perspective::IS_SERVER,
      helper());

  session->SetQuicSessionVisitor(this);

  session->StartCryptoHandshake();

  if (dispatcher_delegate_) {
    dispatcher_delegate_->OnNewSession(session);
  }

  return session;
}

}  // namespace net
