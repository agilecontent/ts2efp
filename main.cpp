#include <iostream>
#include "kissnet/kissnet.hpp"
#include "mpegts_demuxer.h"
#include "simple_buffer.h"
#include "ts_packet.h"

#define LISTEN_INTERFACE "127.0.0.1"
#define LISTEN_PORT 8080

int main() {
  std::cout << "TS-2-EFP Listens at: " << LISTEN_INTERFACE << ":" << unsigned(LISTEN_PORT) << std::endl;
  SimpleBuffer in;
  MpegTsDemuxer demuxer;
  kissnet::udp_socket serverSocket(kissnet::endpoint(LISTEN_INTERFACE, LISTEN_PORT));
  serverSocket.bind();
  kissnet::buffer<4096> receiveBuffer;
  while (true) {
    auto[received_bytes, status] = serverSocket.recv(receiveBuffer);
    if (!received_bytes || status != kissnet::socket_status::valid ) {
      break;
    }
    in.append((const char*)receiveBuffer.data(), received_bytes);
    TsFrame *frame = nullptr;
    demuxer.decode(&in, frame);
    if (frame) {
      std::cout << "GOT: " << unsigned(frame->stream_type);
      if (frame->stream_type == 0x0f) {
        std::cout << " AAC Frame";
      } else if (frame->stream_type == 0x1b) {
        std::cout << " H.264 frame";
      }
      std::cout << std::endl;
    }
  }


  return 0;
}