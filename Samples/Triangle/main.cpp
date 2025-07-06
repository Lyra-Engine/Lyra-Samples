#include <iostream>
#include <string_view>
#include <cmrc/cmrc.hpp>

#include <Lyra/Common/GLM.h>
#include <Lyra/Common.hpp>
#include <Lyra/Render.hpp>
#include <Lyra/Window.hpp>

using namespace lyra;
using namespace lyra::wsi;
using namespace lyra::rhi;

CMRC_DECLARE(resources);

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
};

GPUShaderModule    vshader;
GPUShaderModule    fshader;
GPUBindGroupLayout blayout;
GPUPipelineLayout  playout;
GPURenderPipeline  pipeline;
GPUBuffer          vbuffer;
GPUBuffer          ibuffer;
GPUBuffer          ubuffer;

auto read_shader_source() -> const char*
{
    auto fs      = cmrc::resources::get_filesystem();
    auto data    = fs.open("shader.slang");
    auto program = std::string_view(data.begin(), data.end() - data.begin()).data();
    std::cout << program << std::endl;
    return program;
}

void setup_pipeline()
{
    auto& device  = RHI::get_current_device();
    auto& surface = RHI::get_current_surface();

    auto compiler = execute([&]() {
        auto desc   = CompilerDescriptor{};
        desc.target = CompileTarget::DXIL;
        desc.flags  = CompileFlag::DEBUG | CompileFlag::REFLECT;
        return Compiler::init(desc);
    });

    auto module = execute([&]() {
        auto desc   = CompileDescriptor{};
        desc.module = "test";
        desc.path   = "test.slang";
        desc.source = read_shader_source();
        return compiler->compile(desc);
    });

    vshader = execute([&]() {
        auto code  = module->get_shader_blob("vsmain");
        auto desc  = GPUShaderModuleDescriptor{};
        desc.label = "vertex_shader";
        desc.data  = code->data;
        desc.size  = code->size;
        return device.create_shader_module(desc);
    });

    fshader = execute([&]() {
        auto code  = module->get_shader_blob("fsmain");
        auto desc  = GPUShaderModuleDescriptor{};
        desc.label = "fragment_shader";
        desc.data  = code->data;
        desc.size  = code->size;
        return device.create_shader_module(desc);
    });

    blayout = execute([&]() {
        auto desc                       = GPUBindGroupLayoutDescriptor{};
        auto entry                      = GPUBindGroupLayoutEntry{};
        entry.type                      = GPUBindingResourceType::BUFFER;
        entry.binding                   = 0;
        entry.visibility                = GPUShaderStage::VERTEX;
        entry.buffer.type               = GPUBufferBindingType::UNIFORM;
        entry.buffer.has_dynamic_offset = false;
        desc.entries.push_back(entry);
        return device.create_bind_group_layout(desc);
    });

    playout = execute([&]() {
        auto desc               = GPUPipelineLayoutDescriptor{};
        desc.bind_group_layouts = {blayout};
        return device.create_pipeline_layout(desc);
    });

    pipeline = execute([&]() {
        auto position            = GPUVertexAttribute{};
        position.format          = GPUVertexFormat::FLOAT32x3;
        position.offset          = 0;
        position.shader_location = 0;

        auto color            = GPUVertexAttribute{};
        color.format          = GPUVertexFormat::FLOAT32x3;
        color.offset          = sizeof(float) * 3;
        color.shader_location = 1;

        auto layout         = GPUVertexBufferLayout{};
        layout.attributes   = {position, color};
        layout.array_stride = sizeof(float) * 6;
        layout.step_mode    = GPUVertexStepMode::VERTEX;

        auto target         = GPUColorTargetState{};
        target.format       = surface.get_current_format();
        target.blend_enable = false;

        auto desc                                  = GPURenderPipelineDescriptor{};
        desc.layout                                = playout;
        desc.primitive.cull_mode                   = GPUCullMode::NONE;
        desc.primitive.topology                    = GPUPrimitiveTopology::TRIANGLE_LIST;
        desc.primitive.front_face                  = GPUFrontFace::CCW;
        desc.primitive.strip_index_format          = GPUIndexFormat::UINT32;
        desc.depth_stencil.depth_compare           = GPUCompareFunction::ALWAYS;
        desc.depth_stencil.depth_write_enabled     = false;
        desc.multisample.alpha_to_coverage_enabled = false;
        desc.multisample.count                     = 1;
        desc.vertex.module                         = vshader;
        desc.fragment.module                       = fshader;
        desc.vertex.buffers.push_back(layout);
        desc.fragment.targets.push_back(target);

        return device.create_render_pipeline(desc);
    });
}

