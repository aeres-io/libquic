#
# MIT License
#
# Copyright (c) 2018 aeres.io
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# =============================================================================
#

cmake_minimum_required(VERSION 3.1)

project(libquic)

set(ROOT ${PROJECT_SOURCE_DIR}/../..)

include(${ROOT}/Config.cmake)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MACOSX TRUE)
endif()

set(
  QUIC_SRC

	base/at_exit.cc
	base/base_switches.cc
	base/base64.cc
	base/bind_helpers.cc
	base/callback_helpers.cc
	base/callback_internal.cc
	base/files/file.cc
	base/files/file_descriptor_watcher_posix.cc
	base/files/file_enumerator.cc
	base/files/file_enumerator_posix.cc
	base/files/file_path.cc
	base/files/file_path_constants.cc
	base/files/file_posix.cc
	base/files/file_tracing.cc
	base/files/file_util.cc
	base/files/file_util_posix.cc
	base/files/scoped_file.cc
	base/hash.cc
	base/lazy_instance.cc
	base/memory/ref_counted.cc
	base/memory/singleton.cc
	base/memory/weak_ptr.cc
	base/message_loop/incoming_task_queue.cc
	base/message_loop/message_loop.cc
	base/message_loop/message_loop_task_runner.cc
	base/message_loop/message_pump.cc
	base/message_loop/message_pump_default.cc
	base/message_loop/message_pump_libevent.cc
	base/pickle.cc
	base/rand_util.cc
	base/run_loop.cc
	base/sequenced_task_runner.cc
	base/sha1.cc
	base/strings/pattern.cc
	base/strings/string_number_conversions.cc
	base/strings/string_piece.cc
	base/strings/string_split.cc
	base/strings/string_util.cc
	base/strings/string_util_constants.cc
	base/strings/string16.cc
	base/strings/stringprintf.cc
	base/synchronization/atomic_flag.cc
	base/synchronization/lock.cc
	base/task_runner.cc
	base/third_party/dmg_fp/g_fmt.cc
	base/third_party/dynamic_annotations/dynamic_annotations.c
	base/third_party/dmg_fp/dtoa.cc
	base/third_party/icu/icu_utf.cc
	base/third_party/libevent/buffer.c
	base/third_party/libevent/evbuffer.c
	base/third_party/libevent/evdns.c
	base/third_party/libevent/event.c
	base/third_party/libevent/event_tagging.c
	base/third_party/libevent/evrpc.c
	base/third_party/libevent/evutil.c
	base/third_party/libevent/http.c
	base/third_party/libevent/log.c
	base/third_party/libevent/poll.c
	base/third_party/libevent/select.c
	base/third_party/libevent/signal.c
	base/third_party/libevent/strlcpy.c
	base/third_party/superfasthash/superfasthash.c
	base/threading/post_task_and_reply_impl.cc
	base/threading/scoped_blocking_call.cc
	base/threading/sequence_local_storage_map.cc
	base/threading/sequenced_task_runner_handle.cc
	base/threading/simple_thread.cc
	base/threading/thread.cc
	base/threading/thread_id_name_manager.cc
	base/threading/thread_local_storage.cc
	base/threading/thread_local_storage_posix.cc
	base/threading/thread_task_runner_handle.cc
	base/time/clock.cc
	base/time/default_clock.cc
	base/time/default_tick_clock.cc
	base/time/tick_clock.cc
	base/time/time.cc
	base/timer/timer.cc
	crypto/ec_private_key.cc
	crypto/hkdf.cc
	crypto/hmac.cc
	crypto/openssl_util.cc
	crypto/random.cc
	crypto/rsa_private_key.cc
	crypto/secure_hash.cc
	crypto/secure_util.cc
	crypto/sha2.cc
	crypto/signature_verifier.cc
	crypto/symmetric_key.cc
	net/base/address_family.cc
	net/base/address_list.cc
	net/base/net_errors.cc
	net/base/host_port_pair.cc
	net/base/io_buffer.cc
	net/base/ip_address.cc
	net/base/ip_endpoint.cc
	net/base/load_timing_info.cc
	net/base/network_activity_monitor.cc
	net/base/request_priority.cc
	net/base/sockaddr_storage.cc
	net/quic/chromium/quic_chromium_alarm_factory.cc
	net/quic/chromium/quic_chromium_packet_reader.cc
	net/quic/chromium/quic_chromium_packet_writer.cc
	net/quic/core/congestion_control/cubic_bytes.cc
	net/quic/core/congestion_control/general_loss_algorithm.cc
	net/quic/core/congestion_control/hybrid_slow_start.cc
	net/quic/core/congestion_control/pacing_sender.cc
	net/quic/core/congestion_control/prr_sender.cc
	net/quic/core/congestion_control/rtt_stats.cc
	net/quic/core/congestion_control/send_algorithm_interface.cc
	net/quic/core/congestion_control/tcp_cubic_sender_bytes.cc
	net/quic/core/crypto/aead_base_decrypter.cc
	net/quic/core/crypto/aead_base_encrypter.cc
	net/quic/core/crypto/aes_128_gcm_12_decrypter.cc
	net/quic/core/crypto/aes_128_gcm_12_encrypter.cc
	net/quic/core/crypto/aes_128_gcm_decrypter.cc
	net/quic/core/crypto/aes_128_gcm_encrypter.cc
	net/quic/core/crypto/aes_256_gcm_decrypter.cc
	net/quic/core/crypto/aes_256_gcm_encrypter.cc
	net/quic/core/crypto/cert_compressor.cc
	net/quic/core/crypto/chacha20_poly1305_decrypter.cc
	net/quic/core/crypto/chacha20_poly1305_encrypter.cc
	net/quic/core/crypto/chacha20_poly1305_tls_decrypter.cc
	net/quic/core/crypto/chacha20_poly1305_tls_encrypter.cc
	net/quic/core/crypto/channel_id.cc
	net/quic/core/crypto/common_cert_set.cc
	net/quic/core/crypto/common_cert_set_2.c
	net/quic/core/crypto/common_cert_set_3.c
	net/quic/core/crypto/crypto_framer.cc
	net/quic/core/crypto/crypto_handshake.cc
	net/quic/core/crypto/crypto_handshake_message.cc
	net/quic/core/crypto/crypto_secret_boxer.cc
	net/quic/core/crypto/crypto_server_config_protobuf.cc
	net/quic/core/crypto/crypto_utils.cc
	net/quic/core/crypto/curve25519_key_exchange.cc
	net/quic/core/crypto/null_decrypter.cc
	net/quic/core/crypto/null_encrypter.cc
	net/quic/core/crypto/p256_key_exchange.cc
	net/quic/core/crypto/proof_source.cc
	net/quic/core/crypto/quic_compressed_certs_cache.cc
	net/quic/core/crypto/quic_crypto_client_config.cc
	net/quic/core/crypto/quic_crypto_proof.cc
	net/quic/core/crypto/quic_crypto_server_config.cc
	net/quic/core/crypto/quic_decrypter.cc
	net/quic/core/crypto/quic_encrypter.cc
	net/quic/core/crypto/quic_random.cc
	net/quic/core/crypto/quic_tls_adapter.cc
	net/quic/core/crypto/scoped_evp_aead_ctx.cc
	net/quic/core/frames/quic_ack_frame.cc
	net/quic/core/frames/quic_blocked_frame.cc
	net/quic/core/frames/quic_connection_close_frame.cc
	net/quic/core/frames/quic_frame.cc
	net/quic/core/frames/quic_goaway_frame.cc
	net/quic/core/frames/quic_padding_frame.cc
	net/quic/core/frames/quic_ping_frame.cc
	net/quic/core/frames/quic_rst_stream_frame.cc
	net/quic/core/frames/quic_stop_waiting_frame.cc
	net/quic/core/frames/quic_stream_frame.cc
	net/quic/core/frames/quic_window_update_frame.cc
	net/quic/core/proto/cached_network_parameters.pb.cc
	net/quic/core/proto/source_address_token.pb.cc
	net/quic/core/quic_ack_listener_interface.cc
	net/quic/core/quic_alarm.cc
	net/quic/core/quic_bandwidth.cc
	net/quic/core/quic_buffer_allocator.cc
	net/quic/core/quic_buffered_packet_store.cc
	net/quic/core/quic_config.cc
	net/quic/core/quic_connection.cc
	net/quic/core/quic_connection_stats.cc
	net/quic/core/quic_constants.cc
	net/quic/core/quic_control_frame_manager.cc
	net/quic/core/quic_crypto_client_handshaker.cc
	net/quic/core/quic_crypto_client_stream.cc
	net/quic/core/quic_crypto_handshaker.cc
	net/quic/core/quic_crypto_server_handshaker.cc
	net/quic/core/quic_crypto_server_stream.cc
	net/quic/core/quic_crypto_stream.cc
	net/quic/core/quic_data_reader.cc
	net/quic/core/quic_data_writer.cc
	net/quic/core/quic_error_codes.cc
	net/quic/core/quic_flow_controller.cc
	net/quic/core/quic_framer.cc
	net/quic/core/quic_header_list.cc
	net/quic/core/quic_headers_stream.cc
	net/quic/core/quic_packet_creator.cc
	net/quic/core/quic_packet_generator.cc
	net/quic/core/quic_packets.cc
	net/quic/core/quic_received_packet_manager.cc
	net/quic/core/quic_sent_packet_manager.cc
	net/quic/core/quic_server_id.cc
	net/quic/core/quic_server_session_base.cc
	net/quic/core/quic_session.cc
	net/quic/core/quic_simple_buffer_allocator.cc
	net/quic/core/quic_socket_address_coder.cc
	net/quic/core/quic_stream.cc
	net/quic/core/quic_stream_send_buffer.cc
	net/quic/core/quic_stream_sequencer_buffer.cc
	net/quic/core/quic_stream_sequencer.cc
	net/quic/core/quic_sustained_bandwidth_recorder.cc
	net/quic/core/quic_tag.cc
	net/quic/core/quic_time.cc
	net/quic/core/quic_transmission_info.cc
	net/quic/core/quic_types.cc
	net/quic/core/quic_unacked_packet_map.cc
	net/quic/core/quic_utils.cc
	net/quic/core/quic_version_manager.cc
	net/quic/core/quic_versions.cc
	net/quic/core/quic_write_blocked_list.cc
	net/quic/core/tls_client_handshaker.cc
	net/quic/core/tls_handshaker.cc
	net/quic/core/tls_server_handshaker.cc
	net/quic/platform/api/quic_clock.cc
	net/quic/platform/api/quic_mutex.cc
	net/quic/platform/impl/quic_chromium_clock.cc
	net/quic/platform/impl/quic_flags_impl.cc
	net/quic/platform/impl/quic_mem_slice_impl.cc
	net/quic/platform/impl/quic_mem_slice_span_impl.cc
	net/quic/platform/impl/quic_mutex_impl.cc
	net/quic/quartc/quartc_session.cc
	net/quic/quartc/quartc_stream.cc
	net/socket/client_socket_handle.cc
	net/socket/next_proto.cc
	net/socket/server_socket.cc
	net/socket/socket.cc
	net/socket/socket_descriptor.cc
	net/socket/socket_options.cc
	net/socket/stream_socket.cc
	net/socket/socket_tag.cc
	net/socket/tcp_client_socket.cc
	net/socket/tcp_server_socket.cc
	net/socket/udp_client_socket.cc
	net/socket/udp_server_socket.cc
	net/ssl/channel_id_store.cc
	net/ssl/ssl_config.cc
	net/ssl/ssl_config_service.cc
	net/ssl/ssl_client_session_cache.cc
	net/ssl/ssl_info.cc
	net/ssl/token_binding.cc
	net/tools/quic/chlo_extractor.cc
	net/tools/quic/quic_dispatcher.cc
	net/tools/quic/quic_per_connection_packet_writer.cc
	net/tools/quic/quic_simple_per_connection_packet_writer.cc
	net/tools/quic/quic_simple_server_packet_writer.cc
	net/tools/quic/quic_simple_server_session_helper.cc
	net/tools/quic/quic_time_wait_list_manager.cc
	net/tools/quic/stateless_rejector.cc
	net/tools/quic/relay/quic_alarm_factory_impl.cc
	net/tools/quic/relay/quic_raw_client.cc
	net/tools/quic/relay/quic_raw_client_handshaker.cc
	net/tools/quic/relay/quic_raw_connection.cc
	net/tools/quic/relay/quic_raw_dispatcher.cc
	net/tools/quic/relay/quic_raw_session.cc
	net/tools/quic/relay/quic_raw_server.cc
	net/tools/quic/relay/quic_raw_server_handshaker.cc
	net/tools/quic/relay/quic_raw_stream.cc
	third_party/modp_b64/modp_b64.cc
	third_party/protobuf/src/google/protobuf/arena.cc
	third_party/protobuf/src/google/protobuf/arenastring.cc
	third_party/protobuf/src/google/protobuf/generated_message_util.cc
	third_party/protobuf/src/google/protobuf/io/coded_stream.cc
	third_party/protobuf/src/google/protobuf/io/zero_copy_stream.cc
	third_party/protobuf/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
	third_party/protobuf/src/google/protobuf/message_lite.cc
	third_party/protobuf/src/google/protobuf/repeated_field.cc
	third_party/protobuf/src/google/protobuf/stubs/bytestream.cc
	third_party/protobuf/src/google/protobuf/stubs/common.cc
	third_party/protobuf/src/google/protobuf/stubs/int128.cc
	third_party/protobuf/src/google/protobuf/stubs/once.cc
	third_party/protobuf/src/google/protobuf/stubs/protobuf_stringprintf.cc
	third_party/protobuf/src/google/protobuf/stubs/protobuf_time.cc
	third_party/protobuf/src/google/protobuf/stubs/status.cc
	third_party/protobuf/src/google/protobuf/stubs/statusor.cc
	third_party/protobuf/src/google/protobuf/stubs/stringpiece.cc
	third_party/protobuf/src/google/protobuf/stubs/structurally_valid.cc
	third_party/protobuf/src/google/protobuf/stubs/strutil.cc
	third_party/protobuf/src/google/protobuf/wire_format_lite.cc
)

