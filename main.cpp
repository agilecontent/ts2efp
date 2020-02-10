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
  //EFP Send data
}

//Here is where you need to add logic for how you want to map TS to EFP
//In this simple example we map one h264 video and the first AAC audio we find.
uint16_t videoPID = 0;
uint16_t audioPID = 0;
uint8_t efpStreamIDVideo = 0;
uint8_t efpStreamIDAudio = 0;
bool mapTStoEFP(PMTHeader &rPMTdata) {
  bool foundVideo = false;
  bool foundAudio = false;
  for (auto &rStream: rPMTdata.infos) {
    if (rStream->stream_type == 0x1b && !foundVideo) {
      foundVideo = true;
      videoPID = rStream -> elementary_PID;
      efpStreamIDVideo = 10;
    }
    if (rStream->stream_type == 0x0f && !foundAudio) {
      foundAudio = true;
      audioPID = rStream -> elementary_PID;
      efpStreamIDAudio= 20;
    }
  }
  if (!foundVideo || !foundAudio) return false;
return true;
}

int main() {
  std::cout << "TS-2-EFP Listens at: " << LISTEN_INTERFACE << ":" << unsigned(LISTEN_PORT) << std::endl;

  myEFPSend.sendCallback = std::bind(&sendData, std::placeholders::_1);
  ElasticFrameMessages efpMessage;

  SimpleBuffer in;
  MpegTsDemuxer demuxer;
  kissnet::udp_socket serverSocket(kissnet::endpoint(LISTEN_INTERFACE, LISTEN_PORT));
  serverSocket.bind();
  kissnet::buffer<4096> receiveBuffer;
  bool firstRun = true;
  while (true) {
    auto[received_bytes, status] = serverSocket.recv(receiveBuffer);
    if (!received_bytes || status != kissnet::socket_status::valid ) {
      break;
    }
    in.append((const char*)receiveBuffer.data(), received_bytes);
    TsFrame *frame = nullptr;
    demuxer.decode(&in, frame);
    if (frame) {

      if (firstRun && demuxer.pmtIsValid) {
        if(!mapTStoEFP(demuxer.pmt_header)) {
          std::cout << "Unable to find a video and a audio in this TS stream" << std::endl;
          return EXIT_FAILURE;
        }
      }


      std::cout << "GOT: " << unsigned(frame->stream_type);
      if (frame->stream_type == 0x0f && frame->pid == audioPID) {
        std::cout << " AAC Frame";
        efpMessage = myEFPSend.packAndSendFromPtr((const uint8_t*)frame->_data->data(),
                                                  frame->_data->size(),
                                                  ElasticFrameContent::adts,
                                                  frame->pts,
                                                  frame->pts,
                                                  EFP_CODE('A', 'D', 'T', 'S'),
                                                  efpStreamIDAudio,
                                                  NO_FLAGS
        );

        if (efpMessage != ElasticFrameMessages::noError) {
          std::cout << "h264 packAndSendFromPtr error " << std::endl;
        }
      } else if (frame->stream_type == 0x1b && frame->pid == videoPID) {
        std::cout << " H.264 frame";
        efpMessage = myEFPSend.packAndSendFromPtr((const uint8_t*)frame->_data->data(),
            frame->_data->size(),
            ElasticFrameContent::h264,
            frame->pts,
            frame->dts,
            EFP_CODE('A', 'N', 'X', 'B'),
            efpStreamIDVideo,
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
  return EXIT_SUCCESS;
}