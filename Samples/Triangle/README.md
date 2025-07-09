Triangle
========

This is the most basic example showcasing how to render a triangle.

This example includes:

1. device creation
1. pipeline creation
2. vertex/index/uniform buffer creation
3. bind group (descriptor set) creation
4. necessary texture layout transition
5. basic rendering routines

## Device Creation

**Lyra-Engine** supports single render device, and follows the 3-steps procedure:
1. create an instance (rhi instance)
2. create an adapter (physical device)
3. create device (logical device)

```cpp
// similar to VkInstasnce, IDXGIFactory
// std::unique_ptr, will automatically clean up
auto rhi = execute([&] {
    auto desc    = RHIDescriptor{};
    desc.backend = LYRA_RHI_BACKEND;
    desc.flags   = RHIFlag::DEBUG | RHIFlag::VALIDATION;
    desc.window  = win->handle;
    return RHI::init(desc);
});

// similar to VkPhysicalDevice, IDXGIAdapter
// RHI will clean this up automatically
auto adapter = execute([&]() {
    auto desc = GPUAdapterDescriptor{};
    return rhi->request_adapter(desc);
});

// similar to VkDevice, ID3D12Device
// RHI will clean this up automatically
auto device = execute([&]() {
    auto desc  = GPUDeviceDescriptor{};
    desc.label = "main_device";
    return adapter.request_device(desc);
});
```

Both RHI and adapter are not going to be used very often later.
Device, on the other hand, will be used prevalently throughout in the program.
Therefore a convenience method is provided:

```cpp
auto& device  = RHI::get_current_device();
```

## Surface & Swapchain Creation

**Lyra-Engine** combines the surface and swapchain creation.

```cpp
auto surface = execute([&]() {
    auto desc         = GPUSurfaceDescriptor{};
    desc.label        = "main_surface";
    desc.window       = win->handle;
    desc.present_mode = GPUPresentMode::Fifo;
    return rhi->request_surface(desc);
});
```

The surface object will also be used a lot in the later program,
because users will directly query the current swapchain extent from surface.
Therefore a convenience method is also provided:

```cpp
auto& surface = RHI::get_current_surface();
```

## Buffer creation

**Lyra-Engine** supports creating buffer objects with explicit usages and memory model.

```cpp
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
```

Since these buffers are directly host visible, we are able to map it directly for writes.
A helper method is provided to return the mapped pointer in **TypedBufferRange<T>**.
The TypedBufferRange is like a span, with some basic bounds checking.

```cpp
auto vertices = vbuffer.get_mapped_range<Vertex>();

vertices.at(0).position = {0.0f, 0.0f, 0.0f};
vertices.at(1).position = {1.0f, 0.0f, 0.0f};
vertices.at(2).position = {0.0f, 1.0f, 0.0f};
```

## Pipelien Creation

In order to support creating a graphics pipeline, a number of things are supported as pre-requisites.

### Shader Compiler

This creates a unique_ptr of compiler, which will clean up itself.
CompileTarget also supports **CompileTarget::DXIL**, used for D3D12.

```cpp
auto compiler = execute([&]() {
    auto desc   = CompilerDescriptor{};
    desc.target = CompileTarget::SPIRV;
    desc.flags  = CompileFlag::DEBUG | CompileFlag::REFLECT;
    return Compiler::init(desc);
});
```

### Shader Module

This creates a shader module that can be directly fed into pipeline creation.
The shader module object could be manually destroyed, but RHI will also collect it when program exits.

```cpp
auto vshader = execute([&]() {
    auto code  = module->get_shader_blob("vsmain");
    auto desc  = GPUShaderModuleDescriptor{};
    desc.label = "vertex_shader";
    desc.data  = code->data;
    desc.size  = code->size;
    return device.create_shader_module(desc);
});
```

### Bind Group Layout

This creates a bind group layout that can be directly fed into pipeline creation.
The bind group layout is equivalent to Vulkan's descriptor set layout.
The bind group layout object could be manually destroyed, but RHI will also collect it when program exits.

