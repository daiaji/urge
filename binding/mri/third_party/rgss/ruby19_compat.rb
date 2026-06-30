# Ruby 1.9.2 → 3.x 版本兼容补丁
# =================================================================
# 用途: 修复 Ruby 1.9.2 (VX / VX Ace 运行时) 到 3.x (URGE) 的 C 层行为差异
# 范围: 仅覆盖 Ruby 源码级变更，不涉及 RGSS API
#
# 【1.9.2 专有问题】
#   1.9.2 array.c: rb_obj_class(ary) + DUPSETUP → 子类保留
#   3.3.8 array.c: rb_cArray                     → 子类丢失
#   1.9.2 BasicObject#initialize 接受任意参数，3.x 仅接受 0 个
#
# 【通用兼容（非 1.9 专有，各 RGSS 版本均需要）】
#   2.4+ 合并 Fixnum/Bignum → Integer
#   2.7+ 移除 Object#taint 安全模型
# =================================================================

# ---- 1.9.2 专有: Array 子类保留 ----
# 证据链:
#   1.9.2 array.c: rb_obj_class(ary) + DUPSETUP → 子类保留
#   3.3.8 array.c: rb_cArray                     → 子类丢失
#   VX Ace F12 反射扫描 → 三方互证闭合
#
# 源码对比:
#   ary_make_shared_copy:  rb_obj_class(ary) → rb_cArray
#   rb_ary_subseq:         rb_obj_class(ary) → rb_cArray
#   rb_ary_dup_setup:      DUPSETUP 保留类    → 删除 DUPSETUP
#   uniq 内部:              rb_obj_class(ary) → rb_cArray
#   flatten 内部:           rb_class_of(ary)  → RBASIC_SET_CLASS(rb_cArray)
#
# 波及 Ruby 方法: reverse, rotate, sort, uniq, compact, flatten,
#                 shuffle, reject, take, drop

class Array
  # 避免使用 super（Ruby 3.3 mingw 上 define_method + super 对 C 方法有兼容问题），
  # 改用 instance_method + bind_call 保存原始方法后直接调用。
  %i[reverse sort uniq shuffle rotate
     compact drop flatten reject take].each do |m|
    original = instance_method(m)
    define_method(m) do |*args, &block|
      result = original.bind_call(self, *args, &block)
      if instance_of?(Array) || !result.is_a?(Array)
        result
      else
        rev = self.class.new(result)
        instance_variables.each do |v|
          rev.instance_variable_set(v, instance_variable_get(v))
        end
        rev
      end
    end
  end
end

# ---- 通用兼容: Fixnum/Bignum → Integer ----
# Ruby 2.4+ 统一 Fixnum/Bignum 为 Integer
# 原 RGSS 脚本中 is_a?(Fixnum) / is_a?(Bignum) 会 NameError
Fixnum = Integer unless defined?(Fixnum)
Bignum = Integer unless defined?(Bignum)

# ---- 通用兼容: taint 安全模型 ----
# Ruby 2.7+ 废弃, 3.0+ 移除 Object#tainted? / untrusted?
# 原 RGSS 加密档权限检查依赖这两个方法
unless ''.respond_to?(:tainted?)
  class Object
    def tainted?; false; end
    def taint; self; end
    def untaint; self; end
    def untrusted?; false; end
    def untrust; self; end
    def trust; self; end
  end
end
