// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/spinesprite_impl.h"

namespace content {

SpineSpriteImpl::SpineSpriteImpl(
    RenderScreenImpl* screen,
    spine::SkeletonData* skeleton_data,
    spine::AnimationStateData* animation_state_data,
    float default_mix)
    : GraphicsChild(screen),
      Disposable(screen),
      node_(screen->GetDrawableController(), SortKey()) {
  node_.RegisterEventHandler(base::BindRepeating(
      &SpineSpriteImpl::DrawableNodeHandlerInternal, base::Unretained(this)));
}

SpineSpriteImpl::~SpineSpriteImpl() {
  ExceptionState exception_state;
  Dispose(exception_state);
}

void SpineSpriteImpl::SetLabel(const std::string& label,
                               ExceptionState& exception_state) {
  node_.SetDebugLabel(label);
}

void SpineSpriteImpl::Dispose(ExceptionState& exception_state) {
  Disposable::Dispose(exception_state);
}

bool SpineSpriteImpl::IsDisposed(ExceptionState& exception_state) {
  return Disposable::IsDisposed(exception_state);
}

void SpineSpriteImpl::Update(ExceptionState& exception_state) {}

void SpineSpriteImpl::SetAnimation(int32_t track_index,
                                   const std::string& name,
                                   bool loop,
                                   ExceptionState& exception_state) {}

void SpineSpriteImpl::SetAnimationAlpha(int32_t track_index,
                                        int32_t alpha,
                                        ExceptionState& exception_state) {}

void SpineSpriteImpl::ClearAnimation(int32_t track_index,
                                     ExceptionState& exception_state) {}

void SpineSpriteImpl::SetSkin(const std::vector<std::string>& skin_array,
                              ExceptionState& exception_state) {}

void SpineSpriteImpl::SetBonePosition(const std::string& bone_name,
                                      int32_t x,
                                      int32_t y,
                                      ExceptionState& exception_state) {}

bool SpineSpriteImpl::GetEventTriggered(int32_t track_index,
                                        const std::string& event_name,
                                        bool peek,
                                        ExceptionState& exception_state) {
  return false;
}

int32_t SpineSpriteImpl::GetEventIntValue(int32_t track_index,
                                          const std::string& event_name,
                                          ExceptionState& exception_state) {
  return 0;
}

float SpineSpriteImpl::GetEventFloatValue(int32_t track_index,
                                          const std::string& event_name,
                                          ExceptionState& exception_state) {
  return 0.0f;
}

std::string SpineSpriteImpl::GetEventStringValue(
    int32_t track_index,
    const std::string& event_name,
    ExceptionState& exception_state) {
  return std::string();
}

scoped_refptr<Viewport> SpineSpriteImpl::Get_Viewport(
    ExceptionState& exception_state) {
  return viewport_;
}

void SpineSpriteImpl::Put_Viewport(const scoped_refptr<Viewport>& value,
                                   ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  if (viewport_ == value)
    return;

  viewport_ = ViewportImpl::From(value);
  node_.RebindController(viewport_ ? viewport_->GetDrawableController()
                                   : screen()->GetDrawableController());
}

bool SpineSpriteImpl::Get_Visible(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return false;

  return node_.GetVisibility();
}

void SpineSpriteImpl::Put_Visible(const bool& value,
                                  ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  node_.SetNodeVisibility(value);
}

float SpineSpriteImpl::Get_X(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return false;

  return skeleton_->getX();
}

void SpineSpriteImpl::Put_X(const float& value,
                            ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  skeleton_->setX(value);
}

float SpineSpriteImpl::Get_Y(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return false;

  return skeleton_->getY();
}

void SpineSpriteImpl::Put_Y(const float& value,
                            ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  skeleton_->setY(value);
}

int32_t SpineSpriteImpl::Get_Z(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return node_.GetSortKeys()->weight[0];
}

void SpineSpriteImpl::Put_Z(const int32_t& value,
                            ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  node_.SetNodeSortWeight(value);
}

float SpineSpriteImpl::Get_ZoomX(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return skeleton_->getScaleX();
}

void SpineSpriteImpl::Put_ZoomX(const float& value,
                                ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  skeleton_->setScaleX(value);
}

float SpineSpriteImpl::Get_ZoomY(ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return 0;

  return skeleton_->getScaleY();
}

void SpineSpriteImpl::Put_ZoomY(const float& value,
                                ExceptionState& exception_state) {
  if (CheckDisposed(exception_state))
    return;

  skeleton_->setScaleY(value);
}

void SpineSpriteImpl::OnObjectDisposed() {
  node_.DisposeNode();
}

void SpineSpriteImpl::DrawableNodeHandlerInternal(
    DrawableNode::RenderStage stage,
    DrawableNode::RenderControllerParams* params) {}

}  // namespace content
