/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "InputListener.h"

// ImGui has a very awkward expectation when it comes to processing inputs and making decisions
// based on them. This is what it expects clients to do, in order, every frame:
// 1. Send ImGui all events via the input parameters in ImGuiIO.
// 2. Call ImGui::NewFrame -- that's when events are processed.
// 3. Read the output parameters of ImGuiIO to know which events it wants to capture.
// 4. Forward uncaptured events to other systems.
//
// This is an awkward expectation and we currently don't follow it. Instead, we process events
// before calling ImGui::NewFrame and immediately check whether ImGui wants to capture events, which
// is one frame old. This can be a source of problems if we have multiple input listeners and
// depending on how they process inputs.

namespace iglu {
namespace imgui {

// Implemented at the end of this file, for readability.
ImGuiKey asImguiKey(igl::shell::Key key);

InputListener::InputListener(ImGuiContext* context) {
  _context = context;
}

bool InputListener::process(const igl::shell::CharacterEvent& event) {
  makeCurrentContext();

  ImGuiIO& io = ImGui::GetIO();
  io.AddInputCharacter(event.character);
  return io.WantCaptureKeyboard;
}

bool InputListener::process(const igl::shell::KeyEvent& event) {
  makeCurrentContext();

  ImGuiKey imguiKey = asImguiKey(event.key);

  if (imguiKey == ImGuiKey_LeftShift) {
    _leftShiftDown = event.isDown;
  }
  if (imguiKey == ImGuiKey_LeftCtrl) {
    _leftCtrlDown = event.isDown;
  }
  if (imguiKey == ImGuiKey_LeftAlt) {
    _leftAltDown = event.isDown;
  }
  if (imguiKey == ImGuiKey_LeftSuper) {
    _leftSuperDown = event.isDown;
  }
  if (imguiKey == ImGuiKey_RightShift) {
    _rightShiftDown = event.isDown;
  }
  if (imguiKey == ImGuiKey_RightCtrl) {
    _rightCtrlDown = event.isDown;
  }
  if (imguiKey == ImGuiKey_RightAlt) {
    _rightAltDown = event.isDown;
  }
  if (imguiKey == ImGuiKey_RightSuper) {
    _rightSuperDown = event.isDown;
  }

  ImGuiIO& io = ImGui::GetIO();
  io.AddKeyEvent(imguiKey, event.isDown);
  // On top of regular up/down key events, ImGui expects modifier key events to be sent
  // independently (confirmed via samples in the imgui repo). Not clear why, as it could extract
  // that information from the events themselves...
  io.AddKeyEvent(ImGuiMod_Ctrl, _leftCtrlDown || _rightCtrlDown);
  io.AddKeyEvent(ImGuiMod_Shift, _leftShiftDown || _rightShiftDown);
  io.AddKeyEvent(ImGuiMod_Alt, _leftAltDown || _rightAltDown);
  io.AddKeyEvent(ImGuiMod_Super, _leftSuperDown || _rightSuperDown);
  return io.WantCaptureKeyboard;
}

bool InputListener::process(const igl::shell::MouseButtonEvent& event) {
  makeCurrentContext();

  ImGuiIO& io = ImGui::GetIO();
  io.MouseSource = ImGuiMouseSource_Mouse;
  io.MousePos = ImVec2(event.x, event.y);
  io.MouseDown[event.button] = event.isDown;
  return io.WantCaptureMouse;
}

bool InputListener::process(const igl::shell::MouseMotionEvent& event) {
  makeCurrentContext();

  ImGuiIO& io = ImGui::GetIO();
  io.MouseSource = ImGuiMouseSource_Mouse;
  io.MousePos = ImVec2(event.x, event.y);
  return io.WantCaptureMouse;
}

bool InputListener::process(const igl::shell::MouseWheelEvent& event) {
  makeCurrentContext();

  ImGuiIO& io = ImGui::GetIO();
  io.MouseSource = ImGuiMouseSource_Mouse;
  io.MouseWheelH = event.dx;
  io.MouseWheel = event.dy;
  return io.WantCaptureMouse;
}

bool InputListener::process(const igl::shell::TouchEvent& event) {
  makeCurrentContext();

  ImGuiIO& io = ImGui::GetIO();
  io.MouseSource = ImGuiMouseSource_TouchScreen;
  io.MousePos = ImVec2(event.x, event.y);
  io.MouseDown[0] = event.isDown;
  return io.WantCaptureMouse;
}

void InputListener::makeCurrentContext() const {
  ImGui::SetCurrentContext(_context);
}

ImGuiKey asImguiKey(igl::shell::Key key) {
  switch (key) {
  // Control
  case igl::shell::Key::Escape:
    return ImGuiKey_Escape;
  case igl::shell::Key::LeftShift:
    return ImGuiKey_LeftShift;
  case igl::shell::Key::LeftCtrl:
    return ImGuiKey_LeftCtrl;
  case igl::shell::Key::LeftAlt:
    return ImGuiKey_LeftAlt;
  case igl::shell::Key::LeftSuper:
    return ImGuiKey_LeftSuper;
  case igl::shell::Key::RightShift:
    return ImGuiKey_RightShift;
  case igl::shell::Key::RightCtrl:
    return ImGuiKey_RightCtrl;
  case igl::shell::Key::RightAlt:
    return ImGuiKey_RightAlt;
  case igl::shell::Key::RightSuper:
    return ImGuiKey_RightSuper;

  // Navigation
  case igl::shell::Key::LeftArrow:
    return ImGuiKey_LeftArrow;
  case igl::shell::Key::RightArrow:
    return ImGuiKey_RightArrow;
  case igl::shell::Key::UpArrow:
    return ImGuiKey_UpArrow;
  case igl::shell::Key::DownArrow:
    return ImGuiKey_DownArrow;
  case igl::shell::Key::PageUp:
    return ImGuiKey_PageUp;
  case igl::shell::Key::PageDown:
    return ImGuiKey_PageDown;
  case igl::shell::Key::Home:
    return ImGuiKey_Home;
  case igl::shell::Key::End:
    return ImGuiKey_End;
  case igl::shell::Key::Insert:
    return ImGuiKey_Insert;
  case igl::shell::Key::Delete:
    return ImGuiKey_Delete;

  // Typing
  case igl::shell::Key::Space:
    return ImGuiKey_Space;
  case igl::shell::Key::Enter:
    return ImGuiKey_Enter;
  case igl::shell::Key::Backspace:
    return ImGuiKey_Backspace;
  case igl::shell::Key::Tab:
    return ImGuiKey_Tab;
  case igl::shell::Key::CapsLock:
    return ImGuiKey_CapsLock;

  // Characters
  case igl::shell::Key::A:
    return ImGuiKey_A;
  case igl::shell::Key::B:
    return ImGuiKey_B;
  case igl::shell::Key::C:
    return ImGuiKey_C;
  case igl::shell::Key::D:
    return ImGuiKey_D;
  case igl::shell::Key::E:
    return ImGuiKey_E;
  case igl::shell::Key::F:
    return ImGuiKey_F;
  case igl::shell::Key::G:
    return ImGuiKey_G;
  case igl::shell::Key::H:
    return ImGuiKey_H;
  case igl::shell::Key::I:
    return ImGuiKey_I;
  case igl::shell::Key::J:
    return ImGuiKey_J;
  case igl::shell::Key::K:
    return ImGuiKey_K;
  case igl::shell::Key::L:
    return ImGuiKey_L;
  case igl::shell::Key::M:
    return ImGuiKey_M;
  case igl::shell::Key::N:
    return ImGuiKey_N;
  case igl::shell::Key::O:
    return ImGuiKey_O;
  case igl::shell::Key::P:
    return ImGuiKey_P;
  case igl::shell::Key::Q:
    return ImGuiKey_Q;
  case igl::shell::Key::R:
    return ImGuiKey_R;
  case igl::shell::Key::S:
    return ImGuiKey_S;
  case igl::shell::Key::T:
    return ImGuiKey_T;
  case igl::shell::Key::U:
    return ImGuiKey_U;
  case igl::shell::Key::V:
    return ImGuiKey_V;
  case igl::shell::Key::W:
    return ImGuiKey_W;
  case igl::shell::Key::X:
    return ImGuiKey_X;
  case igl::shell::Key::Y:
    return ImGuiKey_Y;
  case igl::shell::Key::Z:
    return ImGuiKey_Z;

  // Digits
  case igl::shell::Key::Zero:
    return ImGuiKey_0;
  case igl::shell::Key::One:
    return ImGuiKey_1;
  case igl::shell::Key::Two:
    return ImGuiKey_2;
  case igl::shell::Key::Three:
    return ImGuiKey_3;
  case igl::shell::Key::Four:
    return ImGuiKey_4;
  case igl::shell::Key::Five:
    return ImGuiKey_5;
  case igl::shell::Key::Six:
    return ImGuiKey_6;
  case igl::shell::Key::Seven:
    return ImGuiKey_7;
  case igl::shell::Key::Eight:
    return ImGuiKey_8;
  case igl::shell::Key::Nine:
    return ImGuiKey_9;

  // Punctuation
  case igl::shell::Key::GraveAccent:
    return ImGuiKey_GraveAccent;
  case igl::shell::Key::Minus:
    return ImGuiKey_Minus;
  case igl::shell::Key::Equal:
    return ImGuiKey_Equal;
  case igl::shell::Key::LeftBracket:
    return ImGuiKey_LeftBracket;
  case igl::shell::Key::RightBracket:
    return ImGuiKey_RightBracket;
  case igl::shell::Key::Backslash:
    return ImGuiKey_Backslash;
  case igl::shell::Key::Semicolon:
    return ImGuiKey_Semicolon;
  case igl::shell::Key::Apostrophe:
    return ImGuiKey_Apostrophe;
  case igl::shell::Key::Comma:
    return ImGuiKey_Comma;
  case igl::shell::Key::Period:
    return ImGuiKey_Period;
  case igl::shell::Key::Slash:
    return ImGuiKey_Slash;

  // Function
  case igl::shell::Key::F1:
    return ImGuiKey_F1;
  case igl::shell::Key::F2:
    return ImGuiKey_F2;
  case igl::shell::Key::F3:
    return ImGuiKey_F3;
  case igl::shell::Key::F4:
    return ImGuiKey_F4;
  case igl::shell::Key::F5:
    return ImGuiKey_F5;
  case igl::shell::Key::F6:
    return ImGuiKey_F6;
  case igl::shell::Key::F7:
    return ImGuiKey_F7;
  case igl::shell::Key::F8:
    return ImGuiKey_F8;
  case igl::shell::Key::F9:
    return ImGuiKey_F9;
  case igl::shell::Key::F10:
    return ImGuiKey_F10;
  case igl::shell::Key::F11:
    return ImGuiKey_F11;
  case igl::shell::Key::F12:
    return ImGuiKey_F12;

  // Keypad
  case igl::shell::Key::NumLock:
    return ImGuiKey_NumLock;
  case igl::shell::Key::Keypad0:
    return ImGuiKey_Keypad0;
  case igl::shell::Key::Keypad1:
    return ImGuiKey_Keypad1;
  case igl::shell::Key::Keypad2:
    return ImGuiKey_Keypad2;
  case igl::shell::Key::Keypad3:
    return ImGuiKey_Keypad3;
  case igl::shell::Key::Keypad4:
    return ImGuiKey_Keypad4;
  case igl::shell::Key::Keypad5:
    return ImGuiKey_Keypad5;
  case igl::shell::Key::Keypad6:
    return ImGuiKey_Keypad6;
  case igl::shell::Key::Keypad7:
    return ImGuiKey_Keypad7;
  case igl::shell::Key::Keypad8:
    return ImGuiKey_Keypad8;
  case igl::shell::Key::Keypad9:
    return ImGuiKey_Keypad9;
  case igl::shell::Key::KeypadDecimal:
    return ImGuiKey_KeypadDecimal;
  case igl::shell::Key::KeypadDivide:
    return ImGuiKey_KeypadDivide;
  case igl::shell::Key::KeypadMultiply:
    return ImGuiKey_KeypadMultiply;
  case igl::shell::Key::KeypadSubtract:
    return ImGuiKey_KeypadSubtract;
  case igl::shell::Key::KeypadAdd:
    return ImGuiKey_KeypadAdd;
  case igl::shell::Key::KeypadEnter:
    return ImGuiKey_KeypadEnter;
  case igl::shell::Key::KeypadEqual:
    return ImGuiKey_KeypadEqual;

  // Esoteric
  case igl::shell::Key::ScrollLock:
    return ImGuiKey_ScrollLock;
  case igl::shell::Key::PrintScreen:
    return ImGuiKey_PrintScreen;
  case igl::shell::Key::Pause:
    return ImGuiKey_Pause;
  case igl::shell::Key::Menu:
    return ImGuiKey_Menu;
  }

  return ImGuiKey_None;
}

} // namespace imgui
} // namespace iglu
