# RGSS3 behavior compatibility layer.
# Bridges the gap between URGE's generic C++ APIs and VX Ace's expected behaviors.
#
# To use: inject via: rm-toolkit --inject-script 1:rgss3_compat.rb
# (place it right after rgss3_patch.rb in the script array)

class << Graphics
  alias :original_resize_screen :resize_screen
  def resize_screen(width, height)
    original_resize_screen(width, height)
    rect = window_rect
    move_window(rect.x, rect.y, width, height)
  end
end
