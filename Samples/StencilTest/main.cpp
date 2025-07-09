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
GPURenderPipeline  pipeline_mask;
GPURenderPipeline  pipeline_draw;
GPUBuffer          vbuffer_mask;
GPUBuffer          ibuffer_mask;
GPUBuffer          vbuffer_draw;
GPUBuffer          ibuffer_draw;
GPUBuffer          ubuffer;
GPUTexture         dsbuffer;
GPUTextureView     dsview;

auto read_shader_source() -> const char*
{
    auto fs      = cmrc::resources::get_filesystem();
    auto data    = fs.open("shader.slang");
    auto program = std::string_view(data.begin(), data.end() - data.begin()).data();
    std::cout << program << std::endl;
    return program;
}

void setup_pipelines()
{
    auto& device  = RHI::get_current_device();
    auto& surface = RHI::get_current_surface();

    auto compiler = execute([&]() {
        auto desc   = CompilerDescriptor{};
        desc.target = LYRA_RHI_COMPILER;
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

    pipeline_mask = execute([&]() {
        auto position            = GPUVertexAttribute{};
        position.format          = GPUVertexFormat::FLOAT32x3;
        position.offset          = offsetof(Vertex, position);
        position.shader_location = 0;

        auto color            = GPUVertexAttribute{};
        color.format          = GPUVertexFormat::FLOAT32x3;
        color.offset          = offsetof(Vertex, color);
        color.shader_location = 1;

        auto layout         = GPUVertexBufferLayout{};
        layout.attributes   = {position, color};
        layout.array_stride = sizeof(Vertex);
        layout.step_mode    = GPUVertexStepMode::VERTEX;

        auto target         = GPUColorTargetState{};
        target.format       = surface.get_current_format();
        target.blend_enable = false;
        target.write_mask   = GPUColorWrite::NONE; // NOTE: disable color write

        auto desc                                      = GPURenderPipelineDescriptor{};
        desc.layout                                    = playout;
        desc.primitive.cull_mode                       = GPUCullMode::NONE;
        desc.primitive.topology                        = GPUPrimitiveTopology::TRIANGLE_LIST;
        desc.primitive.front_face                      = GPUFrontFace::CCW;
        desc.primitive.strip_index_format              = GPUIndexFormat::UINT32;
        desc.depth_stencil.format                      = GPUTextureFormat::DEPTH24PLUS_STENCIL8;
        desc.depth_stencil.depth_compare               = GPUCompareFunction::ALWAYS;
        desc.depth_stencil.depth_write_enabled         = false;
        desc.depth_stencil.stencil_read_mask           = 0x0;
        desc.depth_stencil.stencil_write_mask          = 0x1;
        desc.depth_stencil.stencil_front.depth_fail_op = GPUStencilOperation::KEEP;
        desc.depth_stencil.stencil_front.compare       = GPUCompareFunction::ALWAYS;
        desc.depth_stencil.stencil_front.pass_op       = GPUStencilOperation::REPLACE;
        desc.depth_stencil.stencil_front.fail_op       = GPUStencilOperation::REPLACE;
        desc.depth_stencil.stencil_back                = desc.depth_stencil.stencil_front;
        desc.multisample.alpha_to_coverage_enabled     = false;
        desc.multisample.count                         = 1;
        desc.vertex.module                             = vshader;
        desc.fragment.module                           = fshader;
        desc.vertex.buffers.push_back(layout);
        desc.fragment.targets.push_back(target);

        return device.create_render_pipeline(desc);
    });

    pipeline_draw = execute([&]() {
        auto position            = GPUVertexAttribute{};
        position.format          = GPUVertexFormat::FLOAT32x3;
        position.offset          = offsetof(Vertex, position);
        position.shader_location = 0;

        auto color            = GPUVertexAttribute{};
        color.format          = GPUVertexFormat::FLOAT32x3;
        color.offset          = offsetof(Vertex, color);
        color.shader_location = 1;

        auto layout         = GPUVertexBufferLayout{};
        layout.attributes   = {position, color};
        layout.array_stride = sizeof(Vertex);
        layout.step_mode    = GPUVertexStepMode::VERTEX;

        auto target         = GPUColorTargetState{};
        target.format       = surface.get_current_format();
        target.blend_enable = false;

        auto desc                                      = GPURenderPipelineDescriptor{};
        desc.layout                                    = playout;
        desc.primitive.cull_mode                       = GPUCullMode::NONE;
        desc.primitive.topology                        = GPUPrimitiveTopology::TRIANGLE_LIST;
        desc.primitive.front_face                      = GPUFrontFace::CCW;
        desc.primitive.strip_index_format              = GPUIndexFormat::UINT32;
        desc.depth_stencil.format                      = GPUTextureFormat::DEPTH24PLUS_STENCIL8;
        desc.depth_stencil.depth_compare               = GPUCompareFunction::ALWAYS;
        desc.depth_stencil.depth_write_enabled         = true;
        desc.depth_stencil.stencil_read_mask           = 0x1;
        desc.depth_stencil.stencil_write_mask          = 0x0;
        desc.depth_stencil.stencil_front.depth_fail_op = GPUStencilOperation::KEEP;
        desc.depth_stencil.stencil_front.compare       = GPUCompareFunction::EQUAL;
        desc.depth_stencil.stencil_front.pass_op       = GPUStencilOperation::KEEP; // don't modify stencil
        desc.depth_stencil.stencil_front.fail_op       = GPUStencilOperation::KEEP; // don't modify stencil
        desc.depth_stencil.stencil_back                = desc.depth_stencil.stencil_front;
        desc.multisample.alpha_to_coverage_enabled     = false;
        desc.multisample.count                         = 1;
        desc.vertex.module                             = vshader;
        desc.fragment.module                           = fshader;
        desc.vertex.buffers.push_back(layout);
        desc.fragment.targets.push_back(target);

        return device.create_render_pipeline(desc);
    });
}

void setup_mask_geometry()
{
    auto& device = RHI::get_current_device();

    vbuffer_mask = execute([&]() {
        auto desc               = GPUBufferDescriptor{};
        desc.label              = "vertex_buffer";
        desc.size               = sizeof(Vertex) * 3;
        desc.usage              = GPUBufferUsage::VERTEX | GPUBufferUsage::MAP_WRITE;
        desc.mapped_at_creation = true;
        return device.create_buffer(desc);
    });

    ibuffer_mask = execute([&]() {
        auto desc               = GPUBufferDescriptor{};
        desc.label              = "index_buffer";
        desc.size               = sizeof(uint32_t) * 3;
        desc.usage              = GPUBufferUsage::INDEX | GPUBufferUsage::MAP_WRITE;
        desc.mapped_at_creation = true;
        return device.create_buffer(desc);
    });

    auto vertices = vbuffer_mask.get_mapped_range<Vertex>();

    // positions (tri 1)
    vertices.at(0).position = {0.0f, 0.0f, 0.0f};
    vertices.at(1).position = {1.0f, 0.0f, 0.0f};
    vertices.at(2).position = {0.0f, 1.0f, 0.0f};

    // colors (tri 1)
    vertices.at(0).color = {1.0f, 1.0f, 0.0f};
    vertices.at(1).color = {1.0f, 1.0f, 0.0f};
    vertices.at(2).color = {1.0f, 1.0f, 0.0f};

    // indices
    auto indices  = ibuffer_mask.get_mapped_range<uint>();
    indices.at(0) = 0;
    indices.at(1) = 1;
    indices.at(2) = 2;
}

void setup_draw_geometry()
{
    auto& device = RHI::get_current_device();

    vbuffer_draw = execute([&]() {
        auto desc               = GPUBufferDescriptor{};
        desc.label              = "vertex_buffer";
        desc.size               = sizeof(Vertex) * 6;
        desc.usage              = GPUBufferUsage::VERTEX | GPUBufferUsage::MAP_WRITE;
        desc.mapped_at_creation = true;
        return device.create_buffer(desc);
    });

    ibuffer_draw = execute([&]() {
        auto desc               = GPUBufferDescriptor{};
        desc.label              = "index_buffer";
        desc.size               = sizeof(uint32_t) * 6;
        desc.usage              = GPUBufferUsage::INDEX | GPUBufferUsage::MAP_WRITE;
        desc.mapped_at_creation = true;
        return device.create_buffer(desc);
    });

    auto vertices = vbuffer_draw.get_mapped_range<Vertex>();

    // positions (tri 1)
    vertices.at(0).position = {0.0f, 0.0f, 0.0f};
    vertices.at(1).position = {1.0f, 0.0f, 0.0f};
    vertices.at(2).position = {0.0f, 1.0f, 0.0f};

    // colors (tri 1)
    vertices.at(0).color = {1.0f, 1.0f, 0.0f};
    vertices.at(1).color = {1.0f, 1.0f, 0.0f};
    vertices.at(2).color = {1.0f, 1.0f, 0.0f};

    // positions (tri 2)
    vertices.at(3).position = {0.0f - 0.25f, 0.0f - 0.25f, +1.0f};
    vertices.at(4).position = {1.0f - 0.25f, 0.0f - 0.25f, +1.0f};
    vertices.at(5).position = {0.0f - 0.25f, 1.0f - 0.25f, +1.0f};

    // colors (tri 2)
    vertices.at(3).color = {0.0f, 1.0f, 1.0f};
    vertices.at(4).color = {0.0f, 1.0f, 1.0f};
    vertices.at(5).color = {0.0f, 1.0f, 1.0f};

    // indices
    auto indices  = ibuffer_draw.get_mapped_range<uint>();
    indices.at(0) = 0;
    indices.at(1) = 1;
    indices.at(2) = 2;
    indices.at(3) = 3;
    indices.at(4) = 4;
    indices.at(5) = 5;
}

void setup_uniform_buffer()
{
    auto& device = RHI::get_current_device();

    ubuffer = execute([&]() {
        auto desc               = GPUBufferDescriptor{};
        desc.label              = "uniform_buffer";
        desc.size               = sizeof(glm::mat4x4);
        desc.usage              = GPUBufferUsage::UNIFORM | GPUBufferUsage::MAP_WRITE;
        desc.mapped_at_creation = true;
        return device.create_buffer(desc);
    });

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

void setup_stencil_buffer()
{
    auto& device  = RHI::get_current_device();
    auto& surface = RHI::get_current_surface();

    dsbuffer = execute([&]() {
        auto extent          = surface.get_current_extent();
        auto desc            = GPUTextureDescriptor{};
        desc.format          = GPUTextureFormat::DEPTH24PLUS_STENCIL8;
        desc.size.width      = extent.width;
        desc.size.height     = extent.height;
        desc.size.depth      = 1;
        desc.array_layers    = 1;
        desc.mip_level_count = 1;
        desc.usage           = GPUTextureUsage::RENDER_ATTACHMENT;
        desc.label           = "depth_stencil_buffer";
        return device.create_texture(desc);
    });

    dsview = dsbuffer.create_view();
}

void cleanup()
{
    auto& device = RHI::get_current_device();
    device.wait();

    // NOTE: This is optional, because all resources will be automatically collected by device at destruction.
    dsbuffer.destroy();
    ibuffer_mask.destroy();
    vbuffer_mask.destroy();
    ibuffer_draw.destroy();
    vbuffer_draw.destroy();
    pipeline_mask.destroy();
    pipeline_draw.destroy();
    vshader.destroy();
    fshader.destroy();
    blayout.destroy();
    playout.destroy();
}

void render_mask(GPUCommandBuffer& command, const GPUSurfaceTexture& backbuffer, const GPUBindGroup& bind_group)
{
    auto& surface = RHI::get_current_surface();
    auto  extent  = surface.get_current_extent();

    // color attachments
    auto color_attachment        = GPURenderPassColorAttachment{};
    color_attachment.clear_value = GPUColor{0.0f, 0.0f, 0.0f, 0.0f};
    color_attachment.load_op     = GPULoadOp::CLEAR;
    color_attachment.store_op    = GPUStoreOp::DISCARD;
    color_attachment.view        = backbuffer.view;

    // stencil attachment
    auto stencil_attachment                = GPURenderPassDepthStencilAttachment{};
    stencil_attachment.view                = dsview;
    stencil_attachment.depth_clear_value   = 1.0f;
    stencil_attachment.depth_load_op       = GPULoadOp::CLEAR;
    stencil_attachment.depth_store_op      = GPUStoreOp::DISCARD;
    stencil_attachment.depth_read_only     = true;
    stencil_attachment.stencil_clear_value = 0;
    stencil_attachment.stencil_load_op     = GPULoadOp::CLEAR;
    stencil_attachment.stencil_store_op    = GPUStoreOp::STORE;
    stencil_attachment.stencil_read_only   = false;

    // render pass info
    auto render_pass                     = GPURenderPassDescriptor{};
    render_pass.color_attachments        = {color_attachment};
    render_pass.depth_stencil_attachment = stencil_attachment;

    command.begin_render_pass(render_pass);
    command.set_viewport(0, 0, extent.width, extent.height);
    command.set_scissor_rect(0, 0, extent.width, extent.height);
    command.set_pipeline(pipeline_mask);
    command.set_vertex_buffer(0, vbuffer_mask);
    command.set_index_buffer(ibuffer_mask, GPUIndexFormat::UINT32);
    command.set_bind_group(0, bind_group);
    command.set_stencil_reference(0x1);
    command.draw_indexed(3, 1, 0, 0, 0);
    command.end_render_pass();
}

void render_color(GPUCommandBuffer& command, const GPUSurfaceTexture& backbuffer, const GPUBindGroup& bind_group)
{
    auto& surface = RHI::get_current_surface();
    auto  extent  = surface.get_current_extent();

    // color attachments
    auto color_attachment        = GPURenderPassColorAttachment{};
    color_attachment.clear_value = GPUColor{0.0f, 0.0f, 0.0f, 0.0f};
    color_attachment.load_op     = GPULoadOp::CLEAR;
    color_attachment.store_op    = GPUStoreOp::STORE;
    color_attachment.view        = backbuffer.view;

    // depth attachment
    auto depth_attachment                = GPURenderPassDepthStencilAttachment{};
    depth_attachment.view                = dsview;
    depth_attachment.depth_clear_value   = 1.0f;
    depth_attachment.depth_load_op       = GPULoadOp::CLEAR;
    depth_attachment.depth_store_op      = GPUStoreOp::STORE;
    depth_attachment.depth_read_only     = false;
    depth_attachment.stencil_clear_value = 0;
    depth_attachment.stencil_load_op     = GPULoadOp::LOAD;
    depth_attachment.stencil_store_op    = GPUStoreOp::DISCARD;
    depth_attachment.stencil_read_only   = true;

    // render pass info
    auto render_pass                     = GPURenderPassDescriptor{};
    render_pass.color_attachments        = {color_attachment};
    render_pass.depth_stencil_attachment = depth_attachment;

    command.begin_render_pass(render_pass);
    command.set_viewport(0, 0, extent.width, extent.height);
    command.set_scissor_rect(0, 0, extent.width, extent.height);
    command.set_pipeline(pipeline_draw);
    command.set_vertex_buffer(0, vbuffer_draw);
    command.set_index_buffer(ibuffer_draw, GPUIndexFormat::UINT32);
    command.set_bind_group(0, bind_group);
    command.set_stencil_reference(0x1);
    command.draw_indexed(6, 1, 0, 0, 0);
    command.end_render_pass();
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

    // commands
    command.wait(texture.available, GPUBarrierSync::PIXEL_SHADING);
    command.resource_barrier(state_transition(texture.texture, undefined_state(), color_attachment_state()));
    command.resource_barrier(state_transition(dsbuffer, undefined_state(), depth_stencil_attachment_state()));
    render_mask(command, texture, bind_group);
    render_color(command, texture, bind_group);
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
        desc.backend = LYRA_RHI_BACKEND;
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

    win->bind<WindowEvent::START>(setup_pipelines);
    win->bind<WindowEvent::START>(setup_mask_geometry);
    win->bind<WindowEvent::START>(setup_draw_geometry);
    win->bind<WindowEvent::START>(setup_uniform_buffer);
    win->bind<WindowEvent::START>(setup_stencil_buffer);
    win->bind<WindowEvent::CLOSE>(cleanup);
    win->bind<WindowEvent::RENDER>(render);
    win->loop();

    return 0;
}