if (DEFINED UNIX)
  if (DEFINED MACOSX)
    set(
      QUIC_SRC ${QUIC_SRC}

      base/mac/scoped_nsautorelease_pool.mm
      base/mac/scoped_mach_port.cc
      base/message_loop/message_pump_mac.mm
      base/native_library_mac.mm
      base/pending_task.cc
      base/posix/safe_strerror.cc
      base/rand_util_posix.cc
      base/strings/sys_string_conversions_mac.mm
      base/strings/utf_string_conversions.cc
      base/strings/utf_string_conversion_utils.cc
      base/third_party/libevent/kqueue.c
      base/synchronization/condition_variable_posix.cc
      base/synchronization/lock_impl_posix.cc
      base/synchronization/waitable_event_mac.cc
      third_party/protobuf/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
      base/threading/platform_thread_internal_posix.cc
      base/threading/platform_thread_posix.cc
      base/threading/platform_thread_mac.mm
      base/time/time_conversion_posix.cc
      base/time/time_mac.cc
      net/base/net_errors_posix.cc
      net/socket/socket_posix.cc
      net/socket/tcp_socket_posix.cc
      net/socket/udp_socket_posix.cc
    )
  else()
    set(
      QUIC_SRC ${QUIC_SRC}

      base/native_library_posix.cc
      base/pending_task.cc
      base/posix/safe_strerror.cc
      base/rand_util_posix.cc
      base/strings/sys_string_conversions_posix.cc
      base/synchronization/condition_variable_posix.cc
      base/synchronization/lock_impl_posix.cc
      base/synchronization/waitable_event_posix.cc
      base/third_party/libevent/epoll.c
      third_party/protobuf/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
      base/threading/platform_thread_internal_posix.cc
      base/threading/platform_thread_posix.cc
      base/threading/platform_thread_linux.cc
      base/time/time_conversion_posix.cc
      base/time/time_exploded_posix.cc
      base/time/time_now_posix.cc
      net/base/net_errors_posix.cc
      net/socket/socket_posix.cc
      net/socket/tcp_socket_posix.cc
      net/socket/udp_socket_posix.cc
    )
  endif (DEFINED MACOSX)
