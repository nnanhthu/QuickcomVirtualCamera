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
#include<sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <iostream>
#include "gocam.h"
//#include "libmyopencv.h"
//#include <opencv2/opencv.hpp>
//extern "C"{
//    #include "queue.h"
//}

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
        std::cout << "Create device with type " << type << std::endl;
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

        fseek(mSequenceFile, 0, SEEK_END);
        mFrameCount = ftell(mSequenceFile) / mFrameSize;

        std::cout<<"Frame size: "<<mFrameSize<<" and frame count: "<<mFrameCount<<std::endl;

        mFramebuffer = new uint8_t[mFrameSize];

        pthread_create(&mMsgSocketThread, NULL, &VCamDevice::InitMsgSocket, this);
        pthread_create(&mThread, NULL, &VCamDevice::EmitFrame, this);
        pthread_create(&mReceiveThread, NULL, &VCamDevice::ReceiveFrame, this);
//        pthread_create(&mSendThread, NULL, &VCamDevice::SendFrame, this);
        pthread_create(&mMsgThread, NULL, &VCamDevice::SendMessage, this);
//        pthread_create(&mProceedThread, NULL, &VCamDevice::ProceedFrame, this);
        
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
        std::cout << "Create stream " << type << std::endl;
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
        std::cout << "Emit frame" << vcamDevice->mFrameSize << std::endl;
//        uint8_t* framebuffer = new uint8_t[vcamDevice->mFrameSize];

//        while (true) {
//            usleep(1000 * 1000 / 60);
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

//Send Frame from camera to socket
//void* VCamDevice::SendFrame(void* device) {
//    //Create queue to receive frame
//    queue_t *q = queue_create();
//        usleep(1000 * 1000 / 60);
//        VCamDevice* vcamDevice = (VCamDevice*)device;
//        //        uint8_t* framebuffer = new uint8_t[vcamDevice->mFrameSize];
//        std::cout << "Send frame from camera " << std::endl;
//        std::freopen(nullptr, "rb", stdin);
//        if(std::ferror(stdin))
//            throw std::runtime_error(std::strerror(errno));
//        std::size_t len;
//        //    std::array<char, INIT_BUFFER_SIZE> buf;
//
//        int frameSize = vcamDevice->mFrameSize;
//        int frameIndex = 0;
//        // somewhere to store the data
//        //    std::vector<char> input;
//        // ソケット作成
//        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
//        if (sock == -1) {
//            std::cerr << "socket" << std::endl;
//            return NULL;
//        }
//        // struct sockaddr_un 作成
//        struct sockaddr_un sa = {0};
//        sa.sun_family = AF_UNIX;
//        strcpy(sa.sun_path, "/tmp/vcam-socket");
//
//        try {
//            // 接続
//            int connectResult = connect(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un));
//            while(connectResult == -1){
//                std::cout<<"Connect to socket failed. Try again."<<std::endl;
//                connectResult = connect(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un));
//            }
//            uint8_t* framebuffer = new uint8_t[frameSize];
//            uint8_t* receiveframebuffer = new uint8_t[frameSize];
//            while( true )
//            {
//                usleep(1000 * 1000 / 60);
//                // whoopsie
//                len = std::fread(framebuffer, sizeof(framebuffer[0]), frameSize, stdin);
//                if(std::ferror(stdin) && !std::feof(stdin)){
//                    delete[] framebuffer;
//                    framebuffer = NULL;
//                    delete[] receiveframebuffer;
//                    receiveframebuffer = NULL;
//                    throw std::runtime_error(std::strerror(errno));
//                }
//
//                queue_put(q, framebuffer);
//                if(len > 0){
//                    ++frameIndex;
//                    std::cout << "frame Index" << frameIndex << std::endl;
//                    // 送信
//                    queue_get(q, (void **)&receiveframebuffer);
//                    if (write(sock, receiveframebuffer, frameSize) == -1) {
//                        delete[] framebuffer;
//                        framebuffer = NULL;
//                        delete[] receiveframebuffer;
//                        receiveframebuffer = NULL;
//                        throw std::runtime_error("write");
//                    }
//                }
//            }
//        } catch (const std::exception& e) {
//            std::cerr << e.what();
//            queue_destroy(q);
//            close(sock);
//            return NULL;
//        }
//}

