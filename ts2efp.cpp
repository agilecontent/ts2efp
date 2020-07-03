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
MpegTsDemuxer demuxer;

bool firstRun = {true};
ElasticFrameMessages efpMessage = {ElasticFrameMessages::noError};

//Here is where you need to add logic for how you want to map TS to EFP
//In this simple example we map one h264 video and the first AAC audio we find.
uint16_t videoPID = 0;
uint16_t audioPID = 0;
uint8_t efpStreamIDVideo = 0;
uint8_t efpStreamIDAudio = 0;

void sendData(const std::vector<uint8_t> &subPacket) {
    //EFP Send data
    serverRelay.send((const std::byte *) subPacket.data(), subPacket.size());
}


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

void dmxOutput(EsFrame *pEs){

    if (firstRun && demuxer.mPmtIsValid) {
        if (!mapTStoEFP(demuxer.mPmtHeader)) {
            std::cout << "Unable to find a video and a audio in this TS stream" << std::endl;
        }
    }

    std::cout << "GOT: " << unsigned(pEs->mStreamType);
    if (pEs->mStreamType == 0x0f && pEs->mPid == audioPID) {

        //  for (auto lIt = demuxer.mStreamPidMap.begin(); lIt != demuxer.mStreamPidMap.end(); lIt++) {
        //      std::cout << "Hej: " << (int)lIt->first << " " << (int)lIt->second << std::endl;
        //  }

        std::cout << " AAC Frame";
        efpMessage = myEFPSend.packAndSendFromPtr((const uint8_t *) pEs->mData->data(),
                                                  pEs->mData->size(),
                                                  ElasticFrameContent::adts,
                                                  pEs->mPts,
                                                  pEs->mPts,
                                                  EFP_CODE('A', 'D', 'T', 'S'),
                                                  efpStreamIDAudio,
                                                  NO_FLAGS
        );

        if (efpMessage != ElasticFrameMessages::noError) {
            std::cout << "h264 packAndSendFromPtr error " << std::endl;
        }
    } else if (pEs->mStreamType == 0x1b && pEs->mPid == videoPID) {
        std::cout << " H.264 frame";
        efpMessage = myEFPSend.packAndSendFromPtr((const uint8_t *) pEs->mData->data(),
                                                  pEs->mData->size(),
                                                  ElasticFrameContent::h264,
                                                  pEs->mPts,
                                                  pEs->mDts,
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

int main() {
    std::cout << "TS-2-EFP Listens at: " << LISTEN_INTERFACE << ":" << unsigned(LISTEN_PORT) << std::endl;

    myEFPSend.sendCallback = std::bind(&sendData, std::placeholders::_1);

    SimpleBuffer in;
    demuxer.esOutCallback = std::bind(&dmxOutput, std::placeholders::_1);

    kissnet::udp_socket serverSocket(kissnet::endpoint(LISTEN_INTERFACE, LISTEN_PORT));
    serverSocket.bind();
    kissnet::buffer<4096> receiveBuffer;
    while (true) {
        auto[received_bytes, status] = serverSocket.recv(receiveBuffer);
        if (!received_bytes || status != kissnet::socket_status::valid) {
            break;
        }
        in.append((uint8_t*)receiveBuffer.data(), received_bytes);
        demuxer.decode(in);
    }
    return EXIT_SUCCESS;
}