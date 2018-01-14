// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/relay/quic_raw_server.h"

#include <string.h>

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/quic/core/crypto/crypto_handshake.h"
#include "net/quic/core/crypto/quic_random.h"
#include "net/quic/core/quic_crypto_stream.h"
#include "net/quic/core/quic_data_reader.h"
#include "net/quic/core/quic_packets.h"
#include "net/quic/core/tls_server_handshaker.h"
#include "net/socket/udp_server_socket.h"
#include "net/tools/quic/quic_simple_per_connection_packet_writer.h"
#include "net/tools/quic/quic_simple_server_packet_writer.h"
#include "net/tools/quic/quic_simple_server_session_helper.h"

#include "net/tools/quic/relay/quic_raw_dispatcher.h"


namespace net {

namespace {

const char kSourceAddressTokenSecret[] = "secret";
const size_t kNumSessionsToCreatePerSocketEvent = 16;

// Allocate some extra space so we can send an error if the client goes over
// the limit.
const int kReadBufferSize = 1024 * kMaxPacketSize;

}  // namespace

QuicRawServer::QuicRawServer(
    std::unique_ptr<ProofSource> proof_source,
    const QuicConfig& config,
    const QuicCryptoServerConfig::ConfigOptions& crypto_config_options,
    const ParsedQuicVersionVector& supported_versions,
    const std::string & unique_server_id)
    : version_manager_(supported_versions),
      helper_(
          new QuicConnectionHelper(&clock_, QuicRandom::GetInstance())),
      alarm_factory_(new QuicChromiumAlarmFactory(
          base::ThreadTaskRunnerHandle::Get().get(),
          &clock_)),
      config_(config),
      crypto_config_options_(crypto_config_options),
      crypto_config_(kSourceAddressTokenSecret,
                     QuicRandom::GetInstance(),
                     std::move(proof_source),
										 TlsServerHandshaker::CreateSslCtx()),
      read_pending_(false),
      synchronous_read_count_(0),
      read_buffer_(new IOBufferWithSize(kReadBufferSize)),
      weak_factory_(this),
      unique_server_id_(unique_server_id) {
  Initialize();
}

void QuicRawServer::Initialize() {
#if MMSG_MORE
  use_recvmmsg_ = true;
#endif

  // If an initial flow control window has not explicitly been set, then use a
  // sensible value for a server: 1 MB for session, 64 KB for each stream.
  const uint32_t kInitialSessionFlowControlWindow = 1 * 1024 * 1024;  // 1 MB
  const uint32_t kInitialStreamFlowControlWindow = 64 * 1024;         // 64 KB
  if (config_.GetInitialStreamFlowControlWindowToSend() ==
      kMinimumFlowControlSendWindow) {
    config_.SetInitialStreamFlowControlWindowToSend(
        kInitialStreamFlowControlWindow);
  }
  if (config_.GetInitialSessionFlowControlWindowToSend() ==
      kMinimumFlowControlSendWindow) {
    config_.SetInitialSessionFlowControlWindowToSend(
        kInitialSessionFlowControlWindow);
  }

  std::unique_ptr<CryptoHandshakeMessage> scfg(crypto_config_.AddDefaultConfig(
      helper_->GetRandomGenerator(), helper_->GetClock(),
      crypto_config_options_));
}

QuicRawServer::~QuicRawServer()
{
}


QuicRawDispatcher::DispatcherDelegate * QuicRawServer::GetDispatcherDelegate() {
  if (dispatcher_) {
    return dispatcher_->GetDispatcherDelegate();
  }
  else {
    return dispatcher_delegate_.get();
  }
}


void QuicRawServer::ResetDispatcherDelegate(QuicRawDispatcher::DispatcherDelegate * val) {
  if (dispatcher_) {
    dispatcher_->ResetDispatcherDelegate(val);
  }
  else {
    dispatcher_delegate_.reset(val);
  }
}

int QuicRawServer::Listen(const IPEndPoint& address) {
  std::unique_ptr<UDPServerSocket> socket(
      new UDPServerSocket());

  socket->AllowAddressReuse();

  int rc = socket->Listen(address);
  if (rc < 0) {
    return rc;
  }

  // These send and receive buffer sizes are sized for a single connection,
  // because the default usage of QuicRawServer is as a test server with
  // one or two clients.  Adjust higher for use with many clients.
  rc = socket->SetReceiveBufferSize(
      static_cast<int32_t>(8 * kDefaultSocketReceiveBuffer)); // 8M
  if (rc < 0) {
    return rc;
  }

  rc = socket->SetSendBufferSize(1024 * kMaxPacketSize);
  if (rc < 0) {
    return rc;
  }

  rc = socket->GetLocalAddress(&server_address_);
  if (rc < 0) {
    return rc;
  }


  socket_.swap(socket);

  dispatcher_.reset(new QuicRawDispatcher(
      config_, &crypto_config_, &version_manager_,
      std::unique_ptr<QuicConnectionHelper>(helper_),
      std::unique_ptr<QuicCryptoServerStream::Helper>(
        /* since we want to able to handle multiple connections, Quartc session helper
         * always return 0 for connection id reject;
         * however simple session helper return a random connection id */
          new QuicSimpleServerSessionHelper(QuicRandom::GetInstance())),
      std::unique_ptr<QuicAlarmFactory>(alarm_factory_),
      unique_server_id_));

  if (dispatcher_delegate_) {
    dispatcher_->ResetDispatcherDelegate(dispatcher_delegate_.release());
  }

  QuicSimpleServerPacketWriter* writer =
      new QuicSimpleServerPacketWriter(socket_.get(), dispatcher_.get());
  dispatcher_->InitializeWithWriter(writer);

  StartReading();

  return OK;
}

void QuicRawServer::Shutdown() {
  // Before we shut down the epoll server, give all active sessions a chance to
  // notify clients that they're closing.
  dispatcher_->Shutdown();

  socket_->Close();
  socket_.reset();
}

void QuicRawServer::StartReading() {
  if (synchronous_read_count_ == 0) {
    // Only process buffered packets once per message loop.
    dispatcher_->ProcessBufferedChlos(kNumSessionsToCreatePerSocketEvent);
  }

  if (read_pending_) {
    return;
  }
  read_pending_ = true;

  int result = socket_->RecvFrom(
      read_buffer_.get(), read_buffer_->size(), &client_address_,
      base::Bind(&QuicRawServer::OnReadComplete, base::Unretained(this)));

  if (result == ERR_IO_PENDING) {
    synchronous_read_count_ = 0;
    if (dispatcher_->HasChlosBuffered()) {
      // No more packets to read, so yield before processing buffered packets.
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          base::Bind(&QuicRawServer::StartReading,
                     weak_factory_.GetWeakPtr()));
    }
    return;
  }

  if (++synchronous_read_count_ > 32) {
    synchronous_read_count_ = 0;
    // Schedule the processing through the message loop to 1) prevent infinite
    // recursion and 2) avoid blocking the thread for too long.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        base::Bind(&QuicRawServer::OnReadComplete,
                   weak_factory_.GetWeakPtr(), result));
  } else {
    OnReadComplete(result);
  }
}

void QuicRawServer::OnReadComplete(int result) {
  read_pending_ = false;
  if (result == 0)
    result = ERR_CONNECTION_CLOSED;

  if (result < 0) {
    Shutdown();
    return;
  }

  QuicReceivedPacket packet(read_buffer_->data(), result,
                            helper_->GetClock()->Now(), false);
  dispatcher_->ProcessPacket(
      IPEndPoint(server_address_),
      IPEndPoint(client_address_), packet);

  StartReading();
}

}  // namespace net
