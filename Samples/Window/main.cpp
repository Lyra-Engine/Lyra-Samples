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

struct Camera
{
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 up;
    glm::vec3 dir;
    float     far;
    float     speed        = 0.0f;
    float     acceleration = 0.0f;
};

struct InverseTransform
{
    glm::mat4 inv_view_proj;
    glm::vec3 camera_pos;
    glm::vec2 fade_range;
};

GPUShaderModule    vshader;
GPUShaderModule    fshader;
GPUBindGroupLayout blayout;
GPUPipelineLayout  playout;
GPURenderPipeline  pipeline;
GPUBuffer          ubuffer;
Camera             camera;

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
        entry.count                     = 1;
        entry.visibility                = GPUShaderStage::FRAGMENT;
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
        desc.fragment.targets.push_back(target);

        return device.create_render_pipeline(desc);
    });
}

void setup_buffers()
{
    auto& device = RHI::get_current_device();

    ubuffer = execute([&]() {
        auto desc               = GPUBufferDescriptor{};
        desc.label              = "uniform_buffer";
        desc.size               = sizeof(InverseTransform);
        desc.usage              = GPUBufferUsage::UNIFORM | GPUBufferUsage::MAP_WRITE;
        desc.mapped_at_creation = true;
        return device.create_buffer(desc);
    });
}

void setup_camera()
{
    camera.position = glm::vec3(0.0f, 1.0f, 3.0f);
    camera.center   = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.up       = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.far      = 100.0f;
}

void cleanup()
{
    auto& device = RHI::get_current_device();
    device.wait();

    // NOTE: This is optional, because all resources will be automatically
    // collected by device at destruction.
    vshader.destroy();
    fshader.destroy();
    blayout.destroy();
    playout.destroy();
    pipeline.destroy();
}

void update(const WindowInput& input)
{
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 right   = glm::cross(forward, camera.up);
    glm::vec3 dir     = glm::vec3(0.0f, 0.0f, 0.0f);

    if (input.is_key_down(KeyButton::W))
        dir += forward;

    if (input.is_key_down(KeyButton::S))
        dir -= forward;

    if (input.is_key_down(KeyButton::A))
        dir -= right;

    if (input.is_key_down(KeyButton::D))
        dir += right;

    if (glm::length(dir) > 0.0) {
        constexpr float damping = 0.01f;
        constexpr float epsilon = 1e-3;

        camera.acceleration = 5.0f;
        camera.speed += camera.acceleration * input.delta_time;
        camera.speed -= damping * input.delta_time;
        camera.speed = std::max(0.0f, camera.speed);
        if (camera.speed > 5.0f)
            camera.speed = 5.0f;
        if (camera.speed < epsilon)
            camera.speed = 0.0f;

        camera.dir = glm::normalize(dir);
    }

    // constant update
    camera.position += camera.speed * dir * input.delta_time;
    camera.center = camera.position + forward;

    auto& surface = RHI::get_current_surface();
    auto  extent  = surface.get_current_extent();

    // update transforms
    auto proj = glm::perspective(
        1.05f, float(extent.width) / float(extent.height), 0.1f, 100.0f);
    auto view = glm::lookAt(camera.position, camera.center, camera.up);

    // update uniform
    auto uniform                = ubuffer.get_mapped_range<InverseTransform>();
    uniform.at(0).inv_view_proj = glm::inverse(proj * view);
    uniform.at(0).camera_pos    = camera.position;
    uniform.at(0).fade_range    = glm::vec2(5.0f, 10.0f);
}

void render()
{
    auto& device  = RHI::get_current_device();
    auto& surface = RHI::get_current_surface();

    // acquire next frame from swapchain
    auto texture = surface.get_current_texture();
    if (texture.suboptimal)
        return;

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
        entry.index         = 0;
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
    command.resource_barrier(state_transition(texture.texture, undefined_state(),
        color_attachment_state()));
    command.begin_render_pass(render_pass);
    command.set_viewport(0, 0, extent.width, extent.height);
    command.set_scissor_rect(0, 0, extent.width, extent.height);
    command.set_pipeline(pipeline);
    command.set_bind_group(0, bind_group);
    command.draw(3, 1, 0, 0);
    command.end_render_pass();
    command.resource_barrier(state_transition(
        texture.texture, color_attachment_state(), present_src_state()));
    command.signal(texture.complete, GPUBarrierSync::RENDER_TARGET);
    command.submit();

    // present this frame to swapchain
    texture.present();
}

void resize(const WindowInfo& info)
{
    std::cout << "Window Resized: " << info.width << "x" << info.height
              << std::endl;
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

        // desc.frames_inflight = 2;
        return rhi->request_surface(desc);
    });

    (void)surface; // avoid unused warning

    win->bind<WindowEvent::START>(setup_pipeline);
    win->bind<WindowEvent::START>(setup_buffers);
    win->bind<WindowEvent::START>(setup_camera);
    win->bind<WindowEvent::CLOSE>(cleanup);
    win->bind<WindowEvent::UPDATE>(update);
    win->bind<WindowEvent::RENDER>(render);
    win->bind<WindowEvent::RESIZE>(resize);
    win->loop();

    return 0;
}
