#pragma once

#include "graphics.hpp"

class MouseCursor {
  public:
  MouseCursor(PixelWriter* writer, PixelColor erase_color, Vector2D<int> initial_position);
  void MoveRelative(Vector2D<int> displacemednt);

  private:
  PixelWriter* pixel_writer_ = nullptr;
  PixelColor erase_color_;
  Vector2D<int> position_;
};

