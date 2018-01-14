// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/relay/quic_raw_client.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/socket/udp_client_socket.h"
#include "net/base/net_errors.h"
#include "net/quic/core/tls_client_handshaker.h"
#include "net/quic/platform/api/quic_text_utils.h"
#include "net/tools/quic/relay/quic_alarm_factory_impl.h"
#include "net/quic/chromium/quic_chromium_packet_writer.h"
#include "net/quic/core/quic_connection.h"
#include "net/tools/quic/relay/quic_raw_connection.h"

using base::StringPiece;
using base::StringToInt;
using std::string;

namespace net {

  namespace {

    class QuicRawClientStreamDelegate : public QuartcStreamInterface::Delegate {
    public:
      void OnReceived(QuartcStreamInterface * stream,
        const char * data, size_t size) override;
      void OnClose(QuartcStreamInterface * stream) override;
      void OnCanWrite(QuartcStreamInterface* stream) override;
    };

    void QuicRawClientStreamDelegate::OnReceived(QuartcStreamInterface * stream,
      const char * data, size_t size)
    {
      std::string o(data, size);
      std::cout << "Got server response: "
        << o << std::endl;
    }

    void QuicRawClientStreamDelegate::OnClose(QuartcStreamInterface * stream)
    {
      std::cout << "Stream closed." << std::endl;
    }

    void QuicRawClientStreamDelegate::OnCanWrite(QuartcStreamInterface *stream)
    {
    }

  }

  void QuicRawClient::ClientQuicDataToResend::Resend() {
    client_->NewStreamAndSendData(body_.data(), body_.size(), fin_);
  }

  QuicRawClient::QuicDataToResend::QuicDataToResend(
    StringPiece body,
    bool fin)
    : body_(body), fin_(fin) {}

  QuicRawClient::QuicDataToResend::~QuicDataToResend() {}


  QuicRawClient::QuicRawClient(
    IPEndPoint server_address,
    const QuicServerId& server_id,
    const net::ParsedQuicVersionVector& supported_versions,
    std::unique_ptr<ProofVerifier> proof_verifier)
    : initialized_(false),
      packet_reader_started_(false),
      weak_factory_(this),
      server_id_(server_id),
      crypto_config_(std::move(proof_verifier),
          TlsClientHandshaker::CreateSslCtx()),
      helper_(CreateQuicConnectionHelper()),
      alarm_factory_(CreateQuicAlarmFactory()),
      initial_max_packet_length_(0),
      num_stateless_rejects_received_(0),
      num_sent_client_hellos_(0),
      supported_versions_(supported_versions),
      connection_error_(QUIC_NO_ERROR),
      connected_or_attempting_connect_(false) {

      set_server_address(server_address);
  }