//Send Frame from file to socket
//    void* VCamDevice::SendFrame(void* device) {
//        usleep(1000 * 1000 / 60);
//        VCamDevice* vcamDevice = (VCamDevice*)device;
//        //        uint8_t* framebuffer = new uint8_t[vcamDevice->mFrameSize];
//                std::cout << "Send frame from file size " << vcamDevice->mFrameSize << std::endl;
//                auto sequenceFile = fopen("/Library/CoreMediaIO/Plug-Ins/DAL/QuickomCamera.plugin/Contents/Resources/ntsc2vuy720x480.yuv", "rb");
//        int frameSize = vcamDevice->mFrameSize;
//        ////        uint8_t* framebuffer = new uint8_t[frameSize];
//        //
//                fseek(sequenceFile, 0, SEEK_END);
//                int frameCount = ftell(sequenceFile) / frameSize;
//
//                int frameIndex = 0;
//
//                // ソケット作成
//                int sock = socket(AF_UNIX, SOCK_STREAM, 0);
//                if (sock == -1) {
//                    std::cerr << "socket" << std::endl;
//                    return NULL;
//                }
//        // struct sockaddr_un 作成
//                    struct sockaddr_un sa = {0};
//                    sa.sun_family = AF_UNIX;
//                    strcpy(sa.sun_path, "/tmp/vcam-socket");
//                    try {
//                            // 接続
//                        int connectResult = connect(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un));
//                        while(connectResult == -1){
//                            std::cout<<"Connect to socket failed. Try again."<<std::endl;
//                            connectResult = connect(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un));
//                        }
////                            if (connect(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un)) == -1) {
////                                throw std::runtime_error("connect failed");
////                            }
//                        std::cout<<"Connect to socket. Start streaming."<<std::endl;
//
//                            uint8_t* framebuffer = new uint8_t[frameSize];
//
//                            while (true) {
//                                usleep(1000 * 1000 / 60);
//
//                                fseek(sequenceFile, (frameIndex % frameCount) * frameSize, SEEK_SET);
//                                fread(framebuffer, 1, frameSize, sequenceFile);
//                                ++frameIndex;
//                                std::cout << "frame Index" << frameIndex << std::endl;
//                                // 送信
//                                if (write(sock, framebuffer, frameSize) == -1) {
//                                    throw std::runtime_error("write");
//                                }
//                            }
//                        } catch (const std::exception& e) {
//                            std::cerr << e.what();
//                            close(sock);
//                            return NULL;
//                        }
//    }


//Send Frame from Go
void* VCamDevice::SendFrame(void* device) {
    usleep(1000 * 1000 / 60);
    VCamDevice* vcamDevice = (VCamDevice*)device;
    std::cout<<"Starting read cam framesize: "<<vcamDevice->mFrameSize<<std::endl;
    GoInt width = 1280;
    GoInt height = 720;
    GoInt deviceId = 0;
        switch (vcamDevice->mFrameSize){
            case 720*480*2:
                width = 720;
                height = 480;
                break;
            case 1280*720*2:
                width = 1280;
                height = 720;
                break;
            case 1920*1080*2:
                width = 1920;
                height = 1080;
                break;
            default:
                break;
        }

        std::string s = "";
        char *cstr = new char[s.length()+1];
        std::strcpy (cstr, s.c_str());
        std::string sk = "/tmp/vcam-socket";
        char *cstr1 = new char[sk.length()+1];
        std::strcpy (cstr1, sk.c_str());
        std::cout<<"Width - height: "<<width<<" - "<<height<<std::endl;
        ReadCamCV(deviceId, width, height, cstr, cstr1);
    //    ReadCam(width, height, cstr, cstr1);
    //    queue_t *q = queue_create();
    //    ReadCam(width, height, cstr, q);
    //    queue_get(q, (void **)&vcamDevice->mFramebuffer);
}

