#include "net/tools/quic/relay/quic_raw_stream.h"

#include <string>
#include <iostream>

namespace net {

void QuicRawStreamDelegate::OnReceived(QuartcStreamInterface * stream,
    const char * data, size_t size)
{
  if (size > 0)
  {
    QuartcStreamInterface::WriteParameters params;

    std::string o(data, size);
    std::cout << "Server has received stream contents: "
      << o << std::endl;

    stream->Write(data, size, params);
  }
}

void QuicRawStreamDelegate::OnClose(QuartcStreamInterface * stream)
{
  std::cout << "Stream closed." << std::endl;
}

void QuicRawStreamDelegate::OnCanWrite(QuartcStreamInterface* stream)
{
}

}