  QuicRawClient::~QuicRawClient() {
    if (connected()) {
      session()->connection()->CloseConnection(
          QUIC_PEER_GOING_AWAY, "Shutting down",
          ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
    }
  }

  bool QuicRawClient::Initialize() {
    num_sent_client_hellos_ = 0;
    num_stateless_rejects_received_ = 0;
    connection_error_ = QUIC_NO_ERROR;
    connected_or_attempting_connect_ = false;

    stream_delegate_.reset(
      new QuicRawClientStreamDelegate
    );

    // If an initial flow control window has not explicitly been set, then use the
    // same values that Chrome uses.
    const uint32_t kSessionMaxRecvWindowSize = 15 * 1024 * 1024;  // 15 MB
    const uint32_t kStreamMaxRecvWindowSize = 6 * 1024 * 1024;    //  6 MB
    if (config()->GetInitialStreamFlowControlWindowToSend() ==
      kMinimumFlowControlSendWindow) {
      config()->SetInitialStreamFlowControlWindowToSend(kStreamMaxRecvWindowSize);
    }
    if (config()->GetInitialSessionFlowControlWindowToSend() ==
      kMinimumFlowControlSendWindow) {
      config()->SetInitialSessionFlowControlWindowToSend(
        kSessionMaxRecvWindowSize);
    }

    if (!CreateUDPSocketAndBind(server_address_, bind_to_address_, local_port_)) {
      return false;
    }

    initialized_ = true;
    return true;
  }

  bool QuicRawClient::Connect() {
    // Attempt multiple connects until the maximum number of client hellos have
    // been sent.
    while (!connected() &&
      GetNumSentClientHellos() <= QuicCryptoClientStream::kMaxClientHellos) {
      StartConnect();
      while (EncryptionBeingEstablished()) {
        WaitForEvents();
      }
      if (FLAGS_quic_reloadable_flag_enable_quic_stateless_reject_support &&
        connected()) {
        // Resend any previously queued data.
        ResendSavedData();
      }
      if (session() != nullptr &&
        session()->error() != QUIC_CRYPTO_HANDSHAKE_STATELESS_REJECT) {
        // We've successfully created a session but we're not connected, and there
        // is no stateless reject to recover from.  Give up trying.
        break;
      }
    }
    if (!connected() &&
      GetNumSentClientHellos() > QuicCryptoClientStream::kMaxClientHellos &&
      session() != nullptr &&
      session()->error() == QUIC_CRYPTO_HANDSHAKE_STATELESS_REJECT) {
      // The overall connection failed due too many stateless rejects.
      set_connection_error(QUIC_CRYPTO_TOO_MANY_REJECTS);
    }
    return session()->connection()->connected();
  }


  bool QuicRawClient::CreateUDPSocketAndBind(IPEndPoint server_address,
                                                IPAddress bind_to_address,
                                                int bind_to_port) {
    std::unique_ptr<UDPClientSocket> socket(
        new UDPClientSocket(DatagramSocket::DEFAULT_BIND, RandIntCallback()));

    if (!bind_to_address.empty()) {
      client_address_ = IPEndPoint(bind_to_address, local_port());
    } else if (server_address.GetFamily() == net::ADDRESS_FAMILY_IPV4) {
      client_address_ = IPEndPoint(IPAddress::IPv4AllZeros(), bind_to_port);
    } else {
      client_address_ = IPEndPoint(IPAddress::IPv6AllZeros(), bind_to_port);
    }

    int rc = socket->Connect(server_address);
    if (rc != OK) {
      return false;
    }

    rc = socket->SetReceiveBufferSize(kDefaultSocketReceiveBuffer);
    if (rc != OK) {
      return false;
    }

    rc = socket->SetSendBufferSize(kDefaultSocketReceiveBuffer);
    if (rc != OK) {
      return false;
    }

    IPEndPoint address;
    rc = socket->GetLocalAddress(&address);
    if (rc != OK) {
      return false;
    }
    client_address_ = address;

    socket_.swap(socket);
    packet_reader_.reset(new QuicChromiumPacketReader(
        socket_.get(), &clock_, this, kQuicYieldAfterPacketsRead,
        QuicTime::Delta::FromMilliseconds(kQuicYieldAfterDurationMilliseconds)));

    if (socket != nullptr) {
      socket->Close();
    }

    return true;
  }

  QuartcStream* QuicRawClient::CreateClientStream() {
    if (!connected()) {
      return nullptr;
    }

    QuartcStream* stream =
      session_->CreateOutgoingDynamicStream();

    stream->SetDelegate(stream_delegate_.get());

    return stream;
  }

  void QuicRawClient::SendData(QuartcStream * stream, const char * data, size_t size, bool fin)
  {
    QuartcStream::WriteParameters params;

    params.fin = fin;

    stream->Write(data, size, params);
  }

  bool QuicRawClient::WaitForEvents() {

    RunEventLoop();

    if (!connected() &&
      session()->error() == QUIC_CRYPTO_HANDSHAKE_STATELESS_REJECT) {
      Connect();
    }

    return session()->num_active_requests() != 0;
  }

  bool QuicRawClient::connected() const {
    return session_.get() && session_->connection() &&
      session_->connection()->connected();
  }

  void QuicRawClient::StartConnect() {

    QuicPacketWriter* writer = CreateQuicPacketWriter();

    if (connected_or_attempting_connect()) {
      // If the last error was not a stateless reject, then the queued up data
      // does not need to be resent.
      if (session()->error() != QUIC_CRYPTO_HANDSHAKE_STATELESS_REJECT) {
        ClearDataToResend();
      }
      // Before we destroy the last session and create a new one, gather its stats
      // and update the stats for the overall connection.
      UpdateStats();
    }

    CreateQuicRawSession(new QuicRawConnection(
      GetNextConnectionId(), server_address(), helper(), alarm_factory(),
      writer,
      /* owns_writer= */ false, Perspective::IS_CLIENT, supported_versions()));

    // Reset |writer()| after |session()| so that the old writer outlives the old
    // session.
    set_writer(writer);

    session()->StartCryptoHandshake();

    // session()->Initialize();
    // session()->CryptoConnect();

    set_connected_or_attempting_connect(true);
  }

  void QuicRawClient::Disconnect() {

    if (connected()) {
      session()->connection()->CloseConnection(
        QUIC_PEER_GOING_AWAY, "Client disconnecting",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
    }

    ClearDataToResend();

    CleanUpAllUDPSockets();

    initialized_ = false;
  }

  ProofVerifier* QuicRawClient::proof_verifier() const {
    return crypto_config_.proof_verifier();
  }

  bool QuicRawClient::EncryptionBeingEstablished() {
    return !session_->IsEncryptionEstablished() &&
      session_->connection()->connected();
  }

  QuartcStream * QuicRawClient::NewStreamAndSendData(const char * data, size_t size, bool fin)
  {
    QuartcStream * stream = CreateClientStream();

    SendData(stream, data, size, fin);

    return stream;
  }

  int QuicRawClient::GetNumSentClientHellos() {
    // If we are not actively attempting to connect, the session object
    // corresponds to the previous connection and should not be used.
    const int current_session_hellos = !connected_or_attempting_connect_
      ? 0
      : session_->GetNumSentClientHellos();
    return num_sent_client_hellos_ + current_session_hellos;
  }

  void QuicRawClient::UpdateStats() {
    num_sent_client_hellos_ += session()->GetNumSentClientHellos();
    if (session()->error() == QUIC_CRYPTO_HANDSHAKE_STATELESS_REJECT) {
      ++num_stateless_rejects_received_;
    }
  }

  QuicRawSession* QuicRawClient::CreateQuicRawSession(
    QuicConnection* connection) {
    const std::string server_finger = "Filegear";
    session_.reset(new QuicRawSession(
      std::move(std::unique_ptr<QuicConnection>(connection)),
      config_,
      server_finger,
      Perspective::IS_CLIENT,
      helper_.get()
    ));

    if (session_delegate_) {
      session()->SetDelegate(session_delegate_.get());
    }

    if (initial_max_packet_length_ != 0) {
      session()->connection()->SetMaxPacketLength(initial_max_packet_length_);
    }
    return session_.get();
  }

  QuicConnectionId QuicRawClient::GetNextConnectionId() {
    QuicConnectionId server_designated_id = GetNextServerDesignatedConnectionId();
    return server_designated_id ? server_designated_id
      : GenerateNewConnectionId();
  }

  QuicConnectionId QuicRawClient::GetNextServerDesignatedConnectionId() {
    QuicCryptoClientConfig::CachedState* cached =
      crypto_config_.LookupOrCreate(server_id_);
    // If the cached state indicates that we should use a server-designated
    // connection ID, then return that connection ID.
    return cached->has_server_designated_connection_id()
      ? cached->GetNextServerDesignatedConnectionId()
      : 0;
  }

  QuicConnectionId QuicRawClient::GenerateNewConnectionId() {
    return QuicRandom::GetInstance()->RandUint64();
  }

  void QuicRawClient::ClearDataToResend() {
    data_to_resend_on_connect_.clear();
  }

  void QuicRawClient::ResendSavedData() {
    // Calling Resend will re-enqueue the data, so swap out
    //  data_to_resend_on_connect_ before iterating.
    std::vector<std::unique_ptr<QuicDataToResend>> old_data;
    old_data.swap(data_to_resend_on_connect_);
    for (const auto& data : old_data) {
      data->Resend();
    }
  }

  void QuicRawClient::WaitForStreamToClose(QuicStreamId id) {
    while (connected() && !session_->IsClosedStream(id)) {
      WaitForEvents();
    }
  }

  bool QuicRawClient::WaitForCryptoHandshakeConfirmed() {
    while (connected() && !session_->IsCryptoHandshakeConfirmed()) {
      WaitForEvents();
    }

    // If the handshake fails due to a timeout, the connection will be closed.
    return connected();
  }

  bool QuicRawClient::MigrateSocket(const IPAddress& new_host) {
    if (!connected()) {
      return false;
    }

    CleanUpAllUDPSockets();

    set_bind_to_address(new_host);
    if (!CreateUDPSocketAndBind(server_address_, bind_to_address_, local_port_)) {
      return false;
    }

    session()->connection()->SetSelfAddress(GetLatestClientAddress());

    QuicPacketWriter* writer = CreateQuicPacketWriter();
    set_writer(writer);
    session()->connection()->SetQuicPacketWriter(writer, false);

    return true;
  }

  bool QuicRawClient::goaway_received() const {
    return session_ != nullptr && session_->goaway_received();
  }


  int QuicRawClient::GetNumReceivedServerConfigUpdates() {
    // If we are not actively attempting to connect, the session object
    // corresponds to the previous connection and should not be used.
    // We do not need to take stateless rejects into account, since we
    // don't expect any scup messages to be sent during a
    // statelessly-rejected connection.
    return !connected_or_attempting_connect_
      ? 0
      : session_->GetNumReceivedServerConfigUpdates();
  }


  QuicErrorCode QuicRawClient::connection_error() const {
    // Return the high-level error if there was one.  Otherwise, return the
    // connection error from the last session.
    if (connection_error_ != QUIC_NO_ERROR) {
      return connection_error_;
    }
    if (session_ == nullptr) {
      return QUIC_NO_ERROR;
    }
    return session_->error();
  }


  void QuicRawClient::MaybeAddDataToResend(StringPiece body,
    bool fin) {
    if (!FLAGS_quic_reloadable_flag_enable_quic_stateless_reject_support) {
      return;
    }

    if (session()->IsCryptoHandshakeConfirmed()) {
      // The handshake is confirmed.  No need to continue saving requests to
      // resend.
      data_to_resend_on_connect_.clear();
      return;
    }

    // The handshake is not confirmed.  Push the data onto the queue of data to
    // resend if statelessly rejected.
    std::unique_ptr<QuicDataToResend> data_to_resend(
      new ClientQuicDataToResend(body, fin, this));
    MaybeAddQuicDataToResend(std::move(data_to_resend));
  }

  void QuicRawClient::MaybeAddQuicDataToResend(
    std::unique_ptr<QuicDataToResend> data_to_resend) {
    data_to_resend_on_connect_.push_back(std::move(data_to_resend));
  }



  void QuicRawClient::CleanUpAllUDPSockets() {
    reset_writer();
    packet_reader_.reset();
    packet_reader_started_ = false;

  }

  void QuicRawClient::StartPacketReaderIfNotStarted() {
    if (!packet_reader_started_) {
      packet_reader_->StartReading();
      packet_reader_started_ = true;
    }
  }

  void QuicRawClient::RunEventLoop() {
    StartPacketReaderIfNotStarted();
    base::RunLoop().RunUntilIdle();
  }

  QuicConnectionHelper* QuicRawClient::CreateQuicConnectionHelper() {
    return new QuicConnectionHelper(&clock_, QuicRandom::GetInstance());
  }

  QuicAlarmFactory* QuicRawClient::CreateQuicAlarmFactory() {
    return new QuicAlarmFactoryImpl(base::ThreadTaskRunnerHandle::Get().get(),
                                        &clock_);
  }

  QuicPacketWriter* QuicRawClient::CreateQuicPacketWriter() {
    return new QuicChromiumPacketWriter(socket_.get(), base::ThreadTaskRunnerHandle::Get().get());
  }

  void QuicRawClient::OnReadError(int result,
                                     const DatagramClientSocket* socket) {
    Disconnect();
  }

  IPEndPoint QuicRawClient::GetLatestClientAddress() const {
    return client_address_;
  }

  bool QuicRawClient::OnPacket(const QuicReceivedPacket& packet,
                               const IPEndPoint& local_address,
                               const IPEndPoint& peer_address) {
    session()->connection()->ProcessUdpPacket(local_address, peer_address, packet);
    if (!session()->connection()->connected()) {
      return false;
    }

    return true;
  }

}  // namespace net
