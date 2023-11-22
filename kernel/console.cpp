/**
 * @file console.cpp
 *
 * コンソール描画のプログラムを集めたファイル．
 */

#include "console.hpp"

#include <cstring>
#include "font.hpp"

// #@@range_begin(constructor)
Console::Console(PixelWriter& writer,
    const PixelColor& fg_color, const PixelColor& bg_color)
    : writer_{writer}, fg_color_{fg_color}, bg_color_{bg_color},
      buffer_{}, cursor_row_{0}, cursor_column_{0} {
}
// #@@range_end(constructor)

// #@@range_begin(put_string)
void Console::PutString(const char* s) {
  while (*s) {
    if (*s == '\n') {
      Newline();
    } else if (cursor_column_ < kColumns - 1) {
      WriteAscii(writer_, 8 * cursor_column_, 16 * cursor_row_, *s, fg_color_);
      buffer_[cursor_row_][cursor_column_] = *s;
      ++cursor_column_;
    }
    ++s;
  }
}
// #@@range_end(put_string)

// #@@range_begin(newline)
void Console::Newline() {
  cursor_column_ = 0;
  if (cursor_row_ < kRows - 1) { // 現在のカーソル位置がまだ最下行に達していないなら
    ++cursor_row_; // カーソルを一行下に移動させる
  } else {
    // 表示領域全域を塗りつぶす
    for (int y = 0; y < 16 * kRows; ++y) {
      for (int x = 0; x < 8 * kColumns; ++x) {
        writer_.Write(x, y, bg_color_);
      }
    }
    for (int row = 0; row < kRows - 1; ++row) {
      // row + 1行目の文字列をrow行目にコピーする
      memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
      // row行目を画面に描画する
      WriteString(writer_, 0, 16 * row, buffer_[row], fg_color_);
    }
    // 最下行をNULL文字で初期化する
    memset(buffer_[kRows - 1], 0, kColumns + 1);
  }
}
// #@@range_end(newline)
