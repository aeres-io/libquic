#include "quic_gear_session_delegate.h"
#include "net/tools/quic/relay/quic_raw_session.h"
#include "quic_relay_client_config.h"
#include "net/socket/tcp_client_socket.h"
#include "net/socket/stream_socket.h"
#include "net/base/net_errors.h"
#include "net/base/io_buffer.h"
#include <iostream>

namespace net { namespace {

const int kReadBufferSize = 16 * 4096;
const int kWriteBufferSize = 16 * 4096;

typedef scoped_refptr<IOBufferWithSize> IOBufferPtr;

//! Delegate for streams inited from Gear side
//! Gear should use one same stream for report
class QuicGearIncomeStreamDelegate : public QuartcStreamInterface::Delegate
{
public:

  QuicGearIncomeStreamDelegate(
      QuicRawSession * session,
      QuartcStreamInterface * stream)
    : session_(session)
    , stream_(stream)
    , read_buffer_(new IOBufferWithSize(kReadBufferSize))
    , write_buffer_(new IOBufferWithSize(kWriteBufferSize))
  {
    connect_callback_ = base::Bind(
        &QuicGearIncomeStreamDelegate::OnConnectCompleted, base::Unretained(this)
        );

    write_callback_ = base::Bind(
        &QuicGearIncomeStreamDelegate::OnWriteCompleted, base::Unretained(this)
        );

    read_callback_ = base::Bind(
        &QuicGearIncomeStreamDelegate::OnReadCompleted, base::Unretained(this)
        );
  }

  ~QuicGearIncomeStreamDelegate()
  {
    // printf("DEBUG: delegation freed=%p\n", this);
  }

  bool StreamClosed()
  {
    return stream_closed_;
  }

  void OnReadCompleted(int rc)
  {
    // printf("DEBUG: OnReadCompleted with rc = %d\n", rc);

    if (!stream_closed_)
    {
      if (rc > 0)
      {
        // printf("DEBUG: receive tcp service response: %s\n", std::string(read_buffer_->data(), rc).c_str());
        stream_->Write(read_buffer_->data(), rc,
            QuartcStreamInterface::WriteParameters());

        StartRead();
      }
      else
      {
        //! TODO:
        //! rc == 0:
        //!   normal connection close, no more data to read, 
        //!   but we need to wait for write completed
        //!   then we can close the connections
        //!          
        //! rc <  0:
        //!   failed connection and we need to close stream
        if (rc == 0)
        {
          read_completed_ = true;
        }
        else
        {
          // stream_->Close();
        }
      }
    }
    else
    {
    }
  }

  void StartRead()
  {
    int rc = tcp_socket_->Read(
        read_buffer_.get(),
        read_buffer_->size(),
        read_callback_);

    if (rc != ERR_IO_PENDING)
    {
      // printf("DEBUG: sync OnReadCompleted\n");
      OnReadCompleted(rc);
    }
  }

  void OnConnectCompleted(int rc)
  {
    if (rc < 0)
    {
      // connection failed
      // TODO:
      //   1. remove this relay connection
      //   2. close corresponding incoming stream

      printf("ERROR: cannot connect to local tcp service\n");
      stream_->Close();
    }

    connected_ = true;

    tcp_socket_->SetNoDelay(true);

    // printf("DEBUG: connected to local service, start read\n");

    TrySendBufferedData();

    StartRead();
  }

  void OnReceived(QuartcStreamInterface * stream,
      const char * data, size_t size) override
  {
    if (size == 0)
      return;

    if (read_times_ == 0)
    {
      if (!CreateTcpClientSocket())
      {
        printf("ERROR: cannot connect to local endpoint\n");
      }
    }

    // printf("DEBUG: receive cloud stream contents: %s\n", std::string(data, size).c_str());

    WriteOrBufferData(data, size);

    read_times_++;
  }

  void WriteOrBufferData(const char * data, size_t size)
  {
    if (!ReadyToWrite())
    {
      IOBufferPtr buffer(new IOBufferWithSize(size));
      memmove(buffer->data(), data, size);
      buffered_writes_.push(buffer);
    }
    else
    {
      bytes_to_write_ = size;
      bytes_written_offset_ = 0;

      memmove(write_buffer_->data(), data, size);
	  int rc = tcp_socket_->Write(write_buffer_.get(), bytes_to_write_, write_callback_);
      if (rc != ERR_IO_PENDING)
      {
        // printf("DEBUG: sync OnWriteCompleted\n");
        OnWriteCompleted(rc);
      }
    }
  }

