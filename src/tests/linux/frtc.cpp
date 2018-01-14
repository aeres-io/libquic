// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A binary wrapper for QuicClient.
// Connects to a host using QUIC, sends a request to the provided URL, and
// displays the response.
//
// Some usage examples:
//
//   TODO(rtenneti): make --host optional by getting IP Address of URL's host.
//
//   Get IP address of the www.google.com
//   IP=`dig www.google.com +short | head -1`
//
// Standard request/response:
//   quic_client http://www.google.com  --host=${IP}
//   quic_client http://www.google.com --quiet  --host=${IP}
//   quic_client https://www.google.com --port=443  --host=${IP}
//
// Use a specific version:
//   quic_client http://www.google.com --quic_version=23  --host=${IP}
//
// Send a POST instead of a GET:
//   quic_client http://www.google.com --body="this is a POST body" --host=${IP}
//
// Append additional headers to the request:
//   quic_client http://www.google.com  --host=${IP}
//               --headers="Header-A: 1234; Header-B: 5678"
//
// Connect to a host different to the URL being requested:
//   Get IP address of the www.google.com
//   IP=`dig www.google.com +short | head -1`
//   quic_client mail.google.com --host=${IP}
//
// Try to connect to a host which does not speak QUIC:
//   Get IP address of the www.example.com
//   IP=`dig www.example.com +short | head -1`
//   quic_client http://www.example.com --host=${IP}

#include <iostream>
#include "base/at_exit.h"
#include "base/message_loop/message_loop.h"
#include "net/base/net_errors.h"
#include "net/base/privacy_mode.h"

#include "net/quic/core/quic_error_codes.h"
#include "net/quic/core/quic_packets.h"
#include "net/quic/core/quic_server_id.h"
#include "net/quic/platform/api/quic_str_cat.h"
#include "net/quic/platform/api/quic_text_utils.h"

#include "net/tools/quic/relay/quic_raw_client.h"
#include "quic_gear_session_delegate.h"
#include "quic_relay_client_config.h"

#include "mongoose.h"

#include "frtc_util.h"
#include "command_line_options.h"


using base::StringPiece;
//using net::CertVerifier;
//using net::CTPolicyEnforcer;
//using net::CTVerifier;
//using net::MultiLogCTVerifier;
using net::ProofVerifier;
//using net::ProofVerifierChromium;
using net::QuicTextUtils;
//using net::TransportSecurityState;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

// If true, a version mismatch in the handshake is not considered a failure.
// Useful for probing a server to determine if it speaks any version of QUIC.
bool FLAGS_version_mismatch_ok = false;
// If true, an HTTP response code of 3xx is considered to be a successful
// response, otherwise a failure.
bool FLAGS_redirect_is_success = true;

class FakeProofVerifier : public ProofVerifier {
public:
	net::QuicAsyncStatus VerifyProof(
		const string& hostname,
		const uint16_t port,
		const string& server_config,
		net::QuicTransportVersion quic_version,
		StringPiece chlo_hash,
		const std::vector<string>& certs,
		const string& cert_sct,
		const string& signature,
		const net::ProofVerifyContext* context,
		string* error_details,
		std::unique_ptr<net::ProofVerifyDetails>* details,
		std::unique_ptr<net::ProofVerifierCallback> callback) override {
		return net::QUIC_SUCCESS;
	}

	net::QuicAsyncStatus VerifyCertChain(
		const std::string& hostname,
		const std::vector<std::string>& certs,
		const net::ProofVerifyContext* verify_context,
		std::string* error_details,
		std::unique_ptr<net::ProofVerifyDetails>* verify_details,
		std::unique_ptr<net::ProofVerifierCallback> callback) override {
		return net::QUIC_SUCCESS;
	}
};

int on_http_request(mg_connection * connection)
{
  mg_request_info * ri = mg_get_request_info(connection);
  (void)ri;

  printf("Incoming HTTP request\n");

  // TODO: handle the request

  return 0; // return 0 for "not handled", 1 for "handled"
}

