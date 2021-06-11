/*
	    File: CMIO_DPA_Sample_Server_VCamDevice.cpp
	Abstract: n/a
	 Version: 1.2
 
*/

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//	Includes
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Self Include
#include "CMIO_DPA_Sample_Server_VCamDevice.h"

// Internal Includes
#include "CMIO_DPA_Sample_Server_VCamInputStream.h"
#include "CAHostTimeBase.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>

namespace CMIO { namespace DPA { namespace Sample { namespace Server
{
	#pragma mark -
	#pragma mark VCamDevice
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// VCamDevice()
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//	VCamDevice::VCamDevice() :
//        Device()
//	{
//        std::cout << "Don't do anything" << std::endl;
//		CreateStreams();
//
//        mSequenceFile = fopen("/Library/CoreMediaIO/Plug-Ins/DAL/QuickomCamera.plugin/Contents/Resources/ntsc2vuy720x480.yuv", "rb");
//        mFrameSize = 1280 * 720 * 2;
////        mFrameSize = 720 * 480 * 2;
//
//        fseek(mSequenceFile, 0, SEEK_END);
//        mFrameCount = ftell(mSequenceFile) / mFrameSize;
//
//        mFramebuffer = new uint8_t[mFrameSize];
//
//        pthread_create(&mThread, NULL, &VCamDevice::EmitFrame, this);
//        pthread_create(&mReceiveThread, NULL, &VCamDevice::ReceiveFrame, this);
//	}

VCamDevice::VCamDevice(int type) :
        Device()
    {
        std::cout << "Create device with type" << type << std::endl;
        CreateStreams(type);

        mSequenceFile = fopen("/Library/CoreMediaIO/Plug-Ins/DAL/QuickomCamera.plugin/Contents/Resources/ntsc2vuy720x480.yuv", "rb");
        switch(type){
            case 480:
                mFrameSize = 720 * 480 * 2;
                break;
            case 720:
                mFrameSize = 1280 * 720 * 2;
                break;
            case 1080:
                mFrameSize = 1920 * 1080 * 2;
                break;
            default:
                mFrameSize = 1280 * 720 * 2;
                break;
        }
//        mFrameSize = 1280 * 720 * 2;
//        mFrameSize = 720 * 480 * 2;

        fseek(mSequenceFile, 0, SEEK_END);
        mFrameCount = ftell(mSequenceFile) / mFrameSize;

        mFramebuffer = new uint8_t[mFrameSize];

        pthread_create(&mThread, NULL, &VCamDevice::EmitFrame, this);
        pthread_create(&mReceiveThread, NULL, &VCamDevice::ReceiveFrame, this);
    }
	
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// ~VCamDevice()
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	VCamDevice::~VCamDevice()
	{
        fclose(mSequenceFile);
	}

