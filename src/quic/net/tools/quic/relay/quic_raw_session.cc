#include "net/tools/quic/relay/quic_raw_session.h"
#include "net/tools/quic/relay/quartc_session_visitor.h"
#include "net/tools/quic/relay/quic_crypto_client_stream_visitor.h"
#include "net/tools/quic/relay/quic_crypto_server_stream_visitor.h"
#include "net/tools/quic/relay/quic_raw_client_handshaker.h"
#include "net/tools/quic/relay/quic_raw_server_handshaker.h"


#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/callback.h"
#include "base/bind.h"

#include "net/quic/platform/impl/quic_chromium_clock.h"

namespace net {

QuicRawSession::~QuicRawSession() {
}

QuicRawSession::QuicRawSession(std::unique_ptr<QuicConnection> connection,
    const QuicConfig & config,
    const std::string & remote_fingerprint_value,
    Perspective perspective,
    QuicConnectionHelper * helper)
  : QuartcSession(std::move(connection),
      config,
      remote_fingerprint_value,
      perspective,
      helper,
      QuicChromiumClock::GetInstance()
      )
{
}

void QuicRawSession::StartCryptoHandshake() {

  QuartcSessionVisitor session(this);

  if (session.GetPerspective() == Perspective::IS_CLIENT)
  {
    QuicServerId serverId(session.ServerId(), 0);

    QuicCryptoClientStreamVisitor stream(new QuicCryptoClientStream(
      serverId, this, new ProofVerifyContext(), session.ClientConfig(), this
    ));

    stream.ResetHandshaker(new QuicRawClientHandshaker(
      serverId, stream.get(), this, new ProofVerifyContext(), session.ClientConfig(), this
    ));

    session.ResetCryptoStream(stream.get());

    QuicSession::Initialize();

    stream.get()->CryptoConnect();
  }
  else
  {
    QuicCompressedCertsCache * certsCache = new QuicCompressedCertsCache(
        QuicCompressedCertsCache::kQuicCompressedCertsCacheSize);
    session.ResetQuicCompressedCertsCache(certsCache);

    bool use_stateless_rejects_if_peer_supported = false;

    QuicCryptoServerStreamVisitor stream(new QuicCryptoServerStream(
        session.ServerConfig(), certsCache,
        use_stateless_rejects_if_peer_supported, this, session.ServerStreamHelper()));

    stream.ResetHandshaker(new QuicRawServerHandshaker(
        session.ServerConfig(), stream.get(), certsCache, use_stateless_rejects_if_peer_supported,
        this, session.ServerStreamHelper()));

    session.ResetCryptoStream(stream.get());

    QuicSession::Initialize();
  }
}


int QuicRawSession::GetNumSentClientHellos()
{
  const QuicCryptoClientStream * stream = static_cast<const QuicCryptoClientStream *>(
    this->GetCryptoStream()
    );

  return stream->num_sent_client_hellos();
}

int QuicRawSession::GetNumReceivedServerConfigUpdates()
{
  const QuicCryptoClientStream * stream = static_cast<const QuicCryptoClientStream *>(
    this->GetCryptoStream()
    );

  return stream->num_scup_messages_received();
}


}
