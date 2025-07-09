# Depth Test

This is a simple example with depth testing enabled.
This example assumes users have read the **Triangle** example.

This example includes:
1. depth buffer setup
2. pipeline setup with depth testing
3. render pass with depth buffer

## Depth Buffer Setup

Creating a depth buffer is no different from creating a regular texture.
However, format must satify the requirements of a depth buffer.
The usage is also **RENDER_ATTACHMENT**, but RHI will infer from the
texture format to understand it is a depth buffer, not a render target.

A texture view is also need for render pass.

```cpp
dbuffer = execute([&]() {
    auto extent          = surface.get_current_extent();
    auto desc            = GPUTextureDescriptor{};
    desc.format          = GPUTextureFormat::DEPTH16UNORM;
    desc.size.width      = extent.width;
    desc.size.height     = extent.height;
    desc.size.depth      = 1;
    desc.array_layers    = 1;
    desc.mip_level_count = 1;
    desc.usage           = GPUTextureUsage::RENDER_ATTACHMENT;
    desc.label           = "depth_buffer";
    return device.create_texture(desc);
});

dview = dbuffer.create_view();
```

## Pipeline with Depth Testing

Follow the previous example, we need to make a few tweaks to enable depth testing.

```cpp
auto desc = GPURenderPipelineDescriptor{};
...
desc.depth_stencil.format              = GPUTextureFormat::DEPTH16UNORM; // NOTE: specify the depth format here
desc.depth_stencil.depth_compare       = GPUCompareFunction::LESS;       // NOTE: specify the depth compare function
desc.depth_stencil.depth_write_enabled = true;                           // NOTE: enable depth write
...
```

## Render Pass with Depth Buffer

```cpp
auto color_attachment        = GPURenderPassColorAttachment{};
...

auto depth_attachment              = GPURenderPassDepthStencilAttachment{};
depth_attachment.view              = dview;
depth_attachment.depth_clear_value = 1.0f;
depth_attachment.depth_load_op     = GPULoadOp::CLEAR;
depth_attachment.depth_store_op    = GPUStoreOp::STORE;
depth_attachment.depth_read_only   = false;

auto render_pass                     = GPURenderPassDescriptor{};
render_pass.color_attachments        = {color_attachment};
render_pass.depth_stencil_attachment = depth_attachment;
```
