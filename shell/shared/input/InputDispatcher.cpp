/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "InputDispatcher.h"
#include "MouseListener.h"
#include "TouchListener.h"

namespace igl {
namespace shell {

void InputDispatcher::processEvents() {
  std::lock_guard<std::mutex> guard(_mutex);

  while (!_events.empty()) {
    auto& event = _events.front();

    if (event.type == EventType::MouseButton || event.type == EventType::MouseMotion ||
        event.type == EventType::MouseWheel) {
      for (auto& listener : _mouseListeners) {
        if (event.type == EventType::MouseButton &&
            listener->process(std::get<MouseButtonEvent>(event.data))) {
          break;
        }
        if (event.type == EventType::MouseMotion &&
            listener->process(std::get<MouseMotionEvent>(event.data))) {
          break;
        }
        if (event.type == EventType::MouseWheel &&
            listener->process(std::get<MouseWheelEvent>(event.data))) {
          break;
        }
      }
    } else if (event.type == EventType::Touch) {
      for (auto& listener : _touchListeners) {
        if (event.type == EventType::Touch && listener->process(std::get<TouchEvent>(event.data))) {
          break;
        }
      }
    } else if (event.type == EventType::Character || event.type == EventType::Key) {
      for (auto& listener : _keyListeners) {
        if (event.type == EventType::Character &&
            listener->process(std::get<CharacterEvent>(event.data))) {
          break;
        }
        if (event.type == EventType::Key && listener->process(std::get<KeyEvent>(event.data))) {
          break;
        }
      }
    } else if (event.type == EventType::Ray) {
      for (auto& listener : _rayListeners) {
        if (event.type == EventType::Ray && listener->process(std::get<RayEvent>(event.data))) {
          break;
        }
      }
    } else if (event.type == EventType::GamepadDevice || event.type == EventType::GamepadButton) {
      for (auto& listener : _gamepadListeners) {
        if (event.type == EventType::GamepadDevice &&
            listener->process(std::get<GamepadDeviceEvent>(event.data))) {
          break;
        }
        if (event.type == EventType::GamepadButton &&
            listener->process(std::get<GamepadButtonEvent>(event.data))) {
          break;
        }
      }
    }

    _events.pop();
  }
}

void InputDispatcher::addMouseListener(const std::shared_ptr<IMouseListener>& listener) {
  std::lock_guard<std::mutex> guard(_mutex);
  _mouseListeners.push_back(listener);
}

void InputDispatcher::removeMouseListener(const std::shared_ptr<IMouseListener>& listener) {
  std::lock_guard<std::mutex> guard(_mutex);
  for (int i = _mouseListeners.size() - 1; i >= 0; --i) {
    if (listener.get() == _mouseListeners[i].get()) {
      _mouseListeners.erase(_mouseListeners.begin() + i);
    }
  }
}

void InputDispatcher::addTouchListener(const std::shared_ptr<ITouchListener>& listener) {
  std::lock_guard<std::mutex> guard(_mutex);
  _touchListeners.push_back(listener);
}

void InputDispatcher::removeTouchListener(const std::shared_ptr<ITouchListener>& listener) {
  std::lock_guard<std::mutex> guard(_mutex);
  for (int i = _touchListeners.size() - 1; i >= 0; --i) {
    if (listener.get() == _touchListeners[i].get()) {
      _touchListeners.erase(_touchListeners.begin() + i);
    }
  }
}

void InputDispatcher::addKeyListener(const std::shared_ptr<IKeyListener>& listener) {
  std::lock_guard<std::mutex> guard(_mutex);
  _keyListeners.push_back(listener);
}

void InputDispatcher::removeKeyListener(const std::shared_ptr<IKeyListener>& listener) {
  std::lock_guard<std::mutex> guard(_mutex);
  for (int i = _keyListeners.size() - 1; i >= 0; --i) {
    if (listener.get() == _keyListeners[i].get()) {
      _keyListeners.erase(_keyListeners.begin() + i);
    }
  }
}

void InputDispatcher::addRayListener(const std::shared_ptr<IRayListener>& listener) {
  std::lock_guard<std::mutex> const guard(_mutex);
  _rayListeners.push_back(listener);
}

void InputDispatcher::removeRayListener(const std::shared_ptr<IRayListener>& listener) {
  std::lock_guard<std::mutex> const guard(_mutex);
  for (int i = _rayListeners.size() - 1; i >= 0; --i) {
    if (listener.get() == _rayListeners[i].get()) {
      _rayListeners.erase(_rayListeners.begin() + i);
    }
  }
}

void InputDispatcher::addGamepadListener(const std::shared_ptr<IGamepadListener>& listener) {
  std::lock_guard<std::mutex> const guard(_mutex);
  _gamepadListeners.push_back(listener);
}

void InputDispatcher::removeGamepadListener(const std::shared_ptr<IGamepadListener>& listener) {
  std::lock_guard<std::mutex> const guard(_mutex);
  for (int i = _gamepadListeners.size() - 1; i >= 0; --i) {
    if (listener.get() == _gamepadListeners[i].get()) {
      _gamepadListeners.erase(_gamepadListeners.begin() + i);
    }
  }
}

#define DEFINE_queueEvent_FUNCTION(EventClass, EventType) \
  void InputDispatcher::queueEvent(EventClass&& event) {  \
    std::lock_guard<std::mutex> guard(_mutex);            \
    _events.emplace(EventType, std::move(event));         \
  }

DEFINE_queueEvent_FUNCTION(MouseButtonEvent, EventType::MouseButton);
DEFINE_queueEvent_FUNCTION(MouseMotionEvent, EventType::MouseMotion);
DEFINE_queueEvent_FUNCTION(MouseWheelEvent, EventType::MouseWheel);
DEFINE_queueEvent_FUNCTION(TouchEvent, EventType::Touch);
DEFINE_queueEvent_FUNCTION(RayEvent, EventType::Ray);
DEFINE_queueEvent_FUNCTION(CharacterEvent, EventType::Character);
DEFINE_queueEvent_FUNCTION(KeyEvent, EventType::Key);
DEFINE_queueEvent_FUNCTION(GamepadDeviceEvent, EventType::GamepadDevice);
DEFINE_queueEvent_FUNCTION(GamepadButtonEvent, EventType::GamepadButton);

} // namespace shell
} // namespace igl
