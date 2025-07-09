# Window

This is an example demonstrating how to create a basic window.

This example includes:
1. window creation
2. window events
3. window inputs

## Window Creation

**Lyra-Engine** supports window creation via **WindowDescriptor**.
The window object is created with **std::unique_ptr**. Therefore
it should be expected to clean up itself after main function exits.

```cpp
auto win = execute([&]() {
    auto desc   = WindowDescriptor{};
    desc.title  = "Lyra Engine :: Sample";
    desc.width  = 1920;
    desc.height = 1080;
    return Window::init(desc);
});
```

## Window Event Handling

**Lyra-ENgine** registers different window events with callbacks.
Multiple callbacks can be bound to the same event. They are expected
to be invoked in the bound order. For example:

```cpp
win->bind<WindowEvent::START>(setup_pipeline);
win->bind<WindowEvent::START>(setup_buffers);
win->bind<WindowEvent::START>(setup_camera);
win->bind<WindowEvent::CLOSE>(cleanup);
win->bind<WindowEvent::UPDATE>(update);
win->bind<WindowEvent::RENDER>(render);
win->bind<WindowEvent::RESIZE>(resize);
```

**WindowEvent::START** is invoked once prior to the window starts.
**WindowEvent::CLOSE** is invoked once after the window closes, but
**WindowEvent::RESIZE** is invoked when window resizes.
invoked in the reversed order.

**WindowEvent::UPDATE** and **WindowEvent::RENDER** are invoked every frame,
with **WindowEvent::UPDATE** invoked before **WindowEvent::RENDER**.
They are artificially introduced events used for the render loop.
**WindowEvent::UPDATE** differs from **WindowEvent::RENDER** in that **UPDATE**
receives **WindowInput**, which can be used to control the application.

There is also **WindowEvent::TIMER**, which is designed for fixed timestamp update.
Fixed-timestamp update is usually used for physics calculation. It is naturally detached
from render loop on purpose.

## Window Input Handling

Unlike **GLFW**, which allows users to bind their callbacks to user inputs, **Lyra-Engine**
handles all the user inputs and record the state of each button/mouse in **WindowInput**.
Users simply query the state of an input control during the frame. For example:

```cpp
if (input.is_key_down(KeyButton::W) || input.is_key_down(KeyButton::UP))
    dir += forward;

if (input.is_key_down(KeyButton::S) || input.is_key_down(KeyButton::DOWN))
    dir -= forward;

if (input.is_key_down(KeyButton::A) || input.is_key_down(KeyButton::LEFT))
    dir -= right;

if (input.is_key_down(KeyButton::D) || input.is_key_down(KeyButton::RIGHT))
    dir += right;
```
