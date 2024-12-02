#include "wgpu_rotating_cube.h"

#include "file.h"
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

namespace jipu
{

WGPURotatingCube::WGPURotatingCube(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
}

WGPURotatingCube::~WGPURotatingCube()
{
    finalizeContext();
}

void WGPURotatingCube::init()
{
    WGPUSample::init();

    changeAPI(APIType::kJipu);
    // changeAPI(APIType::kDawn);
}

void WGPURotatingCube::onUpdate()
{
    WGPUSample::onUpdate();

    auto getTransformationMatrix = [&]() -> glm::mat4 {
        // Projection matrix (예제의 투영 행렬, 필요에 따라 정의 가능)
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);

        // View matrix 초기화
        glm::mat4 viewMatrix = glm::mat4(1.0f); // Identity matrix

        // View matrix에 translation 적용
        viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, -4.0f));

        // 현재 시간 (초 단위)
        auto now = std::chrono::high_resolution_clock::now();
        auto timeSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        float currentTime = static_cast<float>(timeSinceEpoch) / 1000.0f;

        // View matrix에 rotation 적용
        glm::vec3 rotationAxis(std::sin(currentTime), std::cos(currentTime), 0.0f);
        viewMatrix = glm::rotate(viewMatrix, 1.0f, rotationAxis);

        // Model-View-Projection matrix 계산
        glm::mat4 modelViewProjectionMatrix = projectionMatrix * viewMatrix;

        return modelViewProjectionMatrix;
    };

    auto transformationMatrix = getTransformationMatrix();
    wgpu.QueueWriteBuffer(m_queue, m_uniformBuffer, 0, &transformationMatrix, sizeof(glm::mat4));
}

void WGPURotatingCube::onDraw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpu.SurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu.TextureCreateView(surfaceTexture.texture, NULL);
    WGPUTextureView depthTextureView = wgpu.TextureCreateView(m_depthTexture, NULL);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu.DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = surfaceTextureView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };

    WGPURenderPassDepthStencilAttachment depthStencilAttachment{};
    depthStencilAttachment.view = depthTextureView;
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.depthClearValue = 1.0f;

    WGPURenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;
    renderPassDescriptor.depthStencilAttachment = &depthStencilAttachment;

    WGPURenderPassEncoder renderPassEncoder = wgpu.CommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

    wgpu.RenderPassEncoderSetPipeline(renderPassEncoder, m_renderPipeline);
    wgpu.RenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f);
    wgpu.RenderPassEncoderSetScissorRect(renderPassEncoder, 0, 0, m_width, m_height);
    wgpu.RenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, m_cubeVertexBuffer, 0, m_cube.size() * sizeof(float));
    // wgpu.RenderPassEncoderSetIndexBuffer(renderPassEncoder, m_cubeIndexBuffer, WGPUIndexFormat_Uint16, 0, m_indices.size() * sizeof(IndexType));
    // wgpu.RenderPassEncoderDrawIndexed(renderPassEncoder, 3, 1, 0, 0, 0);
    wgpu.RenderPassEncoderDraw(renderPassEncoder, 36, 1, 0, 0);
    wgpu.RenderPassEncoderEnd(renderPassEncoder);
    wgpu.RenderPassEncoderRelease(renderPassEncoder);

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpu.CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpu.QueueSubmit(m_queue, 1, &commandBuffer);
    wgpu.SurfacePresent(m_surface);

    wgpu.CommandBufferRelease(commandBuffer);
    wgpu.CommandEncoderRelease(commandEncoder);
    wgpu.TextureViewRelease(surfaceTextureView);
    wgpu.TextureRelease(surfaceTexture.texture);
}

void WGPURotatingCube::initializeContext()
{
    WGPUSample::initializeContext();

    createCubeBuffer();
    createDepthTexture();
    createUniformBuffer();
    createBindingGroupLayout();
    createBindingGroup();
    createShaderModule();
    createPipelineLayout();
    createPipeline();
}

