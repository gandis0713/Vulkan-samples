#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/webgpu/webgpu_header.h"

#include "jipu/native/instance.h"

#include "jipu/webgpu/event/event_manager.h"

namespace jipu
{

class WebGPUSurface;
class WebGPUInstance : public RefCounted
{
public:
    static WebGPUInstance* create(WGPUInstanceDescriptor const* wgpuDescriptor);

public:
    virtual ~WebGPUInstance() = default;

    WebGPUInstance(const WebGPUInstance&) = delete;
    WebGPUInstance& operator=(const WebGPUInstance&) = delete;

public: // WebGPU API
    WGPUWaitStatus waitAny(const uint64_t waitCount, WGPUFutureWaitInfo* waitInfos, uint64_t timeoutNS);
    void processEvents();
    WGPUFuture requestAdapter(WGPURequestAdapterOptions const* options, WGPURequestAdapterCallbackInfo2 callbackInfo);
    WebGPUSurface* createSurface(WGPUSurfaceDescriptor const* descriptor);

public:
    Instance* getInstance() const;
    EventManager* getEventManager() const;

private:
    [[maybe_unused]] const WGPUInstanceDescriptor m_wgpuDescriptor{};

private:
    std::unique_ptr<Instance> m_instance = nullptr;
    std::unique_ptr<EventManager> m_eventManager = nullptr;

private:
    explicit WebGPUInstance(std::unique_ptr<Instance> instance, const WGPUInstanceDescriptor& wgpuDescriptor);
};

} // namespace jipu