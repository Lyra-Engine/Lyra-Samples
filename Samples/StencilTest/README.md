# Stencil Test

This is a simple example with stencil testing enabled.
This example assumes users have read the **DepthTest** example.

This example becomes more complicated because there are two pipelines
created in order to show the behavior of the stencil testing.

The first pass is to draw into the stencil buffer (with color write disabled).
The second pass is to draw two triangles only in the stencil mask area.

This example includes:

1. depth/stencil buffer creation
2. pipeline with stencil setup
3. render pass wih stencil buffer
4. set stencil value in command buffer

## Depth/Stencil Buffer Creation

For simplicity, **Lyra-Engine** only supports combined depth/stencil buffer.

```cpp
dsbuffer = execute([&]() {
    auto extent          = surface.get_current_extent();
    auto desc            = GPUTextureDescriptor{};
    desc.format          = GPUTextureFormat::DEPTH24PLUS_STENCIL8; // NOTE: stencil is also included
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
```

## Pipeline With Stencil Setup

For the mask pass, we disable the color writes, and set stencil test to always true:

```cpp
auto target         = GPUColorTargetState{};
target.blend_enable = false;
target.write_mask   = GPUColorWrite::NONE; // NOTE: disable color write

auto desc                                      = GPURenderPipelineDescriptor{};
...
desc.depth_stencil.stencil_read_mask           = 0x0;
desc.depth_stencil.stencil_write_mask          = 0x1;
desc.depth_stencil.stencil_front.depth_fail_op = GPUStencilOperation::KEEP;
desc.depth_stencil.stencil_front.compare       = GPUCompareFunction::ALWAYS;
desc.depth_stencil.stencil_front.pass_op       = GPUStencilOperation::REPLACE;
desc.depth_stencil.stencil_front.fail_op       = GPUStencilOperation::REPLACE;
desc.depth_stencil.stencil_back                = desc.depth_stencil.stencil_front;
```

For the color pass, we enable color writes, and set stencil test to equal specific value:
```cpp
auto desc                                      = GPURenderPipelineDescriptor{};
desc.depth_stencil.stencil_read_mask           = 0x1;
desc.depth_stencil.stencil_write_mask          = 0x0;
desc.depth_stencil.stencil_front.depth_fail_op = GPUStencilOperation::KEEP;
desc.depth_stencil.stencil_front.compare       = GPUCompareFunction::EQUAL;
desc.depth_stencil.stencil_front.pass_op       = GPUStencilOperation::KEEP; // don't modify stencil
desc.depth_stencil.stencil_front.fail_op       = GPUStencilOperation::KEEP; // don't modify stencil
desc.depth_stencil.stencil_back                = desc.depth_stencil.stencil_front;
```

## Render Pass Setup with Stencil Buffer

For the mask pass, write to stencil buffer:

```cpp
auto stencil_attachment                = GPURenderPassDepthStencilAttachment{};
stencil_attachment.view                = dsview;
stencil_attachment.stencil_clear_value = 0;
stencil_attachment.stencil_load_op     = GPULoadOp::CLEAR;
stencil_attachment.stencil_store_op    = GPUStoreOp::STORE;
stencil_attachment.stencil_read_only   = false;

render_pass.depth_stencil_attachment = stencil_attachment;
```

For the color pass, disable write to stencil buffer, but use it for stencil testing:

```cpp
// depth attachment
auto depth_attachment                = GPURenderPassDepthStencilAttachment{};
depth_attachment.view                = dsview;
depth_attachment.stencil_clear_value = 0;
depth_attachment.stencil_load_op     = GPULoadOp::LOAD;
depth_attachment.stencil_store_op    = GPUStoreOp::DISCARD;
depth_attachment.stencil_read_only   = true;
```

## Set Stencil Reference in Command Buffer

User can control what value for stencil to write/compare against in the command buffer.

```cpp
command.set_stencil_reference(0x1);
```
