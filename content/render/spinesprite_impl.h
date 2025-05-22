// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDER_SPINESPRITE_IMPL_H_
#define CONTENT_RENDER_SPINESPRITE_IMPL_H_

#include "components/spine2d/skeleton_renderer.h"
#include "content/context/disposable.h"
#include "content/public/engine_spinesprite.h"
#include "content/screen/renderscreen_impl.h"
#include "content/screen/viewport_impl.h"

namespace content {

struct SpineSpriteAgent {};

class SpineSpriteImpl : public SpineSprite,
                        public GraphicsChild,
                        public Disposable {
 public:
  SpineSpriteImpl(RenderScreenImpl* screen, scoped_refptr<ViewportImpl> parent);
  ~SpineSpriteImpl() override;

  SpineSpriteImpl(const SpineSpriteImpl&) = delete;
  SpineSpriteImpl& operator=(const SpineSpriteImpl&) = delete;

 public:
  void SetLabel(const std::string& label,
                ExceptionState& exception_state) override;
  void Dispose(ExceptionState& exception_state) override;
  bool IsDisposed(ExceptionState& exception_state) override;
  void Update(ExceptionState& exception_state) override;
  void SetAnimation(int32_t track_index,
                    const std::string& name,
                    bool loop,
                    ExceptionState& exception_state) override;
  void SetAnimationAlpha(int32_t track_index,
                         int32_t alpha,
                         ExceptionState& exception_state) override;
  void ClearAnimation(int32_t track_index,
                      ExceptionState& exception_state) override;
  void SetSkin(const std::vector<std::string>& skin_array,
               ExceptionState& exception_state) override;
  void SetBonePosition(const std::string& bone_name,
                       int32_t x,
                       int32_t y,
                       ExceptionState& exception_state) override;
  bool GetEventTriggered(int32_t track_index,
                         const std::string& event_name,
                         bool peek,
                         ExceptionState& exception_state) override;
  int32_t GetEventIntValue(int32_t track_index,
                           const std::string& event_name,
                           ExceptionState& exception_state) override;
  float GetEventFloatValue(int32_t track_index,
                           const std::string& event_name,
                           ExceptionState& exception_state) override;
  std::string GetEventStringValue(int32_t track_index,
                                  const std::string& event_name,
                                  ExceptionState& exception_state) override;

  URGE_EXPORT_ATTRIBUTE(Viewport, scoped_refptr<Viewport>);
  URGE_EXPORT_ATTRIBUTE(Visible, bool);
  URGE_EXPORT_ATTRIBUTE(X, int32_t);
  URGE_EXPORT_ATTRIBUTE(Y, int32_t);
  URGE_EXPORT_ATTRIBUTE(Z, int32_t);
  URGE_EXPORT_ATTRIBUTE(ZoomX, float);
  URGE_EXPORT_ATTRIBUTE(ZoomY, float);
  URGE_EXPORT_ATTRIBUTE(Mix, float);

 private:
  void OnObjectDisposed() override;
  std::string DisposedObjectName() override { return "SpineSprite"; }
  void DrawableNodeHandlerInternal(
      DrawableNode::RenderStage stage,
      DrawableNode::RenderControllerParams* params);

  DrawableNode node_;
  SpineSpriteAgent* agent_;

  scoped_refptr<ViewportImpl> viewport_;
};

}  // namespace content

#endif  //! CONTENT_RENDER_SPINESPRITE_IMPL_H_
