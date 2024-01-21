/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <shell/renderSessions/ImguiSession.h>

#include <igl/CommandBuffer.h>
#include <igl/RenderPass.h>

namespace igl {
namespace shell {

void ImguiSession::initialize() noexcept {
  const igl::CommandQueueDesc desc{igl::CommandQueueType::Graphics};
  _commandQueue = getPlatform().getDevice().createCommandQueue(desc, nullptr);

  { // Create the ImGui session
    _imguiSession = std::make_unique<iglu::imgui::Session>(getPlatform().getDevice(),
                                                           getPlatform().getInputDispatcher());
  }
}

void ImguiSession::update(igl::SurfaceTextures surfaceTextures) noexcept {
  igl::DeviceScope deviceScope(getPlatform().getDevice());

  auto cmdBuffer = _commandQueue->createCommandBuffer(igl::CommandBufferDesc(), nullptr);

  igl::FramebufferDesc framebufferDesc;
  framebufferDesc.colorAttachments[0].texture = surfaceTextures.color;
  if (_outputFramebuffer) {
    _outputFramebuffer->updateDrawable(surfaceTextures.color);
  } else {
    _outputFramebuffer = getPlatform().getDevice().createFramebuffer(framebufferDesc, nullptr);
  }

  igl::RenderPassDesc renderPassDesc;
  renderPassDesc.colorAttachments.resize(1);
  renderPassDesc.colorAttachments[0].loadAction = igl::LoadAction::Clear;
  renderPassDesc.colorAttachments[0].storeAction = igl::StoreAction::Store;
  renderPassDesc.colorAttachments[0].clearColor = getPlatform().getDevice().backendDebugColor();
  auto encoder = cmdBuffer->createRenderCommandEncoder(renderPassDesc, _outputFramebuffer);

  { // Draw using ImGui every frame
    _imguiSession->beginFrame(*_outputFramebuffer,
                              getPlatform().getDisplayContext().pixelsPerPoint);

    if (_isFirstFrame) {
      // Suggestion from
      // https://discourse.dearimgui.org/t/add-method-to-modify-default-size-pos-of-demo-window/184/2
      // to avoid terrible default position & size of the demo window.
      ImGuiIO io = ImGui::GetIO();
      float display_width = (float)io.DisplaySize.x;
      float display_height = (float)io.DisplaySize.y;
      float demo_window_pos_x = display_width * 0.10;
      float demo_window_pos_y = display_height * 0.10;
      float demo_window_size_x = display_width * 0.80;
      float demo_window_size_y = display_height * 0.80;

      ImGui::SetNextWindowPos(ImVec2(demo_window_pos_x, demo_window_pos_y), 0);
      ImGui::SetNextWindowSize(ImVec2(demo_window_size_x, demo_window_size_y), 0);
      ImGui::Begin("Dear ImGui Demo");
      ImGui::End();

      _isFirstFrame = false;
    }

    ImGui::ShowDemoWindow();
    _imguiSession->endFrame(getPlatform().getDevice(), *encoder);
  }

  encoder->endEncoding();
  cmdBuffer->present(surfaceTextures.color);

  _commandQueue->submit(*cmdBuffer);
}

} // namespace shell
} // namespace igl
