/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

namespace igl {
namespace shell {

enum class Key {
  // Control
  Escape,
  LeftShift,
  LeftCtrl,
  LeftAlt,
  LeftSuper,
  RightShift,
  RightCtrl,
  RightAlt,
  RightSuper,

  // Navigation
  LeftArrow,
  RightArrow,
  UpArrow,
  DownArrow,
  PageUp,
  PageDown,
  Home,
  End,
  Insert,
  Delete,

  // Typing
  Space,
  Enter,
  Backspace,
  Tab,
  CapsLock,

  // Characters
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,

  // Digits
  Zero,
  One,
  Two,
  Three,
  Four,
  Five,
  Six,
  Seven,
  Eight,
  Nine,

  // Punctuation
  GraveAccent,
  Minus,
  Equal,
  LeftBracket,
  RightBracket,
  Backslash,
  Semicolon,
  Apostrophe,
  Comma,
  Period,
  Slash,

  // Function
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,

  // Keypad
  NumLock,
  Keypad0,
  Keypad1,
  Keypad2,
  Keypad3,
  Keypad4,
  Keypad5,
  Keypad6,
  Keypad7,
  Keypad8,
  Keypad9,
  KeypadDecimal,
  KeypadDivide,
  KeypadMultiply,
  KeypadSubtract,
  KeypadAdd,
  KeypadEnter,
  KeypadEqual,

  // Esoteric
  ScrollLock,
  PrintScreen,
  Pause,
  Menu,
};

struct CharacterEvent {
  unsigned int character;

  CharacterEvent(unsigned int character) : character(character) {}
};

struct KeyEvent {
  Key key;
  bool isDown;

  KeyEvent(bool isDown, Key key) : key(key), isDown(isDown) {}
};

class IKeyListener {
 public:
  virtual bool process(const CharacterEvent& event) = 0;
  virtual bool process(const KeyEvent& event) = 0;

  virtual ~IKeyListener() = default;
};

} // namespace shell
} // namespace igl