void setup_buffers()
{
    auto& device = RHI::get_current_device();

    vbuffer = execute([&]() {
        auto desc               = GPUBufferDescriptor{};
        desc.label              = "vertex_buffer";
        desc.size               = sizeof(Vertex) * 3;
        desc.usage              = GPUBufferUsage::VERTEX | GPUBufferUsage::MAP_WRITE;
        desc.mapped_at_creation = true;
        return device.create_buffer(desc);
    });

    ibuffer = execute([&]() {
        auto desc               = GPUBufferDescriptor{};
        desc.label              = "index_buffer";
        desc.size               = sizeof(uint32_t) * 3;
        desc.usage              = GPUBufferUsage::INDEX | GPUBufferUsage::MAP_WRITE;
        desc.mapped_at_creation = true;
        return device.create_buffer(desc);
    });

    ubuffer = execute([&]() {
        auto desc               = GPUBufferDescriptor{};
        desc.label              = "uniform_buffer";
        desc.size               = sizeof(glm::mat4x4);
        desc.usage              = GPUBufferUsage::UNIFORM | GPUBufferUsage::MAP_WRITE;
        desc.mapped_at_creation = true;
        return device.create_buffer(desc);
    });

    auto vertices = vbuffer.get_mapped_range<Vertex>();

    // positions
    vertices.at(0).position = {0.0f, 0.0f, 0.0f};
    vertices.at(1).position = {1.0f, 0.0f, 0.0f};
    vertices.at(2).position = {0.0f, 1.0f, 0.0f};

    // colors
    vertices.at(0).color = {1.0f, 0.0f, 0.0f};
    vertices.at(1).color = {0.0f, 1.0f, 0.0f};
    vertices.at(2).color = {0.0f, 0.0f, 1.0f};

    // indices
    auto indices  = ibuffer.get_mapped_range<uint>();
    indices.at(0) = 0;
    indices.at(1) = 1;
    indices.at(2) = 2;

    // uniform
    auto& surface    = RHI::get_current_surface();
    auto  extent     = surface.get_current_extent();
    auto  uniform    = ubuffer.get_mapped_range<glm::mat4>();
    auto  projection = glm::perspective(1.05f, float(extent.width) / float(extent.height), 0.1f, 100.0f);
    auto  modelview  = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    uniform.at(0) = projection * modelview;
}

void cleanup()
{
    auto& device = RHI::get_current_device();
    device.wait();

    // NOTE: This is optional, because all resources will be automatically collected by device at destruction.
    ibuffer.destroy();
    vbuffer.destroy();
    vshader.destroy();
    fshader.destroy();
    blayout.destroy();
    playout.destroy();
    pipeline.destroy();
}

void render()
{
    auto& device  = RHI::get_current_device();
    auto& surface = RHI::get_current_surface();

    // acquire next frame from swapchain
    auto texture = surface.get_current_texture();
    if (texture.suboptimal) return;

    // create command buffer
    auto command = execute([&]() {
        auto desc  = GPUCommandBufferDescriptor{};
        desc.queue = GPUQueueType::DEFAULT;
        return device.create_command_buffer(desc);
    });

    // create bind group
    auto bind_group = execute([&]() {
        auto entry          = GPUBindGroupEntry{};
        entry.type          = GPUBindingResourceType::BUFFER;
        entry.binding       = 0;
        entry.buffer.buffer = ubuffer;
        entry.buffer.offset = 0;
        entry.buffer.size   = 0;

        auto desc   = GPUBindGroupDescriptor{};
        desc.layout = blayout;
        desc.entries.push_back(entry);
        return device.create_bind_group(desc);
    });

    auto color_attachment        = GPURenderPassColorAttachment{};
    color_attachment.clear_value = GPUColor{0.0f, 0.0f, 0.0f, 0.0f};
    color_attachment.load_op     = GPULoadOp::CLEAR;
    color_attachment.store_op    = GPUStoreOp::STORE;
    color_attachment.view        = texture.view;

    auto render_pass                     = GPURenderPassDescriptor{};
    render_pass.color_attachments        = {color_attachment};
    render_pass.depth_stencil_attachment = {};

    auto extent = surface.get_current_extent();
    command.wait(texture.available, GPUBarrierSync::PIXEL_SHADING);
    command.resource_barrier(state_transition(texture.texture, undefined_state(), color_attachment_state()));
    command.begin_render_pass(render_pass);
    command.set_viewport(0, 0, extent.width, extent.height);
    command.set_scissor_rect(0, 0, extent.width, extent.height);
    command.set_pipeline(pipeline);
    command.set_vertex_buffer(0, vbuffer);
    command.set_index_buffer(ibuffer, GPUIndexFormat::UINT32);
    command.set_bind_group(0, bind_group);
    command.draw_indexed(3, 1, 0, 0, 0);
    command.end_render_pass();
    command.resource_barrier(state_transition(texture.texture, color_attachment_state(), present_src_state()));
    command.signal(texture.complete, GPUBarrierSync::RENDER_TARGET);
    command.submit();

    // present this frame to swapchain
    texture.present();
}

int main()
{
    auto win = execute([&]() {
        auto desc   = WindowDescriptor{};
        desc.title  = "Lyra Engine :: Sample";
        desc.width  = 1920;
        desc.height = 1080;
        return Window::init(desc);
    });

    auto rhi = execute([&] {
        auto desc    = RHIDescriptor{};
        desc.backend = RHIBackend::D3D12;
        desc.flags   = RHIFlag::DEBUG | RHIFlag::VALIDATION;
        desc.window  = win->handle;
        return RHI::init(desc);
    });

    auto adapter = execute([&]() {
        auto desc = GPUAdapterDescriptor{};
        return rhi->request_adapter(desc);
    });

    auto device = execute([&]() {
        auto desc  = GPUDeviceDescriptor{};
        desc.label = "main_device";
        desc.required_features.push_back(GPUFeatureName::SHADER_F16);
        desc.required_features.push_back(GPUFeatureName::FLOAT32_BLENDABLE);
        return adapter.request_device(desc);
    });

    auto surface = execute([&]() {
        auto desc         = GPUSurfaceDescriptor{};
        desc.label        = "main_surface";
        desc.window       = win->handle;
        desc.present_mode = GPUPresentMode::Fifo;
        return rhi->request_surface(desc);
    });

    (void)surface; // avoid unused warning

    win->bind<WindowEvent::START>(setup_pipeline);
    win->bind<WindowEvent::START>(setup_buffers);
    win->bind<WindowEvent::CLOSE>(cleanup);
    win->bind<WindowEvent::RENDER>(render);
    win->loop();

    return 0;
}