	#pragma mark -
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//  CreateStreams()
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void VCamDevice::CreateStreams(int type)
	{
        UInt32 streamID = 0;
        
        CACFDictionary format;
        switch(type){
            case 480:
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_CodecType), kYUV422_720x480);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_CodecFlags), kSampleCodecFlags_60fps | kSampleCodecFlags_1001_1000_adjust);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_Width), 720);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_Height), 480);
                break;
            case 720:
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_CodecType), kYUV422_1280x720);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_CodecFlags), kSampleCodecFlags_60fps | kSampleCodecFlags_1001_1000_adjust);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_Width), 1280);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_Height), 720);
                break;
            case 1080:
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_CodecType), kYUV422_1920x1080);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_CodecFlags), kSampleCodecFlags_60fps | kSampleCodecFlags_1001_1000_adjust);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_Width), 1920);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_Height), 1080);
                break;
            default:
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_CodecType), kYUV422_1280x720);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_CodecFlags), kSampleCodecFlags_60fps | kSampleCodecFlags_1001_1000_adjust);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_Width), 1280);
                format.AddUInt32(CFSTR(kIOVideoStreamFormatKey_Height), 720);
                break;
        }
        CACFArray formats;
        formats.AppendDictionary(format.GetDict());

        CACFDictionary streamDict;
        streamDict.AddArray(CFSTR(kIOVideoStreamKey_AvailableFormats), formats.GetCFArray());
        streamDict.AddUInt32(CFSTR(kIOVideoStreamKey_StartingDeviceChannelNumber), 1);

        mInputStream = new VCamInputStream(this, streamDict.GetDict(), kCMIODevicePropertyScopeInput);
        mInputStreams[streamID] = mInputStream;
    }
    
    #pragma mark -
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //  EmitFrame()
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    void* VCamDevice::EmitFrame(void* device) {
        VCamDevice* vcamDevice = (VCamDevice*)device;
//        uint8_t* framebuffer = new uint8_t[vcamDevice->mFrameSize];

//        while (true) {
//            usleep(1000 * 1000 / 30);
//
//            fseek(vcamDevice->mSequenceFile, (vcamDevice->mFrameIndex % vcamDevice->mFrameCount) * vcamDevice->mFrameSize, SEEK_SET);
//            fread(framebuffer, 1, vcamDevice->mFrameSize, vcamDevice->mSequenceFile);
//            ++vcamDevice->mFrameIndex;
//            // Hack because it seems that vcamDevice->mInputStream->GetTimecode() is always 0
//            UInt64 vbiTime = CAHostTimeBase::GetCurrentTimeInNanos();
//            vcamDevice->mInputStream->FrameArrived(vcamDevice->mFrameSize, framebuffer, vbiTime);
//        }

        while (true) {
            usleep(1000 * 1000 / 60);

            UInt64 vbiTime = CAHostTimeBase::GetCurrentTimeInNanos();
            vcamDevice->mInputStream->FrameArrived(vcamDevice->mFrameSize, vcamDevice->mFramebuffer, vbiTime);
        }
    }

    void* VCamDevice::ReceiveFrame(void* device) {
        VCamDevice* vcamDevice = (VCamDevice*)device;

        // サーバーソケット作成
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("socket");
            return NULL;
        }

        // Set socket buffer size
        int bufferSize = 1024 * 1024;
        socklen_t bufferSizeLen = sizeof(bufferSize);
        if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&bufferSize), bufferSizeLen) == -1) {
            perror("setsockopt error");
        }

        // struct sockaddr_un 作成
        struct sockaddr_un sa = {0};
        sa.sun_family = AF_UNIX;
        strcpy(sa.sun_path, "/tmp/vcam-socket");
//        strcpy(sa.sun_path, "wss://signal-conference-staging.quickom.com/?id=abc");

        // 既に同一ファイルが存在していたら削除
        remove(sa.sun_path);

        // バインド
        if (bind(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un)) == -1) {
            perror("bind");
            goto bail;
        }

        // リッスン
        if (listen(sock, 128) == -1) {
            perror("listen");
            goto bail;
        }

        while (1) {
            // クライアントの接続を待つ
            int fd = accept(sock, NULL, NULL);
            if (fd == -1) {
                perror("accept");
                goto bail;
            }

            int totalReceived = 0;

            while (true) {
                // 受信
                int recv_size = read(fd, vcamDevice->mFramebuffer + totalReceived, vcamDevice->mFrameSize - totalReceived);
                if (recv_size == -1)
                {
                    perror("read");
                    close(fd);
                    goto bail;
                }
                if (recv_size == 0) {
                    // disconnected
                    totalReceived = 0;
                    break;
                }
                //std::cout << recv_size << std::endl;
                totalReceived += recv_size;

                if (totalReceived == vcamDevice->mFrameSize) {
                    // frame complete
                    totalReceived = 0;
                    ++vcamDevice->mFrameCount;
//                    std::cout << "test file:" << vcamDevice->mFrameCount << std::endl;
                }
            }

            // ソケットのクローズ
            if (close(fd) == -1) {
                perror("close");
                goto bail;
            }
        }

    bail:
        // エラーが発生した場合の処理
        close(sock);
        return NULL;
    }

}}}}
