// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A toy client, which connects to a specified port and sends QUIC
// request to that endpoint.


#ifndef NET_TOOLS_QUIC_QUIC_RAW_CLIENT_H_
#define NET_TOOLS_QUIC_QUIC_RAW_CLIENT_H_

#pragma once

#include "net/quic/platform/impl/quic_chromium_clock.h"
#include "net/quic/chromium/quic_chromium_packet_reader.h"
#include "net/quic/quartc/quartc_stream.h"
#include "net/tools/quic/relay/quic_raw_session.h"


namespace net {

class QuicChromiumConnectionHelper;
class UDPClientSocket;


class NET_EXPORT QuicRawClient : public QuicChromiumPacketReader::Visitor {
 public:

   class QuicDataToResend {
   public:
     QuicDataToResend(base::StringPiece body, bool fin);

     virtual ~QuicDataToResend();

     // Must be overridden by specific classes with the actual method for
     // re-sending data.
     virtual void Resend() = 0;

   protected:
     base::StringPiece body_;
     bool fin_;

   private:
     DISALLOW_COPY_AND_ASSIGN(QuicDataToResend);
   };


  // Create a quic client, which will have events managed by the message loop.
  QuicRawClient(IPEndPoint server_address,
                   const QuicServerId& server_id,
                   const net::ParsedQuicVersionVector& supported_versions,
		   std::unique_ptr<ProofVerifier> proof_verifier);

  ~QuicRawClient() override;

  //From QuicChromiumPacketReader::Visitor 
  void OnReadError(int result, const DatagramClientSocket* socket) override;

  bool OnPacket(const QuicReceivedPacket& packet,
                const IPEndPoint& local_address,
                const IPEndPoint& peer_address) override;

  // Initializes the client to create a connection. Should be called exactly
  // once before calling StartConnect or Connect. Returns true if the
  // initialization succeeds, false otherwise.
  virtual bool Initialize();

  // "Connect" to the QUIC server, including performing synchronous crypto
  // handshake.
  bool Connect();

  // Start the crypto handshake.  This can be done in place of the synchronous
  // Connect(), but callers are responsible for making sure the crypto
  // handshake
  // completes.
  void StartConnect();

  // Disconnects from the QUIC server.
  void Disconnect();

  // Returns true if the crypto handshake has yet to establish encryption.
  // Returns false if encryption is active (even if the server hasn't
  // confirmed
  // the handshake) or if the connection has been closed.
  bool EncryptionBeingEstablished();

  // Returns a newly created QuicSpdyClientStream, owned by the
  // QuicSimpleClient.
  virtual QuartcStream * CreateClientStream();

  virtual QuartcStream * NewStreamAndSendData(const char * data, size_t size, bool fin);

  virtual void SendData(QuartcStream * stream, const char * data, size_t size, bool fin);

  // Wait for events until the stream with the given ID is closed.
  void WaitForStreamToClose(QuicStreamId id);

  // Wait for events until the handshake is confirmed.
  // Returns true if the crypto handshake succeeds, false otherwise.
  bool WaitForCryptoHandshakeConfirmed() WARN_UNUSED_RESULT;

  // Wait up to 50ms, and handle any events which occur.
  // Returns true if there are any outstanding requests.
  bool WaitForEvents();

  // Migrate to a new socket during an active connection.
  bool MigrateSocket(const IPAddress& new_host);

  QuicRawSession* session() { return session_.get(); }

  bool connected() const;
  bool goaway_received() const;

  const QuicServerId& server_id() const { return server_id_; }

  // This should only be set before the initial Connect()
  void set_server_id(const QuicServerId& server_id) { server_id_ = server_id; }

  void SetUserAgentID(const std::string& user_agent_id) {
    crypto_config_.set_user_agent_id(user_agent_id);
  }

  // SetChannelIDSource sets a ChannelIDSource that will be called, when the
  // server supports channel IDs, to obtain a channel ID for signing a message
  // proving possession of the channel ID. This object takes ownership of
  // |source|.
  void SetChannelIDSource(ChannelIDSource* source) {
    crypto_config_.SetChannelIDSource(source);
  }

  // UseTokenBinding enables token binding negotiation in the client.  This
  // should only be called before the initial Connect().  The client will
  // still
  // need to check that token binding is negotiated with the server, and add
  // token binding headers to requests if so.  server, and add token binding
  // headers to requests if so.  The negotiated token binding parameters can
  // be
  // found on the QuicCryptoNegotiatedParameters object in
  // token_binding_key_param.
  void UseTokenBinding() {
    crypto_config_.tb_key_params = QuicTagVector{ kTB10 };
  }

