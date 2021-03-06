#include "pch.h"

#include "Codec/EncoderFactory.h"
#include "Context.h"
#include "IUnityGraphics.h"
#include "GraphicsDevice/GraphicsDevice.h"

enum class VideoStreamRenderEventID
{
    Initialize = 0,
    Encode = 1,
    Finalize = 2
};

namespace unity
{
namespace webrtc
{

    IUnityInterfaces* s_UnityInterfaces = nullptr;
    IUnityGraphics* s_Graphics = nullptr;
    Context* s_context = nullptr;
    IGraphicsDevice* s_device;

} // end namespace webrtc
} // end namespace unity

using namespace unity::webrtc;

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    switch (eventType)
    {
    case kUnityGfxDeviceEventInitialize:
    {
        break;
    }
    case kUnityGfxDeviceEventShutdown:
    {
        //UnityPluginUnload not called normally
        s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
        break;
    }
    case kUnityGfxDeviceEventBeforeReset:
    {
        break;
    }
    case kUnityGfxDeviceEventAfterReset:
    {
        break;
    }
    };
}
// Unity plugin load event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    s_UnityInterfaces = unityInterfaces;
    s_Graphics = unityInterfaces->Get<IUnityGraphics>();
    s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

std::map<const ::webrtc::MediaStreamTrackInterface*, std::unique_ptr<IEncoder>> m_mapEncoder;

static void UNITY_INTERFACE_API OnRenderEvent(int eventID, void* data)
{
    if (s_context == nullptr)
    {
        return;
    }
    const auto track = reinterpret_cast<::webrtc::MediaStreamTrackInterface*>(data);
    const auto event = static_cast<VideoStreamRenderEventID>(eventID);

    switch(event)
    {
        case VideoStreamRenderEventID::Initialize:
        {
            if (!GraphicsDevice::GetInstance().IsInitialized())
            {
                GraphicsDevice::GetInstance().Init(s_UnityInterfaces);
            }
            s_device = GraphicsDevice::GetInstance().GetDevice();
            const VideoEncoderParameter* param = s_context->GetEncoderParameter(track);
            const UnityEncoderType encoderType = s_context->GetEncoderType();
            m_mapEncoder[track] = EncoderFactory::GetInstance().Init(param->width, param->height, s_device, encoderType);
            s_context->InitializeEncoder(m_mapEncoder[track].get(), track);
            return;
        }
        case VideoStreamRenderEventID::Encode:
        {
            if(!s_context->EncodeFrame(track))
            {
                LogPrint("Encode frame failed");
            }
            return;
        }
        case VideoStreamRenderEventID::Finalize:
        {
            m_mapEncoder.erase(track);
            GraphicsDevice::GetInstance().Shutdown();
            return;
        }
        default: {
            LogPrint("Unknown event id %d", eventID);
            return;
        }
    }
}

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc(Context* context)
{
    s_context = context;
    return OnRenderEvent;
}
