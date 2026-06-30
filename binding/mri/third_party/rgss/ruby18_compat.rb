# Ruby 1.8 → 1.9+ 版本兼容补丁
# =================================================================
# 用途: 修复 Ruby 1.8.x (RGSS1/RMXP 运行时) 到 3.x (URGE) 的 C 层行为差异
# 范围: 仅覆盖 Ruby 源码级变更，不涉及 RGSS API
#
# 参考: mkxp-z scripts/preload/ruby_classic_wrap.rb (WaywardHeart, 2023, CC0)
# =================================================================

class Hash
  alias_method :index, :key unless method_defined?(:index)
end

class Object
  TRUE = true unless const_defined?("TRUE")
  FALSE = false unless const_defined?("FALSE")
  NIL = nil unless const_defined?("NIL")

  alias_method :id, :object_id unless method_defined?(:id)
  alias_method :type, :class unless method_defined?(:type)
end

class NilClass
  def id
    4
  end
end

class TrueClass
  def id
    2
  end
end
