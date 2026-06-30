# URGE generic compatibility layer.
# Version-independent aliases bridging URGE native API to mkxp-z style naming.
# Safe to inject for any RGSS version (1/2/3).
#
# To use: inject before any rgss*_compat.rb via:
#   rm-toolkit --inject-script 1:urge_compat.rb
#
# Loading order: urge_compat.rb → ruby1{8,9}_compat.rb → rgss*_patch.rb → rgss3_compat.rb

# --- Kernel output redirection ---
# URGE is a GUI app — stdout is invisible. Redirect to Console.
module Kernel
  def print(*args)
    Console.write(*args)
  end

  def p(*args)
    Console.puts(*args.map(&:inspect))
  end
end

# --- Kernel#caller rewrite (mkxp-z compat) ---
# Strip eval scaffolding from backtraces for cleaner error messages.
# caller is a module_function — preserve both Kernel.caller and Kernel#caller.
original_kernel_caller = Kernel.method(:caller)

Kernel.send(:define_method, :caller) do |*args|
  trace = original_kernel_caller.call(*args)
  next trace unless trace.is_a?(Array) && trace.size >= 2

  trace = trace.dup
  trace.pop    # remove "ruby:1:in 'eval'"
  trace.shift  # remove this wrapper

  trace[-1] = trace[-1].sub(/:in `<main>'/, '') if trace.any?

  trace
end

Kernel.send(:module_function, :caller)

# --- mkxp-z compatible aliases ---
# Bridge URGE module naming to mkxp-z style API (Ruby layer, no C++ changes)

module Input
  class << self
    def mouse_x
      Mouse.x
    end
    def mouse_y
      Mouse.y
    end
    def scroll_v
      Mouse.scroll_y
    end
  end
end

class << Graphics
  def show_cursor
    Mouse.visible
  end
  def show_cursor=(v)
    Mouse.visible = v
  end

  # screenshot requires Bitmap#save_png binding; planned for future
end

# System module — mkxp-z compatible wrappers on top of URGE module
module System
  class << self
    def platform
      URGE.platform
    end
    def is_windows?
      URGE.platform&.include?("Win")
    end
    def is_mac?
      URGE.platform&.include?("Mac OS")
    end
    def is_linux?
      p = URGE.platform
      p&.include?("Linux") && !p&.include?("Android")
    end
    def puts(*args)
      Console.puts(*args)
    end
    def data_directory
      Dir.pwd
    end
    def user_language
      URGE.user_language
    end
    def nproc
      URGE.nproc
    end
    def memory
      URGE.memory
    end
    def power_state
      URGE.power_state
    end
  end
end