//Send Frame from OpenCV
//void* VCamDevice::SendFrame(void* device) {
//    std::cout<<"Start get frame from camera open cv";
//    GetVideo();
//}
    
void* VCamDevice::SendMessage(void * device){
    // ソケット作成
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "socket" << std::endl;
        return NULL;
    }
            // struct sockaddr_un 作成
    struct sockaddr_un sa = {0};
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, "/tmp/message-socket");
    try {
        // 接続
        int connectResult = connect(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un));
        while(connectResult == -1){
            std::cout<<"Connect to socket failed. Try again."<<std::endl;
            connectResult = connect(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un));
        }
        std::cout<<"Connect to socket. Start sending msg."<<std::endl;
    
        while(1){
            std::string sk = "{\"deviceId\":0}";
            char *cstr = new char[sk.length()+1];
            std::strcpy (cstr, sk.c_str());
            
            std::cout<<"Write device to socket:"<<sk<<std::endl;
            if (write(sock, cstr, sk.length()+1) == -1) {
                throw std::runtime_error("write");
            }
            std::cout << "Sleeping for 30 seconds ..." << std::endl;
            sleep(30);
            
            std::string sk1 = "{\"deviceId\":1}";
            char *cstr1 = new char[sk1.length()+1];
            std::strcpy (cstr1, sk1.c_str());
            
            std::cout<<"Write device to socket:"<<sk1<<std::endl;
            if (write(sock, cstr1, sk1.length()+1) == -1) {
                throw std::runtime_error("write");
            }
            std::cout << "Sleeping for 30 seconds ..." << std::endl;
            sleep(30);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what();
        close(sock);
        return NULL;
    }
}

