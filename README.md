# Chilli

Chilli is a 3D game engine utilizing modern Vulkan 1.3 for rendering.

## Structure

### ChilliCore
Handles the foundational engine systems:
- **Windowing**: Creation and management of application windows (GLFW).
- **Input**: Keyboard, mouse, and controller input handling.
- **Events**: Event dispatching and processing.
- **Layers**: Layered architecture for modular engine features.
- **Applications**: Application lifecycle management.

### ChilliRenderAPI
Primarily Vulkan-based (for now), responsible for:
- **Rendering**: Command buffer management, frame rendering.
- **Resource Creation/Deletion**: Buffers, textures, pipelines, etc.
- **Synchronization**: Fences, semaphores, frame pacing.

### ChilliExtensions (Planned)
Future module to transform Chilli from a rendering engine into a full game engine:
- **Gameplay Systems**
- **Scripting**
- **Physics**
- **Audio**

## Dependencies

- [GLFW](https://www.glfw.org/) (Windowing & Input)
- [glm](https://github.com/g-truc/glm) (Math library)
- [Vulkan Memory Allocator (VMA)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) (Efficient Vulkan memory management)
- [spdlog](https://github.com/gabime/spdlog) (Logging)

## Example Code

### 1. Resource Creation: Vulkan Buffer

````````
			std::vector<Vertex> vertices;
			vertices.reserve(3);
			vertices.push_back(glm::vec3(0.0f, -0.5f, 0.0f));
			vertices.push_back(glm::vec3(0.5f, 0.5f, 0.0f));
			vertices.push_back(glm::vec3(-0.5f, 0.5f, 0.0f));

			Chilli::VertexBufferSpec Spec{};
			Spec.Data = vertices.data();
			Spec.Size = vertices.size() * sizeof(Vertex);
			Spec.Count = vertices.size();
			Spec.Type = Chilli::BufferType::STATIC_DRAW;

			VertexBuffer = Chilli::Renderer::GetResourceFactory()->CreateVertexBuffer(Spec);
````````markdown

### 2. Render Loop

````````
		Chilli::Renderer::BeginFrame();

		Chilli::Renderer::BeginRenderPass();

		Chilli::Renderer::Submit(Shader, VertexBuffer, IndexBuffer);

		Chilli::Renderer::EndRenderPass();

		Chilli::Renderer::RenderFrame();
		Chilli::Renderer::Present();

		Chilli::Renderer::EndFrame();
````````markdown

### 3. Resource Deletion

````````
		Chilli::Renderer::GetResourceFactory()->DestroyVertexBuffer(VertexBuffer);
````````markdown

## Screenshots

_Add screenshots of your application here!_

![Screenshot Placeholder](C:\Users\anujb\OneDrive\Pictures\Screenshots\Screenshot 2025-10-01 214751.png)

---

For more details, see the documentation and source code.
