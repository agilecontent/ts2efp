//
// Created by Anders Cedronius on 2020-06-15.
//

#include <iostream>
#include "kissnet/kissnet.hpp"
#include "mpegts_muxer.h"
#include "ElasticFrameProtocol.h"

#define LISTEN_INTERFACE "127.0.0.1"
#define LISTEN_PORT 8090
#define TARGET_INTERFACE "127.0.0.1"
#define TARGET_PORT 8100

#define TYPE_AUDIO 0x0f
#define TYPE_VIDEO 0x1b
#define AUDIO_PID 257
#define VIDEO_PID 256

#define PMT_PID 100

kissnet::udp_socket mpegTsOut(kissnet::endpoint(TARGET_INTERFACE, TARGET_PORT));
ElasticFrameProtocolReceiver myEFPReciever(100,40);
MpegTsMuxer *muxer;
std::map<uint8_t, int> streamPidMap;


void gotData(ElasticFrameProtocolReceiver::pFramePtr &rPacket) {
    EsFrame esFrame;
    if (rPacket->mDataContent == ElasticFrameContent::h264) {
        esFrame.mData = std::make_shared<SimpleBuffer>();
        esFrame.mData->append(rPacket->pFrameData, rPacket->mFrameSize);
        esFrame.mPts = rPacket->mPts;
        esFrame.mDts = rPacket->mDts;
        esFrame.mPcr = rPacket->mDts;
        esFrame.mStreamType = TYPE_VIDEO;
        esFrame.mStreamId = 224;
        esFrame.mPid = VIDEO_PID;
        esFrame.mExpectedPesPacketLength = 0;
        esFrame.mCompleted = true;
        muxer->encode(esFrame);
    } else if (rPacket->mDataContent == ElasticFrameContent::adts) {
        esFrame.mData = std::make_shared<SimpleBuffer>();
        esFrame.mData->append(rPacket->pFrameData, rPacket->mFrameSize);
        esFrame.mPts = rPacket->mPts;
        esFrame.mDts = rPacket->mPts;
        esFrame.mPcr = 0;
        esFrame.mStreamType = TYPE_AUDIO;
        esFrame.mStreamId = 192;
        esFrame.mPid = AUDIO_PID;
        esFrame.mExpectedPesPacketLength = 0;
        esFrame.mCompleted = true;
        muxer->encode(esFrame);
    }
}

void muxOutput(SimpleBuffer &rTsOutBuffer) {
    double packets = (double)rTsOutBuffer.size() / 188.0;
    std::cout << "Sending -> " << packets << " MPEG-TS packets" << std::endl;
    uint8_t* lpData = rTsOutBuffer.data();
    for (int lI = 0 ; lI < packets ; lI++) {
        mpegTsOut.send((const std::byte *)lpData+(lI*188), 188);
    }
}

int main() {
    std::cout << "EFP-2-TS Listens at: " << LISTEN_INTERFACE << ":" << unsigned(LISTEN_PORT) << std::endl;

    //Set the pids. In this example hardcoded. EFPSignal should be used here for dynamic flows.
    streamPidMap[TYPE_AUDIO] = AUDIO_PID;
    streamPidMap[TYPE_VIDEO] = VIDEO_PID;
    muxer = new MpegTsMuxer(streamPidMap, PMT_PID, VIDEO_PID, MpegTsMuxer::MuxType::h222Type);
    muxer->tsOutCallback = std::bind(&muxOutput, std::placeholders::_1);

    myEFPReciever.receiveCallback = std::bind(gotData, std::placeholders::_1);
    ElasticFrameMessages efpMessage;

    kissnet::udp_socket serverSocket(kissnet::endpoint(LISTEN_INTERFACE, LISTEN_PORT));
    serverSocket.bind();
    kissnet::buffer<4096> receiveBuffer;
    while (true) {
        auto[received_bytes, status] = serverSocket.recv(receiveBuffer);
        if (!received_bytes || status != kissnet::socket_status::valid) {
            break;
        }
        efpMessage = myEFPReciever.receiveFragmentFromPtr((const uint8_t*) receiveBuffer.data(), received_bytes, 0);
        if (efpMessage < ElasticFrameMessages::noError) {
            std::cout << "EFP error: " << (int16_t)efpMessage << std::endl;
        }
    }
    delete muxer;
    return EXIT_SUCCESS;
}