  const ParsedQuicVersionVector& supported_versions() const {
    return supported_versions_;
  }

  void SetSupportedVersions(const ParsedQuicVersionVector& versions) {
    supported_versions_ = versions;
  }
 
  QuicConfig* config() { return &config_; }

  QuicCryptoClientConfig* crypto_config() { return &crypto_config_; }

  // Change the initial maximum packet size of the connection.  Has to be
  // called
  // before Connect()/StartConnect() in order to have any effect.
  void set_initial_max_packet_length(QuicByteCount initial_max_packet_length) {
    initial_max_packet_length_ = initial_max_packet_length;
  }

  int num_stateless_rejects_received() const {
    return num_stateless_rejects_received_;
  }

  // The number of client hellos sent, taking stateless rejects into
  // account.  In the case of a stateless reject, the initial
  // connection object may be torn down and a new one created.  The
  // user cannot rely upon the latest connection object to get the
  // total number of client hellos sent, and should use this function
  // instead.
  int GetNumSentClientHellos();

  // Gather the stats for the last session and update the stats for the
  // overall
  // connection.
  void UpdateStats();

  // The number of server config updates received.  We assume no
  // updates can be sent during a previously, statelessly rejected
  // connection, so only the latest session is taken into account.
  int GetNumReceivedServerConfigUpdates();

  // Returns any errors that occurred at the connection-level (as
  // opposed to the session-level).  When a stateless reject occurs,
  // the error of the last session may not reflect the overall state
  // of the connection.
  QuicErrorCode connection_error() const;
  void set_connection_error(QuicErrorCode connection_error) {
    connection_error_ = connection_error;
  }

  bool connected_or_attempting_connect() const {
    return connected_or_attempting_connect_;
  }
  void set_connected_or_attempting_connect(
    bool connected_or_attempting_connect) {
    connected_or_attempting_connect_ = connected_or_attempting_connect;
  }

  QuicPacketWriter* writer() { return writer_.get(); }
  void set_writer(QuicPacketWriter* writer) {
    if (writer_.get() != writer) {
      writer_.reset(writer);
    }
  }
  void reset_writer() { writer_.reset(); }

  QuicByteCount initial_max_packet_length() {
    return initial_max_packet_length_;
  }

  ProofVerifier* proof_verifier() const;

  void set_session(QuicRawSession* session) { session_.reset(session); }

  // If the crypto handshake has not yet been confirmed, adds the data to the
  // queue of data to resend if the client receives a stateless reject.
  // Otherwise, deletes the data.
  void MaybeAddQuicDataToResend(
    std::unique_ptr<QuicDataToResend> data_to_resend);


  void set_bind_to_address(IPAddress address) {
    bind_to_address_ = address;
  }

  IPAddress bind_to_address() const { return bind_to_address_; }

  void set_local_port(int local_port) { local_port_ = local_port; }

  int local_port() const { return local_port_; }

  const IPEndPoint& server_address() const { return server_address_; }

  void set_server_address(const IPEndPoint& server_address) {
    server_address_ = server_address;
  }

  void ResetSessionDelegate(QuartcSessionInterface::Delegate * val) {
    session_delegate_.reset(val);
  }

 protected:
  // Unregister and close all open UDP sockets.
  void CleanUpAllUDPSockets();

  // Creates a packet writer to be used for the next connection.
  QuicPacketWriter* CreateQuicPacketWriter();

  // Takes ownership of |connection|.
  QuicRawSession* CreateQuicRawSession(
    QuicConnection* connection);

  // Runs one iteration of the event loop.
  void RunEventLoop();

  // Used during initialization: creates the UDP socket FD, sets socket
  // options,
  // and binds the socket to our address.
  bool CreateUDPSocketAndBind(IPEndPoint server_address,
    IPAddress bind_to_address,
    int bind_to_port);

  
  // If the client has at least one UDP socket, return address of the latest
  // created one. Otherwise, return an empty socket address.
  IPEndPoint GetLatestClientAddress() const;

  // Generates the next ConnectionId for |server_id_|.  By default, if the
  // cached server config contains a server-designated ID, that ID will be
  // returned.  Otherwise, the next random ID will be returned.
  QuicConnectionId GetNextConnectionId();

  // Returns the next server-designated ConnectionId from the cached config
  // for
  // |server_id_|, if it exists.  Otherwise, returns 0.
  QuicConnectionId GetNextServerDesignatedConnectionId();

  // Generates a new, random connection ID (as opposed to a server-designated
  // connection ID).
  virtual QuicConnectionId GenerateNewConnectionId();

