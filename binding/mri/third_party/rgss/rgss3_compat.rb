# RGSS3 behavior compatibility layer.
# Bridges the gap between URGE's generic C++ APIs and VX Ace's expected behaviors.
#
# To use: inject via: rm-toolkit --inject-script 1:rgss3_compat.rb
# (place it right after rgss3_patch.rb in the script array)
#
# Requires urge_compat.rb loaded first for generic aliases.
# Loading order: urge_compat.rb → ruby1{8,9}_compat.rb → rgss*_patch.rb → rgss3_compat.rb

RGSS_VERSION = "3.0.1"

# --- Font shadow color ---
# RGSS shadow is always black. URGE defaults to black (0,0,0,128) in engine,
# but also exposes Font#shadow_color and Font.default_shadow_color for
# game authors who want custom shadow effects.
# No override needed — the C++ default is already black.

# --- ME/BGM fade interaction ---
# RGSS behavior: BGM fades out (~200ms) when ME starts, fades in (~1000ms) when ME ends.
# URGE engine provides bgm_volume/bgm_volume= and me_pos for this in Ruby.
class RPG::ME
  alias :original_me_play :play
  def play
    if @name.empty?
      Audio.me_stop
      return
    end

    # Save BGM state
    last_bgm = RPG::BGM.last
    saved = (last_bgm && !last_bgm.name.empty?) ? last_bgm.clone : nil

    if saved
      target_vol = saved.volume

      Thread.new do
        # Fade out BGM over ~200ms (20 steps x 10ms)
        steps_out = 20
        steps_out.times do |i|
          Audio.bgm_volume = target_vol * (steps_out - 1 - i) / steps_out
          sleep 0.01
        end
        Audio.bgm_volume = 0

        # Play ME; engine's MeThreadMonitorInternal will pause BGM
        Audio.me_play('Audio/ME/' + @name, @volume, @pitch)

        # Wait for ME to finish (me_pos returns 0 when stopped)
        loop do
          me_pos = Audio.me_pos
          break if me_pos.nil? || me_pos == 0
          sleep 0.05
        end

        # Fade in BGM over ~1000ms (100 steps x 10ms)
        # BGM is still paused by engine at this point; resume volume first
        steps_in = 100
        steps_in.times do |i|
          Audio.bgm_volume = target_vol * (i + 1) / steps_in
          sleep 0.01
        end
        Audio.bgm_volume = target_vol
      end
    else
      original_me_play
    end
  end
end

# --- Kernel#msgbox / msgbox_p ---
# RGSS3 kernel functions for popup messages.
# Currently logs to Console; upgrade to native MessageBox when
# C++ binding for URGE.alert / SDL_ShowSimpleMessageBox is wired.
module Kernel
  def msgbox(*args)
    Console.puts(*args)
  end

  def msgbox_p(*args)
    Console.puts(*args.map(&:inspect))
  end
end

class << Graphics
  alias :original_resize_screen :resize_screen
  def resize_screen(width, height)
    original_resize_screen(width, height)
    rect = window_rect
    move_window(rect.x, rect.y, width, height)
  end
end

# --- Bitmap#text_size ---
# C++ textSize already performs "str + space" width compensation internally.
# No additional Ruby-level adjustment needed.
class Bitmap
  alias :original_text_size :text_size
  def text_size(str)
    return original_text_size(str) unless str.is_a?(String)
    original_text_size(str)
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
      temp = Bitmap.new(tw, h)
      temp.font = font.clone
      temp.original_draw_text(0, 0, tw, h, str, 0)
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

    # Pass align=0 (Left) because x is already adjusted above.
    original_draw_text(x, y, w, h, str, 0)
  end
end
