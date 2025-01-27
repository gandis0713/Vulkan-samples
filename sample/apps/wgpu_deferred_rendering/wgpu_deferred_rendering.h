#include "wgpu_sample.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace jipu
{

class WGPUDeferredRenderingSample : public WGPUSample
{
public:
    WGPUDeferredRenderingSample() = delete;
    WGPUDeferredRenderingSample(const WGPUSampleDescriptor& descriptor);
    ~WGPUDeferredRenderingSample() override;

    void init() override;
    void onBeforeUpdate() override;
    void onUpdate() override;
    void onDraw() override;

    void initializeContext() override;
    void finalizeContext() override;

private:
private:
    int m_mode = 0;
    int m_numLights = 24;
};

} // namespace jipu