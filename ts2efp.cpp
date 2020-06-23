#include <iostream>
#include "kissnet/kissnet.hpp"
#include "mpegts_demuxer.h"
#include "ElasticFrameProtocol.h"

#define MTU 1456 //SRT-max
#define LISTEN_INTERFACE "127.0.0.1"
#define LISTEN_PORT 8080
#define TARGET_INTERFACE "127.0.0.1"
#define TARGET_PORT 8090

#define TYPE_AUDIO 0x0f
#define TYPE_VIDEO 0x1b

ElasticFrameProtocolSender myEFPSend(MTU);
kissnet::udp_socket serverRelay(kissnet::endpoint(TARGET_INTERFACE, TARGET_PORT));

void sendData(const std::vector<uint8_t> &subPacket) {
    //EFP Send data
    serverRelay.send((const std::byte *) subPacket.data(), subPacket.size());
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
    for (auto &rStream: rPMTdata.mInfos) {
        if (rStream->mStreamType == TYPE_VIDEO && !foundVideo) {
            foundVideo = true;
            videoPID = rStream->mElementaryPid;
            efpStreamIDVideo = 10;
        }
        if (rStream->mStreamType == TYPE_AUDIO && !foundAudio) {
            foundAudio = true;
            audioPID = rStream->mElementaryPid;
            efpStreamIDAudio = 20;
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
        if (!received_bytes || status != kissnet::socket_status::valid) {
            break;
        }
        in.append((const char *) receiveBuffer.data(), received_bytes);
        TsFrame *frame = nullptr;
        demuxer.decode(&in, frame);
        if (frame) {

            if (firstRun && demuxer.mPmtIsValid) {
                if (!mapTStoEFP(demuxer.mPmtHeader)) {
                    std::cout << "Unable to find a video and a audio in this TS stream" << std::endl;
                    return EXIT_FAILURE;
                }
            }


            std::cout << "GOT: " << unsigned(frame->mStreamType);
            if (frame->mStreamType == 0x0f && frame->mPid == audioPID) {

              //  for (auto lIt = demuxer.mStreamPidMap.begin(); lIt != demuxer.mStreamPidMap.end(); lIt++) {
              //      std::cout << "Hej: " << (int)lIt->first << " " << (int)lIt->second << std::endl;
              //  }

                std::cout << " AAC Frame";
                efpMessage = myEFPSend.packAndSendFromPtr((const uint8_t *) frame->mData->data(),
                                                          frame->mData->size(),
                                                          ElasticFrameContent::adts,
                                                          frame->mPts,
                                                          frame->mPts,
                                                          EFP_CODE('A', 'D', 'T', 'S'),
                                                          efpStreamIDAudio,
                                                          NO_FLAGS
                );

                if (efpMessage != ElasticFrameMessages::noError) {
                    std::cout << "h264 packAndSendFromPtr error " << std::endl;
                }
            } else if (frame->mStreamType == 0x1b && frame->mPid == videoPID) {
                std::cout << " H.264 frame";
                efpMessage = myEFPSend.packAndSendFromPtr((const uint8_t *) frame->mData->data(),
                                                          frame->mData->size(),
                                                          ElasticFrameContent::h264,
                                                          frame->mPts,
                                                          frame->mDts,
                                                          EFP_CODE('A', 'N', 'X', 'B'),
                                                          efpStreamIDVideo,
                                                          NO_FLAGS
                );
                if (efpMessage != ElasticFrameMessages::noError) {
                    std::cout << "h264 packAndSendFromPtr error " << std::endl;
                }
            }
            std::cout << std::endl;
        }
    }
    return EXIT_SUCCESS;
}