  bool LastWriteCompleted()
  {
    return bytes_written_offset_ == bytes_to_write_;
  }

  bool ReadyToWrite()
  {
    return LastWriteCompleted() && Connected();
  }

  bool Connected() const
  {
    return connected_;
  }

  void TrySendBufferedData()
  {
    if (ReadyToWrite())
    {
      if (buffered_writes_.size())
      {
        IOBufferPtr buffer = buffered_writes_.front();
        buffered_writes_.pop();

        assert(buffer->size() < write_buffer_->size());
        memmove(write_buffer_->data(), buffer->data(), buffer->size());

        bytes_to_write_ = buffer->size();
        bytes_written_offset_ = 0;

        int rc = tcp_socket_->Write(write_buffer_.get(),
            bytes_to_write_,
            write_callback_);

        if (rc != ERR_IO_PENDING)
        {
          // printf("DEBUG: sync OnWriteCompleted\n");
          OnWriteCompleted(rc);
        }
      }
      else
      {
        // no more data to write
        if (read_completed_)
        {
          // stream_->Close();
        }
      }
    }
  }

  void OnWriteCompleted(int rc)
  {
    // printf("DEBUG: write completed with rc=%d\n", rc);
    if (rc > 0)
    {
      bytes_written_offset_ += rc;
      TrySendBufferedData();
    }
    else
    {
      // stream_->Close();
    }
  }

  bool CreateTcpClientSocket()
  {
    AddressList address_list = AddressList(
      IPEndPoint(
			  QuicRelayClientConfig::kIpAddress,
			  QuicRelayClientConfig::kTcpServicePort
      )
    );

    assert(tcp_socket_.get() == nullptr);

    tcp_socket_.reset(
      new TCPClientSocket(address_list, NULL)
    );

    int rc = tcp_socket_->Connect(connect_callback_);
    if (rc != ERR_IO_PENDING)
    {
      OnConnectCompleted(rc);

      if (!Connected())
      {
        stream_->Close();
        return false;
      }
    }

    return true;
  }

  void OnClose(QuartcStreamInterface * stream) override
  {
    stream_closed_ = true;
  }

  void OnCanWrite(QuartcStreamInterface* stream) override
  {
  }

private:

  // unowned
  QuicRawSession * session_;
  QuartcStreamInterface * stream_;

  std::unique_ptr<TCPClientSocket> tcp_socket_;
  std::queue<IOBufferPtr> buffered_writes_;

  IOBufferPtr read_buffer_;
  IOBufferPtr write_buffer_;

  CompletionCallback connect_callback_;
  CompletionCallback write_callback_;
  CompletionCallback read_callback_;

  size_t bytes_to_write_ = 0;
  size_t bytes_written_offset_ = 0;
  size_t read_times_ = 0;

  bool connected_ = false;
  bool stream_closed_ = false;
  bool read_completed_ = false;

};


}}

//--------------------------------------------------------------------------------------- 

namespace net {

QuicGearSessionDelegate::QuicGearSessionDelegate(
    QuicRawSession * target
    )
  : session_(target)
{
}


void QuicGearSessionDelegate::OnCryptoHandshakeComplete() {
  std::cout << "A new session handshake completed." << std::endl;
}


void QuicGearSessionDelegate::OnIncomingStream(QuartcStreamInterface * stream) {
  // std::cout << "New incoming stream." << std::endl;

  // clean closed stream delegates
  auto it = stream_delegates_.begin();
  while (it != stream_delegates_.end())
  {
    QuicGearIncomeStreamDelegate * d = 
      static_cast<QuicGearIncomeStreamDelegate *>(*it);

    if (d->StreamClosed())
    {
      delete d;
      stream_delegates_.erase(it++);
    }
    else
    {
      ++it;
    }
  }

  // create a new delegate instance, which is responsible for relay stuff
  QuartcStreamInterface::Delegate * delegate = 
    new QuicGearIncomeStreamDelegate(session_, stream);

  stream->SetDelegate(delegate);

  stream_delegates_.insert(delegate);
}


void QuicGearSessionDelegate::OnConnectionClosed(int error_code, bool from_remote) {
  std::cout << "connection closed" << std::endl;
}

QuicGearSessionDelegate::~QuicGearSessionDelegate()
{
  for (auto d : stream_delegates_)
  {
    delete d;
  }

  stream_delegates_.clear();
}

}
