#include <iostream>
#include <fstream> //Debug
#include "kissnet/kissnet.hpp"
#include "mpegts_demuxer.h"
#include "simple_buffer.h"
#include "ts_packet.h"
#include "ElasticFrameProtocol.h"

#define MTU 1456 //SRT-max
#define LISTEN_INTERFACE "127.0.0.1"
#define LISTEN_PORT 8080

ElasticFrameProtocol myEFPSend(MTU, ElasticFrameMode::sender);

void sendData(const std::vector<uint8_t> &subPacket) {
  std::cout << "Boom!" << std::endl;
}

int main() {
  std::cout << "TS-2-EFP Listens at: " << LISTEN_INTERFACE << ":" << unsigned(LISTEN_PORT) << std::endl;

  std::ofstream out_adts("out.adts", std::ios::binary);
  std::ofstream out_h264("out.h264", std::ios::binary);

  myEFPSend.sendCallback = std::bind(&sendData, std::placeholders::_1);
  ElasticFrameMessages efpMessage;

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
       // out_adts.write(frame->_data->data(), frame->_data->size());
        efpMessage = myEFPSend.packAndSendFromPtr((const uint8_t*)frame->_data->data(),
                                                  frame->_data->size(),
                                                  ElasticFrameContent::adts,
                                                  frame->pts,
                                                  frame->pts,
                                                  EFP_CODE('A', 'D', 'T', 'S'),
                                                  21,
                                                  NO_FLAGS
        );

        if (efpMessage != ElasticFrameMessages::noError) {
          std::cout << "h264 packAndSendFromPtr error " << std::endl;
        }
      } else if (frame->stream_type == 0x1b) {
        std::cout << " H.264 frame";
       // out_h264.write(frame->_data->data(), frame->_data->size());
        efpMessage = myEFPSend.packAndSendFromPtr((const uint8_t*)frame->_data->data(),
            frame->_data->size(),
            ElasticFrameContent::h264,
            frame->pts,
            frame->dts,
            EFP_CODE('A', 'N', 'X', 'B'),
            20,
            NO_FLAGS
            );

         if (efpMessage != ElasticFrameMessages::noError) {
           std::cout << "h264 packAndSendFromPtr error " << std::endl;
         }

      }

      std::cout << " completed: " << unsigned(frame->_data->size()) << " " << unsigned(frame->pts) ;

      std::cout << std::endl;
    }
  }
  out_h264.close();
  out_adts.close();
  return EXIT_SUCCESS;
}