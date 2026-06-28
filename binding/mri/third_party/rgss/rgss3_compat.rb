# RGSS3 behavior compatibility layer.
# Bridges the gap between URGE's generic C++ APIs and VX Ace's expected behaviors.
#
# To use: this file is auto-loaded via rgss3_patch.rb's require_relative.
# Or require it directly in your script editor.

class << Graphics
  alias :original_resize_screen :resize_screen
  def resize_screen(width, height)
    original_resize_screen(width, height)
    move_window(0, 0, width, height)
  end
end