int main(int argc, char* argv[]) {
  if (!CommandLineOptions::Init(argc, argv))
  {
    return 1;
  }

  /*
	if (line->HasSwitch("quic-version")) {
		int quic_version;
		if (base::StringToInt(line->GetSwitchValueASCII("quic-version"),
			&quic_version)) {
			FLAGS_quic_version = quic_version;
		}
	}
	if (line->HasSwitch("version_mismatch_ok")) {
		FLAGS_version_mismatch_ok = true;
	}
	if (line->HasSwitch("redirect_is_success")) {
		FLAGS_redirect_is_success = true;
	}
  */

  // TODO: replace this with a command-line option
  const char* certificate = 
#if defined(_WIN32) || defined(WIN32)
    "c:\\filegear-mesh\\WebCertificate.pem";
#else
    "/var/www/html/WebCertificate.pem";
#endif

  // TODO: implement callback functions. Compare with Jason's code
  mg_callbacks callbacks = {};
  callbacks.begin_request = &on_http_request;

  const char* options[] = {
    "listening_ports", "443s", //relayPort.c_str(),     // local listening port
    "num_threads", "10",
    "ssl_certificate", certificate,           // path to certificate file
    "document_root",
#if defined(_WIN32) || defined(WIN32)
    "c:\\html",
#else
    "/var/www/html",
#endif
    NULL
  };

  mg_context* context = mg_start(&callbacks, NULL, options);

  if (context == nullptr)
  {
    printf("Unable to start web server.\n");
    return 1;
  }

  base::AtExitManager exit_manager;
  base::MessageLoopForIO message_loop;


  net::IPAddress ip_addr;

  int rv = ResolveIPAddress(CommandLineOptions::host, ip_addr);

  if (rv != net::OK) {
    return 1;
  }

  //the code below is used to testing IPV6 only
  //net::IPAddress address2;
  //string host2 = "www.google.com";
  //rv = ResolveIPAddress(host2, address2);
  //if (rv != net::OK) printf("Testing for IP6 failed\n");
  

  // Build the client, and try to connect.
  net::QuicServerId server_id(CommandLineOptions::host, CommandLineOptions::port,
    net::PRIVACY_MODE_DISABLED);
  net::ParsedQuicVersionVector versions = net::AllSupportedVersions();

  if (CommandLineOptions::quic_version != -1) {
    versions.clear();
    versions.push_back(net::ParsedQuicVersion(net::PROTOCOL_QUIC_CRYPTO, (net::QuicTransportVersion)CommandLineOptions::quic_version));
  }

  size_t retry_times = 0;
  const size_t max_retry_times = 1;

	do {
		// determine usable public ip address
		bool found_valid_ip = false;

    found_valid_ip = true;
    net::QuicRelayClientConfig::kIpAddress = net::IPAddress(127, 0, 0, 1);

		if (!found_valid_ip)
		{
			std::cerr << "Looks like no public ip available" << std::endl;
			sleep(3);
			continue;
		}

		// For secure QUIC we need to verify the cert chain.
		//std::unique_ptr<CertVerifier> cert_verifier(CertVerifier::CreateDefault());
		//std::unique_ptr<TransportSecurityState> transport_security_state(
			//new TransportSecurityState);
		//std::unique_ptr<CTVerifier> ct_verifier(new MultiLogCTVerifier());
		//std::unique_ptr<CTPolicyEnforcer> ct_policy_enforcer(new CTPolicyEnforcer());
		std::unique_ptr<ProofVerifier> proof_verifier;
    /*
		if (!CommandLineOptions::verify_certificates)
    {
			proof_verifier.reset(new FakeProofVerifier());
		}
		else
    {
			proof_verifier.reset(new ProofVerifierChromium(
				cert_verifier.get(), ct_policy_enforcer.get(),
				transport_security_state.get(), ct_verifier.get()));
		}
		*/
    proof_verifier.reset(new FakeProofVerifier());

		net::QuicRawClient client(net::IPEndPoint(ip_addr, CommandLineOptions::port), server_id, versions, std::move(proof_verifier));
		client.set_initial_max_packet_length(CommandLineOptions::initial_mtu != 0 ? CommandLineOptions::initial_mtu : net::kDefaultMaxPacketSize);

    if (!client.Initialize())
    {
			cerr << "Failed to initialize client." << endl;
			return 1;
		}

    client.ResetSessionDelegate(new net::QuicGearSessionDelegate(client.session()));

		if (!client.Connect())
    {
			net::QuicErrorCode error = client.session()->error();
			cerr << "Failed to connect to " << CommandLineOptions::host << ":" << CommandLineOptions::port
				<< ". Error: " << net::QuicErrorCodeToString(error) << endl;

			sleep(2);

			continue;
		}

		cout << "Connected to " << CommandLineOptions::host << ":" << CommandLineOptions::port << endl;

		// Send the request.
		const std::string data_to_send = CommandLineOptions::uuid + ".filegear.me" + CommandLineOptions::token;

		net::QuartcStream * s = client.CreateClientStream();

		client.SendData(s, data_to_send.c_str(), data_to_send.size(), false);

		while (client.connected()) {
			client.WaitForEvents();
			//USleep(10 * 1000);
		}

	} while (retry_times++ < max_retry_times);

	// service will restart the gear relay in case the device uuid changed
	// or the token invalidated

  mg_stop(context);

	return 0;
}
