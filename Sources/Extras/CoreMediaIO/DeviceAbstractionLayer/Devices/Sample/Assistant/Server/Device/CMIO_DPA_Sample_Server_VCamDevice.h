/*
	    File: CMIO_DPA_Sample_Server_VCamDevice.h
	Abstract: n/a
	 Version: 1.2
	
*/

#if !defined(__CMIO_DPA_Sample_Server_VCamDevice_h__)
#define __CMIO_DPA_Sample_Server_VCamDevice_h__

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//	Includes
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "CMIO_DPA_Sample_Server_Device.h"

namespace CMIO { namespace DPA { namespace Sample { namespace Server
{
    class VCamInputStream;
    struct ThreadArgs {
        void* vCamDevice;
        int deviceId;
        int times;
    };
    
    class VCamDevice: public Device
	{
	public:

    #pragma mark -
	#pragma mark Device
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Device
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Construction/Destruction
	public:
									VCamDevice(int type);
		virtual						~VCamDevice();
	
	private:
		VCamDevice&					operator=(VCamDevice& that); // Don't allow copying
        
    private:
        VCamInputStream*            mInputStream;
        void						CreateStreams(int type);
        
    private:
        pthread_t                   mThread;
        pthread_t                   mReceiveThread;
        pthread_t                   mSendThread;
        pthread_t                   mMsgSocketThread;
        pthread_t                   mProceedThread;
        pthread_t                   mMsgThread;
        FILE*                       mSequenceFile;
        size_t                      mFrameIndex;
        size_t                      mFrameSize;
        size_t                      mFrameCount;
        uint8_t* mFramebuffer;
        
        static void*                EmitFrame(void*);
        static void*                ReceiveFrame(void*);
        static void*                SendFrame(void*);
        static void*                ProceedFrame(void*);
        static void*                InitMsgSocket(void*);
        static void*                SendMessage(void*);
	};
}}}}
#endif
