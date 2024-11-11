#include "wgpu_rotating_cube.h"

#include "file.h"
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

    // changeAPI(APIType::kJipu);
    changeAPI(APIType::kDawn);
}

void WGPURotatingCube::update()
{
    WGPUSample::update();
}

void WGPURotatingCube::draw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpu.SurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu.TextureCreateView(surfaceTexture.texture, NULL);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu.DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = surfaceTextureView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };

    WGPURenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;

    WGPURenderPassEncoder renderPassEncoder = wgpu.CommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

    wgpu.RenderPassEncoderSetPipeline(renderPassEncoder, m_renderPipeline);
    wgpu.RenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f);
    wgpu.RenderPassEncoderSetScissorRect(renderPassEncoder, 0, 0, m_width, m_height);
    wgpu.RenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, m_cubeVertexBuffer, 0, m_vertices.size() * sizeof(Vertex));
    wgpu.RenderPassEncoderSetIndexBuffer(renderPassEncoder, m_cubeIndexBuffer, WGPUIndexFormat_Uint16, 0, m_indices.size() * sizeof(IndexType));
    wgpu.RenderPassEncoderDrawIndexed(renderPassEncoder, 3, 1, 0, 0, 0);
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

    if (m_cubeIndexBuffer)
    {
        wgpu.BufferRelease(m_cubeIndexBuffer);
        m_cubeIndexBuffer = nullptr;
    }

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
    const uint32_t cubeVertexSize = 4 * 10; // Byte size of one cube vertex.
    const uint32_t cubePositionOffset = 0;
    const uint32_t cubeColorOffset = 4 * 4; // Byte offset of cube vertex color attribute.
    const uint32_t cubeUVOffset = 4 * 8;
    const uint32_t cubeVertexCount = 36;

    // clang-format off
    const std::vector<float> cube{
        // float4 position, float4 color, float2 uv,
        1, -1, 1, 1,   1, 0, 1, 1,  0, 1,
        -1, -1, 1, 1,  0, 0, 1, 1,  1, 1,
        -1, -1, -1, 1, 0, 0, 0, 1,  1, 0,
        1, -1, -1, 1,  1, 0, 0, 1,  0, 0,
        1, -1, 1, 1,   1, 0, 1, 1,  0, 1,
        -1, -1, -1, 1, 0, 0, 0, 1,  1, 0,

        1, 1, 1, 1,    1, 1, 1, 1,  0, 1,
        1, -1, 1, 1,   1, 0, 1, 1,  1, 1,
        1, -1, -1, 1,  1, 0, 0, 1,  1, 0,
        1, 1, -1, 1,   1, 1, 0, 1,  0, 0,
        1, 1, 1, 1,    1, 1, 1, 1,  0, 1,
        1, -1, -1, 1,  1, 0, 0, 1,  1, 0,

        -1, 1, 1, 1,   0, 1, 1, 1,  0, 1,
        1, 1, 1, 1,    1, 1, 1, 1,  1, 1,
        1, 1, -1, 1,   1, 1, 0, 1,  1, 0,
        -1, 1, -1, 1,  0, 1, 0, 1,  0, 0,
        -1, 1, 1, 1,   0, 1, 1, 1,  0, 1,
        1, 1, -1, 1,   1, 1, 0, 1,  1, 0,

        -1, -1, 1, 1,  0, 0, 1, 1,  0, 1,
        -1, 1, 1, 1,   0, 1, 1, 1,  1, 1,
        -1, 1, -1, 1,  0, 1, 0, 1,  1, 0,
        -1, -1, -1, 1, 0, 0, 0, 1,  0, 0,
        -1, -1, 1, 1,  0, 0, 1, 1,  0, 1,
        -1, 1, -1, 1,  0, 1, 0, 1,  1, 0,

        1, 1, 1, 1,    1, 1, 1, 1,  0, 1,
        -1, 1, 1, 1,   0, 1, 1, 1,  1, 1,
        -1, -1, 1, 1,  0, 0, 1, 1,  1, 0,
        -1, -1, 1, 1,  0, 0, 1, 1,  1, 0,
        1, -1, 1, 1,   1, 0, 1, 1,  0, 0,
        1, 1, 1, 1,    1, 1, 1, 1,  0, 1,

        1, -1, -1, 1,  1, 0, 0, 1,  0, 1,
        -1, -1, -1, 1, 0, 0, 0, 1,  1, 1,
        -1, 1, -1, 1,  0, 1, 0, 1,  1, 0,
        1, 1, -1, 1,   1, 1, 0, 1,  0, 0,
        1, -1, -1, 1,  1, 0, 0, 1,  0, 1,
        -1, 1, -1, 1,  0, 1, 0, 1,  1, 0,
    };
    // clang-format on
    {
        size_t vertexBufferSize = m_vertices.size() * sizeof(Vertex);
        WGPUBufferDescriptor bufferDescriptor{};
        bufferDescriptor.size = vertexBufferSize;
        bufferDescriptor.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        // bufferDescriptor.mappedAtCreation = true;

        m_cubeVertexBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
        assert(m_cubeVertexBuffer);
        // void* mappedVertexPtr = wgpu.BufferGetMappedRange(m_cubeVertexBuffer, 0, vertexBufferSize);
        // memcpy(mappedVertexPtr, m_vertices.data(), vertexBufferSize);
        // wgpu.BufferUnmap(m_cubeVertexBuffer);

        wgpu.QueueWriteBuffer(m_queue, m_cubeVertexBuffer, 0, m_vertices.data(), vertexBufferSize);
    }

    {
        size_t indexBufferSize = m_indices.size() * sizeof(IndexType);
        WGPUBufferDescriptor bufferDescriptor{};
        bufferDescriptor.size = indexBufferSize;
        bufferDescriptor.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        // bufferDescriptor.mappedAtCreation = true;

        m_cubeIndexBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
        assert(m_cubeIndexBuffer);

        // void* mappedIndexPtr = wgpu.BufferGetMappedRange(m_cubeIndexBuffer, 0, indexBufferSize);
        // memcpy(mappedIndexPtr, m_indices.data(), indexBufferSize);
        // wgpu.BufferUnmap(m_cubeIndexBuffer);

        wgpu.QueueWriteBuffer(m_queue, m_cubeIndexBuffer, 0, m_indices.data(), indexBufferSize);
    }
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
    m_pipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_pipelineLayout);
}

void WGPURotatingCube::createPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_None; // TODO: backface culling
    primitiveState.frontFace = WGPUFrontFace_CCW;
    // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

    std::vector<WGPUVertexAttribute> attributes{};
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x3;
        attribute.offset = offsetof(Vertex, pos);
        attribute.shaderLocation = 0;

        attributes.push_back(attribute);
    }
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x3;
        attribute.offset = offsetof(Vertex, color);
        attribute.shaderLocation = 1;

        attributes.push_back(attribute);
    }

    std::vector<WGPUVertexBufferLayout> vertexBufferLayout(1);
    vertexBufferLayout[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout[0].attributes = attributes.data();
    vertexBufferLayout[0].attributeCount = static_cast<uint32_t>(attributes.size());
    vertexBufferLayout[0].arrayStride = sizeof(Vertex);

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

    // WGPUDepthStencilState depthStencilState{};
    // depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;

    WGPUMultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;

    WGPURenderPipelineDescriptor renderPipelineDescriptor{};
    renderPipelineDescriptor.layout = m_pipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.multisample = multisampleState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragState;

    m_renderPipeline = wgpu.DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);

    assert(m_renderPipeline);
}

} // namespace jipu