```cpp
auto blayout = execute([&]() {
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
```

### Pipeline Layout

This creates a pipeline layout that can be directly fed into pipeline creation.
The pipeline layout object could be manually destroyed, but RHI will also collect it when program exits.

```cpp
auto playout = execute([&]() {
    auto desc               = GPUPipelineLayoutDescriptor{};
    desc.bind_group_layouts = {blayout};
    return device.create_pipeline_layout(desc);
});
```

### Pipeline Creation

This creates a graphics pipeline state object.
The pipeline state object could be manually destroyed, but RHI will also collect it when program exits.

```cpp
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
```

## Command Buffer Creation

Command buffer objects are created per-frame, and will not be usable in the next frame.
Command buffer objects do not need to be destroyed. **Lyra-Engine** supports multiple
queue types: **DEFAULT** for graphics/compute/copy, **COMPUTE** for compute/copy, and
**TRANSFER** for copy-only queues.

```cpp
auto command = execute([&]() {
    auto desc  = GPUCommandBufferDescriptor{};
    desc.queue = GPUQueueType::DEFAULT;
    return device.create_command_buffer(desc);
});
```

Once a command buffer is created, it does not need to call methods like **begin** or **end**
because RHI will automatically handle it in the backend.

Users could also create command bundle. These are secondary command buffers that cannot
be directly submitted to queues, but have to be invoked by primary command buffers.

## Frame Synchronization with Fences

For every frame, we will receive two fences from the surface texture. They are:
1. texture.available: indicating when the current back buffer will be available for rendering
2. texture.complete: indiciating when the current frame will finish rendering

For every frame, we will need to wait for the current back buffer to be available,
and notify when this frame rendering will be complete for the future frames.

```cpp
auto texture = surface.get_current_texture();
command.wait(texture.available, GPUBarrierSync::PIXEL_SHADING);
command.signal(texture.complete, GPUBarrierSync::RENDER_TARGET);
```

The **wait** and **signal** will NOT immediately take effect, but they will be making a
difference when the command buffer is submitted.

## Render Pass with State Transition

**Lyra-Engine** uses D3D12 style render pass (and Vulkan's dynamic rendering).
In order to begin a render pass, color attachments, depth/stencil attachment should be
specified properly.

```cpp
auto color_attachment        = GPURenderPassColorAttachment{};
color_attachment.clear_value = GPUColor{0.0f, 0.0f, 0.0f, 0.0f};
color_attachment.load_op     = GPULoadOp::CLEAR;
color_attachment.store_op    = GPUStoreOp::STORE;
color_attachment.view        = texture.view;

auto render_pass                     = GPURenderPassDescriptor{};
render_pass.color_attachments        = {color_attachment};
render_pass.depth_stencil_attachment = {};

command.resource_barrier(state_transition(texture.texture, undefined_state(), color_attachment_state()));
command.begin_render_pass(render_pass);
command.set_viewport(0, 0, extent.width, extent.height);
command.set_scissor_rect(0, 0, extent.width, extent.height);
...
command.end_render_pass();
command.resource_barrier(state_transition(texture.texture, color_attachment_state(), present_src_state()));
```

Notably, render pass does not handle state transitions (unlike traditional VkRenderPass).
Therefore, manual state transition from undefined layout to color attachment layout should be done prior to render pass.
Manual state transition from color attachment to present src for screen display is also necessary.

## Bind Group Creation

Bind groups are descriptor objects being used as arguments to shaders.
Bind group objects are also per-frame. No need to manually destroy.

```cpp
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
```

## Rendering Commands

In order to correctly draw a triangle, one must bind pipeline, bind vertex and index buffers, bind descriptors,
and finally draw.

```cpp
command.set_pipeline(pipeline);
command.set_vertex_buffer(0, vbuffer);
command.set_index_buffer(ibuffer, GPUIndexFormat::UINT32);
command.set_bind_group(0, bind_group);
command.draw_indexed(3, 1, 0, 0, 0);
```

## Swapchain Presentation

Simply present to **GPUSurfaceTexture**.

```cpp
texture.present();
```
