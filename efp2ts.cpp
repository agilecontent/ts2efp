//
// Created by Anders Cedronius on 2020-06-15.
//

#include <iostream>
#include "kissnet/kissnet.hpp"
#include "mpegts_muxer.h"
#include "simple_buffer.h"
#include "ts_packet.h"
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
ElasticFrameProtocolReceiver myEFPReciever(10,4);
MpegTsMuxer muxer;
std::map<uint8_t, int> streamPidMap;
SimpleBuffer tsOutBuffer;
TsFrame tsFrame;

void gotData(ElasticFrameProtocolReceiver::pFramePtr &rPacket) {
    if (rPacket->mDataContent == ElasticFrameContent::h264) {
        tsFrame.mData = std::make_shared<SimpleBuffer>();
        tsFrame.mData->append((const char *) rPacket->pFrameData, rPacket->mFrameSize);
        tsFrame.mPts = rPacket->mPts;
        tsFrame.mDts = rPacket->mDts;
        tsFrame.mPcr = 0;
        tsFrame.mStreamType = TYPE_VIDEO;
        tsFrame.mStreamId = 224;
        tsFrame.mPid = VIDEO_PID;
        tsFrame.mExpectedPesPacketLength = 0;
        tsFrame.mCompleted = true;
        muxer.encode(&tsFrame, streamPidMap, PMT_PID, &tsOutBuffer);
    } else if (rPacket->mDataContent == ElasticFrameContent::adts) {
        tsFrame.mData = std::make_shared<SimpleBuffer>();
        tsFrame.mData->append((const char *) rPacket->pFrameData, rPacket->mFrameSize);
        tsFrame.mPts = rPacket->mPts;
        tsFrame.mDts = rPacket->mPts;
        tsFrame.mPcr = 0;
        tsFrame.mStreamType = TYPE_AUDIO;
        tsFrame.mStreamId = 192;
        tsFrame.mPid = AUDIO_PID;
        tsFrame.mExpectedPesPacketLength = 0;
        tsFrame.mCompleted = true;
        muxer.encode(&tsFrame, streamPidMap, PMT_PID, &tsOutBuffer);
    }

    //Double to fail at non integer data and be able to visualize in the print-out
    double packets = (double)tsOutBuffer.size() / 188.0;
    std::cout << "Sending -> " << packets << " MPEG-TS packets" << std::endl;
    char* lpData = tsOutBuffer.data();
    for (int lI = 0 ; lI < packets ; lI++) {
        mpegTsOut.send((const std::byte *)lpData+(lI*188), 188);
    }
    tsOutBuffer.erase(tsOutBuffer.size());
}

int main() {
    std::cout << "EFP-2-TS Listens at: " << LISTEN_INTERFACE << ":" << unsigned(LISTEN_PORT) << std::endl;

    //Set the pids. In this example hardcoded. EFPSignal should be used here for dynamic flows.
    streamPidMap[TYPE_AUDIO] = AUDIO_PID;
    streamPidMap[TYPE_VIDEO] = VIDEO_PID;

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
    return EXIT_SUCCESS;
}