void WGPURotatingCube::finalizeContext()
{
    // TODO: check ways release and destory.
    if (m_cubeVertexBuffer)
    {
        wgpu.BufferRelease(m_cubeVertexBuffer);
        m_cubeVertexBuffer = nullptr;
    }

    // if (m_cubeIndexBuffer)
    // {
    //     wgpu.BufferRelease(m_cubeIndexBuffer);
    //     m_cubeIndexBuffer = nullptr;
    // }

    if (m_uniformBuffer)
    {
        wgpu.BufferRelease(m_uniformBuffer);
        m_uniformBuffer = nullptr;
    }

    if (m_depthTexture)
    {
        wgpu.TextureRelease(m_depthTexture);
        m_depthTexture = nullptr;
    }

    // if (m_bindGroup)
    // {
    //     wgpu.BindGroupRelease(m_bindGroup);
    //     m_bindGroup = nullptr;
    // }

    // if (m_bindGroupLayout)
    // {
    //     wgpu.BindGroupLayoutRelease(m_bindGroupLayout);
    //     m_bindGroupLayout = nullptr;
    // }

    if (m_renderPipeline)
    {
        wgpu.RenderPipelineRelease(m_renderPipeline);
        m_renderPipeline = nullptr;
    }

    if (m_pipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_pipelineLayout);
        m_pipelineLayout = nullptr;
    }

    if (m_vertWGSLShaderModule)
    {
        wgpu.ShaderModuleRelease(m_vertWGSLShaderModule);
        m_vertWGSLShaderModule = nullptr;
    }

    if (m_fragWGSLShaderModule)
    {
        wgpu.ShaderModuleRelease(m_fragWGSLShaderModule);
        m_fragWGSLShaderModule = nullptr;
    }

    WGPUSample::finalizeContext();
}

void WGPURotatingCube::createCubeBuffer()
{
    {
        size_t vertexBufferSize = m_cube.size() * sizeof(float);
        WGPUBufferDescriptor bufferDescriptor{};
        bufferDescriptor.size = vertexBufferSize;
        bufferDescriptor.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        // bufferDescriptor.mappedAtCreation = true;

        m_cubeVertexBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
        assert(m_cubeVertexBuffer);
        // void* mappedVertexPtr = wgpu.BufferGetMappedRange(m_cubeVertexBuffer, 0, vertexBufferSize);
        // memcpy(mappedVertexPtr, m_cube.data(), vertexBufferSize);
        // wgpu.BufferUnmap(m_cubeVertexBuffer);

        wgpu.QueueWriteBuffer(m_queue, m_cubeVertexBuffer, 0, m_cube.data(), vertexBufferSize);
    }

    // {
    //     size_t indexBufferSize = m_indices.size() * sizeof(IndexType);
    //     WGPUBufferDescriptor bufferDescriptor{};
    //     bufferDescriptor.size = indexBufferSize;
    //     bufferDescriptor.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
    //     // bufferDescriptor.mappedAtCreation = true;

    //     m_cubeIndexBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
    //     assert(m_cubeIndexBuffer);

    //     // void* mappedIndexPtr = wgpu.BufferGetMappedRange(m_cubeIndexBuffer, 0, indexBufferSize);
    //     // memcpy(mappedIndexPtr, m_indices.data(), indexBufferSize);
    //     // wgpu.BufferUnmap(m_cubeIndexBuffer);

    //     wgpu.QueueWriteBuffer(m_queue, m_cubeIndexBuffer, 0, m_indices.data(), indexBufferSize);
    // }
}

void WGPURotatingCube::createDepthTexture()
{
    WGPUTextureDescriptor descriptor{};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size.width = m_width;
    descriptor.size.height = m_height;
    descriptor.size.depthOrArrayLayers = 1;
    descriptor.sampleCount = 1;
    descriptor.format = WGPUTextureFormat_Depth24Plus;
    descriptor.mipLevelCount = 1;
    descriptor.usage = WGPUTextureUsage_RenderAttachment;

    m_depthTexture = wgpu.DeviceCreateTexture(m_device, &descriptor);
    assert(m_depthTexture);
}

void WGPURotatingCube::createUniformBuffer()
{
    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = sizeof(glm::mat4);
    bufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    m_uniformBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
    assert(m_uniformBuffer);
}
void WGPURotatingCube::createBindingGroupLayout()
{
    WGPUBindGroupLayoutEntry bindGroupLayoutEntries[1] = {
        { .binding = 0, .visibility = WGPUShaderStage_Vertex, .buffer = { .type = WGPUBufferBindingType_Uniform } },
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = 1;
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries;

    m_bindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_bindGroupLayout);
}

