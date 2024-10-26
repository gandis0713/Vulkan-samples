#pragma once

#include "fps.h"
#include "hpc_watcher.h"
#include "im_gui.h"
#include "window.h"

#include <deque>
#include <filesystem>
#include <optional>
#include <unordered_set>

#include <jipu/native/include/device.h>
#include <jipu/native/include/instance.h>
#include <jipu/native/include/physical_device.h>
#include <jipu/native/include/queue.h>
#include <jipu/native/include/surface.h>
#include <jipu/native/include/swapchain.h>

#include "hpc/instance.h"

namespace jipu
{

struct SampleDescriptor
{
    WindowDescriptor windowDescriptor;
    std::filesystem::path path;
};

class Sample : public Window
{
public:
    Sample() = delete;
    Sample(const SampleDescriptor& descriptor);
    virtual ~Sample();

public:
    virtual void createInstance();
    virtual void getPhysicalDevices();
    virtual void createSurface();
    virtual void createDevice();
    virtual void createSwapchain();
    virtual void createQueue();

public:
    void init() override;
    void update() override;

public:
    void recordImGui(std::vector<std::function<void()>> cmds);
    void windowImGui(const char* title, std::vector<std::function<void()>> uis);
    void drawImGui(CommandEncoder* commandEncoder, TextureView& renderView);

public:
    void onHPCListner(Values values);

protected:
    std::filesystem::path m_appPath;
    std::filesystem::path m_appDir;

    std::unique_ptr<Instance> m_instance = nullptr;
    std::vector<std::unique_ptr<PhysicalDevice>> m_physicalDevices{};
    std::unique_ptr<Device> m_device = nullptr;
    std::unique_ptr<Surface> m_surface = nullptr;
    std::unique_ptr<Queue> m_queue = nullptr;
    std::unique_ptr<Swapchain> m_swapchain = nullptr;
    std::unique_ptr<CommandEncoder> m_commandEncoder = nullptr;
    TextureView* m_renderView = nullptr;

protected:
    std::optional<Im_Gui> m_imgui = std::nullopt;

protected:
    std::unique_ptr<HPCWatcher> m_hpcWatcher = nullptr;
    std::unique_ptr<hpc::Instance> m_hpcInstance = nullptr;

protected:
    void createHPCWatcher(const std::unordered_set<hpc::Counter>& counters = {});
    void drawPolyline(std::string title, std::deque<float> data, std::string unit = "");
    void profilingWindow();

private:
    FPS m_fps{};
    std::unordered_map<hpc::Counter, std::deque<float>> m_profiling{};
};

} // namespace jipu