  // If the crypto handshake has not yet been confirmed, adds the data to the
  // queue of data to resend if the client receives a stateless reject.
  // Otherwise, deletes the data.
  void MaybeAddDataToResend(base::StringPiece body,
    bool fin);

  void ClearDataToResend();

  void ResendSavedData();

  QuicConnectionHelper* helper() { return helper_.get(); }

  QuicAlarmFactory* alarm_factory() { return alarm_factory_.get(); }

  void set_num_sent_client_hellos(int num_sent_client_hellos) {
    num_sent_client_hellos_ = num_sent_client_hellos;
  }

  void set_num_stateless_rejects_received(int num_stateless_rejects_received) {
    num_stateless_rejects_received_ = num_stateless_rejects_received;
  }

 private:
  QuicAlarmFactory* CreateQuicAlarmFactory();
  QuicConnectionHelper* CreateQuicConnectionHelper();

  void StartPacketReaderIfNotStarted();

  class ClientQuicDataToResend : public QuicDataToResend {
    public:
      ClientQuicDataToResend(base::StringPiece body,
        bool fin,
        QuicRawClient * client)
        : QuicDataToResend(body, fin), client_(client) {
      }

      ~ClientQuicDataToResend() override {}
      void Resend() override;

    private:
      QuicRawClient * client_;
      DISALLOW_COPY_AND_ASSIGN(ClientQuicDataToResend);
  };

  //  Used by |helper_| to time alarms.
  QuicChromiumClock clock_;

  // Address of the client if the client is connected to the server.
  IPEndPoint client_address_;

  // UDP socket connected to the server.
  std::unique_ptr<UDPClientSocket> socket_;

  // Tracks if the client is initialized to connect.
  bool initialized_;

  std::unique_ptr<QuicChromiumPacketReader> packet_reader_;

  bool packet_reader_started_;

  base::WeakPtrFactory<QuicRawClient> weak_factory_;

  // |server_id_| is a tuple (hostname, port, is_https) of the server.
  QuicServerId server_id_;

  
  // Address of the server.
  IPEndPoint server_address_;

  // If initialized, the address to bind to.
  IPAddress bind_to_address_;

  // Local port to bind to. Initialize to 0.
  int local_port_;

  // config_ and crypto_config_ contain configuration and cached state about
  // servers.
  QuicConfig config_;
  QuicCryptoClientConfig crypto_config_;

  // Helper to be used by created connections. Must outlive |session_|.
  std::unique_ptr<QuicConnectionHelper> helper_;

  // Alarm factory to be used by created connections. Must outlive |session_|.
  std::unique_ptr<QuicAlarmFactory> alarm_factory_;

  // Writer used to actually send packets to the wire. Must outlive
  // |session_|.
  std::unique_ptr<QuicPacketWriter> writer_;

  // Session which manages streams.
  std::unique_ptr<QuicRawSession> session_;

  // The initial value of maximum packet size of the connection.  If set to
  // zero, the default is used.
  QuicByteCount initial_max_packet_length_;

  // The number of stateless rejects received during the current/latest
  // connection.
  // TODO(jokulik): Consider some consistent naming scheme (or other) for
  // member
  // variables that are kept per-request, per-connection, and over the
  // client's
  // lifetime.
  int num_stateless_rejects_received_;

  // The number of hellos sent during the current/latest connection.
  int num_sent_client_hellos_;

  // This vector contains QUIC versions which we currently support.
  // This should be ordered such that the highest supported version is the
  // first
  // element, with subsequent elements in descending order (versions can be
  // skipped as necessary). We will always pick supported_versions_[0] as the
  // initial version to use.
  ParsedQuicVersionVector supported_versions_;
  
  // Used to store any errors that occurred with the overall connection (as
  // opposed to that associated with the last session object).
  QuicErrorCode connection_error_;

  // True when the client is attempting to connect or re-connect the session
  // (in
  // the case of a stateless reject).  Set to false  between a call to
  // Disconnect() and the subsequent call to StartConnect().  When
  // connected_or_attempting_connect_ is false, the session object corresponds
  // to the previous client-level connection.
  bool connected_or_attempting_connect_;

  // Keeps track of any data that must be resent upon a subsequent successful
  // connection, in case the client receives a stateless reject.
  std::vector<std::unique_ptr<QuicDataToResend>> data_to_resend_on_connect_;

  std::unique_ptr<QuartcStreamInterface::Delegate>  stream_delegate_;

  std::unique_ptr<QuartcSessionInterface::Delegate> session_delegate_;

  DISALLOW_COPY_AND_ASSIGN(QuicRawClient);
};

}  // namespace net

#endif  // NET_TOOLS_QUIC_QUIC_RAW_CLIENT_H_