void WGPURotatingCube::createBindingGroup()
{
    WGPUBindGroupEntry bindGroupEntries[1] = {
        { .binding = 0, .buffer = m_uniformBuffer, .offset = 0, .size = sizeof(glm::mat4) },
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_bindGroupLayout;
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = bindGroupEntries;

    m_bindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_bindGroup);
}

void WGPURotatingCube::createShaderModule()
{
    std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "rotating_cube.vert.wgsl", m_handle);
    std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "rotating_cube.frag.wgsl", m_handle);

    std::string vertexShaderCode(vertexShaderSource.begin(), vertexShaderSource.end());
    std::string fragmentShaderCode(fragmentShaderSource.begin(), fragmentShaderSource.end());

    WGPUShaderModuleWGSLDescriptor vertexShaderModuleWGSLDescriptor{};
    vertexShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    vertexShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = vertexShaderCode.data(), .length = vertexShaderCode.size() };

    WGPUShaderModuleDescriptor vertexShaderModuleDescriptor{};
    vertexShaderModuleDescriptor.nextInChain = &vertexShaderModuleWGSLDescriptor.chain;

    m_vertWGSLShaderModule = wgpu.DeviceCreateShaderModule(m_device, &vertexShaderModuleDescriptor);

    assert(m_vertWGSLShaderModule);

    WGPUShaderModuleWGSLDescriptor fragShaderModuleWGSLDescriptor{};
    fragShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    fragShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = fragmentShaderCode.data(), .length = fragmentShaderCode.size() };

    WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
    fragShaderModuleDescriptor.nextInChain = &fragShaderModuleWGSLDescriptor.chain;

    m_fragWGSLShaderModule = wgpu.DeviceCreateShaderModule(m_device, &fragShaderModuleDescriptor);

    assert(m_fragWGSLShaderModule);
}

void WGPURotatingCube::createPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_bindGroupLayout;

    m_pipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);
    assert(m_pipelineLayout);
}

void WGPURotatingCube::createPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_Back;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

    std::vector<WGPUVertexAttribute> attributes{};
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x4;
        attribute.offset = 0;
        attribute.shaderLocation = 0;

        attributes.push_back(attribute);
    }
    // {
    //     WGPUVertexAttribute attribute{};
    //     attribute.format = WGPUVertexFormat_Float32x4;
    //     attribute.offset = sizeof(float) * 4;
    //     attribute.shaderLocation = 0;

    //     attributes.push_back(attribute);
    // }
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x2;
        attribute.offset = sizeof(float) * 8;
        attribute.shaderLocation = 1;

        attributes.push_back(attribute);
    }

    std::vector<WGPUVertexBufferLayout> vertexBufferLayout(1);
    vertexBufferLayout[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout[0].attributes = attributes.data();
    vertexBufferLayout[0].attributeCount = static_cast<uint32_t>(attributes.size());
    vertexBufferLayout[0].arrayStride = sizeof(float) * 10;

    std::string entryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    vertexState.module = m_vertWGSLShaderModule;
    vertexState.bufferCount = static_cast<uint32_t>(vertexBufferLayout.size());
    vertexState.buffers = vertexBufferLayout.data();

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceCapabilities.formats[0];
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState{};
    fragState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    fragState.module = m_fragWGSLShaderModule;
    fragState.targetCount = 1;
    fragState.targets = &colorTargetState;

    WGPUDepthStencilState depthStencilState{};
    depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
    depthStencilState.depthCompare = WGPUCompareFunction_Less;
    depthStencilState.format = WGPUTextureFormat_Depth24Plus;

    WGPUMultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;

    WGPURenderPipelineDescriptor renderPipelineDescriptor{};
    renderPipelineDescriptor.layout = m_pipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.multisample = multisampleState;
    renderPipelineDescriptor.depthStencil = &depthStencilState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragState;

    m_renderPipeline = wgpu.DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);

    assert(m_renderPipeline);
}

} // namespace jipu