endif (DEFINED UNIX)

add_library(quic SHARED ${QUIC_SRC})

set_target_properties(quic PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)
set_target_properties(quic PROPERTIES C_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)
set_property(TARGET quic PROPERTY POSITION_INDEPENDENT_CODE ON)

add_definitions(-DHAVE_CONFIG_H -DNO_TCMALLOC -DCOMPONENT_BUILD -DBASE_IMPLEMENTATION -DNET_IMPLEMENTATION -DURL_IMPLEMENTATION)

add_definitions(-DGOOGLE_PROTOBUF_NO_RTTI -DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER -DHAVE_PTHREAD)

include_directories("..")
include_directories("../third_party/boringssl/src/include")
include_directories("third_party/protobuf/src")
include_directories("third_party/googletest/src/googlemock/include")
include_directories("gen")

aeres_lib(quic libboringssl ${LIBDIR}/libboringssl.a)

if (DEFINED UNIX)
  if (((NOT DEFINED NSS_CFLAGS) AND (NOT DEFINED NSS_LIBS)) OR ((NOT DEFINED NSPR_CFLAGS) AND (NOT DEFINED NSPR_LIBS)))
    include_directories(".")
    include_directories("net/quic/core/proto")
    if (DEFINED MACOSX)
      include_directories("base/third_party/libevent/mac")
      add_definitions(-DUSE_NSS_CERTS -DOS_MACOSX)
      set(WARNINGS "${WARNINGS} -Wno-error=unused-const-variable -Wno-error=unused-variable -Wno-error=sign-compare -Wno-error=attributes")
    else()
      include_directories("base/third_party/libevent/linux")
      add_definitions(-DUSE_NSS_CERTS -DOS_LINUX)
      set(WARNINGS "${WARNINGS} -Wno-error=unused-const-variable -Wno-error=unused-variable -Wno-error=sign-compare -Wno-error=attributes")
    endif (DEFINED MACOSX)
    set(WARNINGS "${WARNINGS} -Wno-error=switch -Wno-error=unused-function -Wno-error=unused-local-typedefs")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNINGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNINGS} -fno-exceptions")

    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(NSS "nss")
      pkg_check_modules(NSPR "nspr")
      if (NSS_FOUND AND NSPR_FOUND)
        set(NSS_LIBS ${NSS_LIBRARIES})
        set(NSPR_LIBS ${NSPR_LIBRARIES})
      else()
        message(FATAL_ERROR "nss and nspr config not found in pkg-config. Please define NSS_CFLAGS, NSS_LIBS, NSPR_CFLAGS and NSPR_LIBS.")
      endif (NSS_FOUND AND NSPR_FOUND)
    else()
      message(FATAL_ERROR "nss and nspr config not found. Please define NSS_CFLAGS, NSS_LIBS, NSPR_CFLAGS and NSPR_LIBS.")
    endif (PKG_CONFIG_FOUND)
  endif (((NOT DEFINED NSS_CFLAGS) AND (NOT DEFINED NSS_LIBS)) OR ((NOT DEFINED NSPR_CFLAGS) AND (NOT DEFINED NSPR_LIBS)))

  add_definitions(${NSS_CFLAGS})
  add_definitions(${NSPR_CFLAGS})
  aeres_sys_lib(quic ${NSS_LDFLAGS})
  aeres_sys_lib(quic ${NSPR_LDFLAGS})

  if (DEFINED MACOSX)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework CoreFoundation -framework Cocoa -lz")
  endif (DEFINED MACOSX)
  
endif (DEFINED UNIX)

aeres_use_pthread(quic)