void* VCamDevice::InitMsgSocket(void * device){
    std::cout << "Init message socket" << std::endl;
    VCamDevice* vcamDevice = (VCamDevice*)device;
    // サーバーソケット作成
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return NULL;
    }
    // Set socket buffer size
    int bufferSize = 14;// 1024 * 1024;
    socklen_t bufferSizeLen = sizeof(bufferSize);
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&bufferSize), bufferSizeLen) == -1) {
        perror("setsockopt error");
    }

    // struct sockaddr_un 作成
    struct sockaddr_un sa = {0};
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, "/tmp/message-socket");
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
//        uint8_t* framebuffer = new uint8_t[bufferSize];
        while (true) {
            // 受信
            uint8_t* framebuffer = new uint8_t[bufferSize];
            int recv_size = read(fd, framebuffer, bufferSize);
//            std::cout << "Received size:" << recv_size << std::endl;
            if (recv_size == -1)
            {
                perror("read");
                close(fd);
                goto bail;
            }
            if (recv_size == 0)
            {
                totalReceived = 0;
                break;
            }
            totalReceived += recv_size;
//            std::cout << "Total received size:" << totalReceived << std::endl;

            if (totalReceived == bufferSize) {
                // frame complete
                totalReceived = 0;
                std::string str = (char*)framebuffer;
                if (str.size() == bufferSize){
                    std::cout << "Receive message:" << str << std::endl;
                    //Analyze received message
                    size_t index1 = str.find(":");
                    std::string str1 = str.substr(index1 + 1);
                    size_t length = str1.size() - 1;
                    std::string str2 = str1.substr(0, length);
                    int deId = stoi(str2);
                    GoInt width = 1280;
                    GoInt height = 720;
                    GoInt deviceId = deId;
                        switch (vcamDevice->mFrameSize){
                            case 720*480*2:
                                width = 720;
                                height = 480;
                                break;
                            case 1280*720*2:
                                width = 1280;
                                height = 720;
                                break;
                            case 1920*1080*2:
                                width = 1920;
                                height = 1080;
                                break;
                            default:
                                break;
                        }

                        std::string s = "";
                        char *cstr = new char[s.length()+1];
                        std::strcpy (cstr, s.c_str());
                        std::string sk = "/tmp/vcam-socket";
                        char *cstr1 = new char[sk.length()+1];
                        std::strcpy (cstr1, sk.c_str());
                    std::cout << "Start reading cam from device Id:" << deId << std::endl;
                        std::cout<<"Width - height: "<<width<<" - "<<height<<std::endl;
                        ReadCamCV(deviceId, width, height, cstr, cstr1);
                }
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

    void* VCamDevice::ReceiveFrame(void* device) {
        std::cout << "Receive frame and init socket server" << std::endl;
        VCamDevice* vcamDevice = (VCamDevice*)device;

        // サーバーソケット作成
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("socket");
            return NULL;
        }
        // Set socket buffer size
        int bufferSize = vcamDevice->mFrameSize; //1024 * 1024;
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
//                    std::cout << "Receive frame:" << vcamDevice->mFrameCount << std::endl;
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

void* VCamDevice::ProceedFrame(void* device) {
    std::cout << "Proceed both send and receive frame without socket server" << std::endl;
    VCamDevice* vcamDevice = (VCamDevice*)device;

    auto sequenceFile = fopen("/Library/CoreMediaIO/Plug-Ins/DAL/QuickomCamera.plugin/Contents/Resources/ntsc2vuy720x480.yuv", "rb");
    int frameSize = vcamDevice->mFrameSize;
    fseek(sequenceFile, 0, SEEK_END);
    int frameCount = ftell(sequenceFile) / frameSize;

    int frameIndex = 0;
    
    while (1) {
        // クライアントの接続を待つ
//        int fd = open("/Library/CoreMediaIO/Plug-Ins/DAL/QuickomCamera.plugin/Contents/Resources/ntsc2vuy720x480.yuv", O_RDONLY);
//        if (fd == -1) {
//            perror("accept");
//            goto bail;
//        }

        int totalReceived = 0;

        while (true) {
            usleep(1000 * 1000 / 60);
            fseek(sequenceFile, (frameIndex % frameCount) * frameSize, SEEK_SET);
            int recv_size = fread(vcamDevice->mFramebuffer + totalReceived, 1, frameSize, sequenceFile);
            ++frameIndex;
            // 受信
//            int recv_size = read(fd, vcamDevice->mFramebuffer + totalReceived, vcamDevice->mFrameSize - totalReceived);
            std::cout<<"Received size: "<< recv_size<<std::endl;
            if (recv_size == -1)
            {
                perror("read");
//                close(fd);
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
                std::cout << "Receive frame:" << vcamDevice->mFrameCount << std::endl;
            }
        }

        // ソケットのクローズ
//        if (close(fd) == -1) {
//            perror("close");
//            goto bail;
//        }
    }
//    uint8_t* framebuffer = new uint8_t[frameSize];
//    int totalReceived = 0;
//    while (true) {
//        usleep(1000 * 1000 / 60);
//
////        fseek(sequenceFile, (frameIndex % frameCount) * frameSize, SEEK_SET);
////        fread(vcamDevice->mFramebuffer, 1, frameSize, sequenceFile);
////        ++frameIndex;
////        std::cout << "frame Index" << frameIndex << std::endl;
////        ++vcamDevice->mFrameCount;
////        std::cout << "Receive frame:" << vcamDevice->mFrameCount << std::endl;
//
//        fseek(sequenceFile, (frameIndex % frameCount) * frameSize, SEEK_SET);
//        int recv_size = fread(vcamDevice->mFramebuffer + totalReceived, 1, vcamDevice->mFrameSize - totalReceived, sequenceFile);
//        if (recv_size == -1)
//        {
//            perror("read");
//            fclose(sequenceFile);
//            goto bail;
//        }
//        if (recv_size == 0) {
//            // disconnected
//            totalReceived = 0;
//            break;
//        }
//        //std::cout << recv_size << std::endl;
//        totalReceived += recv_size;
//
//        if (totalReceived == vcamDevice->mFrameSize) {
//            // frame complete
//            totalReceived = 0;
//            ++vcamDevice->mFrameCount;
//            std::cout << "Receive frame:" << vcamDevice->mFrameCount << std::endl;
//        }
//    }
    bail:
        return NULL;
    }

}}}}
