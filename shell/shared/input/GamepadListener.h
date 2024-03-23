/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>
#include <unordered_map>

namespace igl {
namespace shell {

// Note: Definitions below assume the button layout of a standard Xbox controller. See
// https://github.com/mdqinc/SDL_GameControllerDB

enum class GamepadButton {
  Up,
  Right,
  Down,
  Left,
  A,
  B,
  X,
  Y,
  LeftBumper,
  RightBumper,
  LeftStick,
  RightStick,
  Back,
  Start,
  Guide,
};

enum class GamepadAxis {
  LeftStickX, // -1.0 (left) .. 1.0 (right)
  LeftStickY, // -1.0 (down) .. 1.0 (up)
  LeftTrigger, // 0.0 (up) .. (1.0) down
  RightStickX, // -1.0 (left) .. 1.0 (right)
  RightStickY, // -1.0 (down) .. 1.0 (up)
  RightTrigger, // 0.0 (up) .. (1.0) down
};

class Gamepad {
 public:
  const std::string name;

  const std::unordered_map<GamepadButton, bool>& getButtonStates() const {
    return _buttonStates;
  }
  const std::unordered_map<GamepadAxis, float>& getAxisValues() const {
    return _axisValues;
  }

  void updateButtonStates(std::unordered_map<GamepadButton, bool>&& states) {
    _buttonStates = std::move(states);
  }
  void updateAxisValues(std::unordered_map<GamepadAxis, float>&& values) {
    _axisValues = std::move(values);
  }

  Gamepad(const char* name) : name(name) {}

 private:
  std::unordered_map<GamepadButton, bool> _buttonStates;
  std::unordered_map<GamepadAxis, float> _axisValues;
};

struct GamepadDeviceEvent {
  const Gamepad& device;
  const bool isConnected;

  GamepadDeviceEvent(const Gamepad& device, bool isConnected) :
    device(device), isConnected(isConnected) {}
};

struct GamepadButtonEvent {
  const Gamepad& device;
  const GamepadButton button;
  const bool isDown;

  GamepadButtonEvent(const Gamepad& device, GamepadButton button, bool isDown) :
    device(device), button(button), isDown(isDown) {}
};

class IGamepadListener {
 public:
  virtual bool process(const GamepadDeviceEvent& event) = 0;
  virtual bool process(const GamepadButtonEvent& event) = 0;

  virtual ~IGamepadListener() = default;
};

} // namespace shell
} // namespace igl
