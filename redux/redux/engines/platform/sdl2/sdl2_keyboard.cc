/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "redux/engines/platform/sdl2/sdl2_keyboard.h"

#include "redux/engines/platform/device_manager.h"

namespace redux {

#define KEYCODES                        \
  KEY(A, SDLK_a)                        \
  KEY(B, SDLK_b)                        \
  KEY(C, SDLK_c)                        \
  KEY(D, SDLK_d)                        \
  KEY(E, SDLK_e)                        \
  KEY(F, SDLK_f)                        \
  KEY(G, SDLK_g)                        \
  KEY(H, SDLK_h)                        \
  KEY(I, SDLK_i)                        \
  KEY(J, SDLK_j)                        \
  KEY(K, SDLK_k)                        \
  KEY(L, SDLK_l)                        \
  KEY(M, SDLK_m)                        \
  KEY(N, SDLK_n)                        \
  KEY(O, SDLK_o)                        \
  KEY(P, SDLK_p)                        \
  KEY(Q, SDLK_q)                        \
  KEY(R, SDLK_r)                        \
  KEY(S, SDLK_s)                        \
  KEY(T, SDLK_t)                        \
  KEY(U, SDLK_u)                        \
  KEY(V, SDLK_v)                        \
  KEY(W, SDLK_w)                        \
  KEY(X, SDLK_x)                        \
  KEY(Y, SDLK_y)                        \
  KEY(Z, SDLK_z)                        \
  KEY(0, SDLK_0)                        \
  KEY(1, SDLK_1)                        \
  KEY(2, SDLK_2)                        \
  KEY(3, SDLK_3)                        \
  KEY(4, SDLK_4)                        \
  KEY(5, SDLK_5)                        \
  KEY(6, SDLK_6)                        \
  KEY(7, SDLK_7)                        \
  KEY(8, SDLK_8)                        \
  KEY(9, SDLK_9)                        \
  KEY(KP_0, SDLK_KP_0)                  \
  KEY(KP_1, SDLK_KP_1)                  \
  KEY(KP_2, SDLK_KP_2)                  \
  KEY(KP_3, SDLK_KP_3)                  \
  KEY(KP_4, SDLK_KP_4)                  \
  KEY(KP_5, SDLK_KP_5)                  \
  KEY(KP_6, SDLK_KP_6)                  \
  KEY(KP_7, SDLK_KP_7)                  \
  KEY(KP_8, SDLK_KP_8)                  \
  KEY(KP_9, SDLK_KP_9)                  \
  KEY(KP_PLUS, SDLK_KP_PLUS)            \
  KEY(KP_MINUS, SDLK_KP_MINUS)          \
  KEY(KP_MULTIPLY, SDLK_KP_MULTIPLY)    \
  KEY(KP_DIVIDE, SDLK_KP_DIVIDE)        \
  KEY(KP_ENTER, SDLK_KP_ENTER)          \
  KEY(KP_PERIOD, SDLK_KP_PERIOD)        \
  KEY(KP_EQUALS, SDLK_KP_EQUALS)        \
  KEY(ESCAPE, SDLK_ESCAPE)              \
  KEY(TAB, SDLK_TAB)                    \
  KEY(RETURN, SDLK_RETURN)              \
  KEY(SPACE, SDLK_SPACE)                \
  KEY(BACKSPACE, SDLK_BACKSPACE)        \
  KEY(COMMA, SDLK_COMMA)                \
  KEY(PERIOD, SDLK_PERIOD)              \
  KEY(SLASH, SDLK_SLASH)                \
  KEY(BACKSLASH, SDLK_BACKSLASH)        \
  KEY(COLON, SDLK_COLON)                \
  KEY(SEMICOLON, SDLK_SEMICOLON)        \
  KEY(LEFT_BRACKET, SDLK_LEFTBRACKET)   \
  KEY(RIGHT_BRACKET, SDLK_RIGHTBRACKET) \
  KEY(LEFT_PAREN, SDLK_LEFTPAREN)       \
  KEY(RIGHT_PAREN, SDLK_RIGHTPAREN)     \
  KEY(SINGLE_QUOTE, SDLK_QUOTE)         \
  KEY(DOUBLE_QUOTE, SDLK_QUOTEDBL)      \
  KEY(BACK_QUOTE, SDLK_BACKQUOTE)       \
  KEY(EXCLAMATION, SDLK_EXCLAIM)        \
  KEY(AT, SDLK_AT)                      \
  KEY(HASH, SDLK_HASH)                  \
  KEY(DOLLAR, SDLK_DOLLAR)              \
  KEY(PERCENT, SDLK_PERCENT)            \
  KEY(CARET, SDLK_CARET)                \
  KEY(AMPERSAND, SDLK_AMPERSAND)        \
  KEY(ASTERISK, SDLK_ASTERISK)          \
  KEY(QUESTION, SDLK_QUESTION)          \
  KEY(PLUS, SDLK_PLUS)                  \
  KEY(MINUS, SDLK_MINUS)                \
  KEY(LESS, SDLK_LESS)                  \
  KEY(EQUALS, SDLK_EQUALS)              \
  KEY(GREATER, SDLK_GREATER)            \
  KEY(UNDERSCORE, SDLK_UNDERSCORE)      \
  KEY(UP, SDLK_UP)                      \
  KEY(DOWN, SDLK_DOWN)                  \
  KEY(LEFT, SDLK_LEFT)                  \
  KEY(RIGHT, SDLK_RIGHT)                \
  KEY(CAPS_LOCK, SDLK_CAPSLOCK)         \
  KEY(NUM_LOCK, SDLK_NUMLOCKCLEAR)      \
  KEY(SCROLL_LOCK, SDLK_SCROLLLOCK)     \
  KEY(PRINT_SCREEN, SDLK_PRINTSCREEN)   \
  KEY(PAUSE, SDLK_PAUSE)                \
  KEY(INSERT, SDLK_INSERT)              \
  KEY(DELETE, SDLK_DELETE)              \
  KEY(HOME, SDLK_HOME)                  \
  KEY(END, SDLK_END)                    \
  KEY(PAGEUP, SDLK_PAGEUP)              \
  KEY(PAGEDOWN, SDLK_PAGEDOWN)          \
  KEY(LEFT_CTRL, SDLK_LCTRL)            \
  KEY(LEFT_SHIFT, SDLK_LSHIFT)          \
  KEY(LEFT_ALT, SDLK_LALT)              \
  KEY(LEFT_GUI, SDLK_LGUI)              \
  KEY(RIGHT_CTRL, SDLK_RCTRL)           \
  KEY(RIGHT_SHIFT, SDLK_RSHIFT)         \
  KEY(RIGHT_ALT, SDLK_RALT)             \
  KEY(RIGHT_GUI, SDLK_RGUI)             \
  KEY(MODE, SDLK_MODE)                  \
  KEY(F1, SDLK_F1)                      \
  KEY(F2, SDLK_F2)                      \
  KEY(F3, SDLK_F3)                      \
  KEY(F4, SDLK_F4)                      \
  KEY(F5, SDLK_F5)                      \
  KEY(F6, SDLK_F6)                      \
  KEY(F7, SDLK_F7)                      \
  KEY(F8, SDLK_F8)                      \
  KEY(F9, SDLK_F9)                      \
  KEY(F10, SDLK_F10)                    \
  KEY(F11, SDLK_F11)                    \
  KEY(F12, SDLK_F12)                    \
  KEY(F13, SDLK_F13)                    \
  KEY(F14, SDLK_F14)                    \
  KEY(F15, SDLK_F15)                    \
  KEY(F16, SDLK_F16)                    \
  KEY(F17, SDLK_F17)                    \
  KEY(F18, SDLK_F18)                    \
  KEY(F19, SDLK_F19)                    \
  KEY(F20, SDLK_F20)                    \
  KEY(F21, SDLK_F21)                    \
  KEY(F22, SDLK_F22)                    \
  KEY(F23, SDLK_F23)                    \
  KEY(F24, SDLK_F24)

// The mapping bewteen the redux name and the SDL name for a give key modifier.
#define KEYMODS            \
  KEY(NONE, KMOD_NONE)     \
  KEY(LSHIFT, KMOD_LSHIFT) \
  KEY(RSHIFT, KMOD_RSHIFT) \
  KEY(LCTRL, KMOD_LCTRL)   \
  KEY(RCTRL, KMOD_RCTRL)   \
  KEY(LALT, KMOD_LALT)     \
  KEY(RALT, KMOD_RALT)     \
  KEY(LGUI, KMOD_LGUI)     \
  KEY(RGUI, KMOD_RGUI)     \
  KEY(NUM, KMOD_NUM)       \
  KEY(CAPS, KMOD_CAPS)     \
  KEY(MODE, KMOD_MODE)

static KeyCode ConvertSdlKeycode(SDL_Keycode sdl) {
#define KEY(X, Y) \
  case Y:         \
    return KEYCODE_##X;
  switch (sdl) {
    KEYCODES
    default:
      CHECK(false) << "Unknown SDL keycode: " << sdl;
  }
#undef KEY
}

static KeyModifier ConvertSdlKeyModifier(SDL_Keymod sdl) {
#define KEY(X, Y) static_assert(static_cast<int>(KEYMOD_##X) == Y);
  KEYMODS
#undef KEY
  return static_cast<KeyModifier>(sdl);
}

Sdl2Keyboard::Sdl2Keyboard(DeviceManager* dm) {
  KeyboardProfile profile;
  keyboard_ = dm->Connect(profile);
}

Sdl2Keyboard::~Sdl2Keyboard() { keyboard_.reset(); }

void Sdl2Keyboard::HandleEvent(SDL_Event event) {
  if (event.type == SDL_KEYDOWN) {
    keyboard_->PressKey(ConvertSdlKeycode(event.key.keysym.sym));
  }
}

void Sdl2Keyboard::Commit() {
  const SDL_Keymod mod = SDL_GetModState();
  keyboard_->SetModifierState(ConvertSdlKeyModifier(mod));
}
}  // namespace redux
