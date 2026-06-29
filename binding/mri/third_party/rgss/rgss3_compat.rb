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

# --- Bitmap#text_size space compensation ---
# FreeType sometimes overestimates the last character's width;
# appending a space and subtracting its width improves accuracy.
class Bitmap
  alias :original_text_size :text_size
  def text_size(str)
    return original_text_size(str) unless str.is_a?(String)
    w = original_text_size(str + " ")
    ws = original_text_size(" ")
    Rect.new(0, 0, [w.width - ws.width, 0].max, w.height)
  end
end

# --- Bitmap#draw_text alignment with outline compensation ---
# Center/Right alignment needs to account for outline stroke
# width to avoid visual offset. Also handles text squeezing
# when content exceeds the rect width.
class Bitmap
  alias :original_draw_text :draw_text
  def draw_text(*args)
    case args.size
    when 3
      rect, str, align = args[0], args[1], args[2]
      x, y, w, h = rect.x, rect.y, rect.width, rect.height
    when 5
      x, y, w, h, str = args[0], args[1], args[2], args[3], args[4]
      align = 0
    when 6
      x, y, w, h, str, align = args[0], args[1], args[2], args[3], args[4], args[5]
    else
      return original_draw_text(*args)
    end

    return original_draw_text(*args) unless str.is_a?(String)

    outline_w = font.outline ? 1 : 0

    # Squeeze: if text is wider than rect, scale width to fit
    tw = text_size(str).width
    if tw > 0 && tw > w
      zoom = w.to_f / tw
      # Render to temp bitmap at original size, then stretch_blt squeezed
      temp = Bitmap.new(tw, h)
      temp.font = font.clone
      case align
      when 0 then temp.draw_text(0, 0, tw, h, str, 0)
      when 1 then temp.draw_text(0, 0, tw, h, str, 1)
      when 2 then temp.draw_text(0, 0, tw, h, str, 2)
      end
      stretch_blt(Rect.new(x, y, (tw * zoom).to_i, h), temp, temp.rect)
      temp.dispose
      return
    end

    # Adjust alignment for outline
    case align
    when 1  # Center
      x += (w - (tw + outline_w)) / 2
    when 2  # Right
      x += w - tw - outline_w * 2
    end

    original_draw_text(x, y, w, h, str, align)
  end
end
