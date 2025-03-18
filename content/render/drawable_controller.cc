// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/drawable_controller.h"

namespace content {

static int64_t g_creation_stamp = 0;

SortKey::SortKey() : weight{0, 0, ++g_creation_stamp} {}

SortKey::SortKey(int64_t key1) : weight{key1, 0, ++g_creation_stamp} {}

SortKey::SortKey(int64_t key1, int64_t key2)
    : weight{key1, key2, ++g_creation_stamp} {}

SortKey::SortKey(int64_t key1, int64_t key2, int64_t key3)
    : weight{key1, key2, key3} {}

DrawableNode::DrawableNode(DrawNodeController* controller,
                           const SortKey& default_key,
                           bool visible)
    : controller_(controller), key_(default_key), visible_(visible) {
  if (controller_) {
    // Bind this node to initial parent.
    controller_->children_list_.Append(this);
  }
}

DrawableNode::~DrawableNode() {
  DisposeNode();
}

void DrawableNode::RegisterEventHandler(const NotificationHandler& handler) {
  DCHECK(!handler.is_null() && handler_.is_null());
  handler_ = handler;
}

void DrawableNode::RebindController(DrawNodeController* controller) {
  if (controller_ == controller_)
    return;

  // Setup new parent controller
  controller_ = controller;

  // Rebind orderly node link
  base::LinkNode<DrawableNode>::RemoveFromList();
  controller_->InsertChildNodeInternal(this);
}

void DrawableNode::DisposeNode() {
  DCHECK(controller_);
  base::LinkNode<DrawableNode>::RemoveFromList();
  controller_ = nullptr;
}

void DrawableNode::SetNodeVisibility(bool visible) {
  visible_ = visible;
}

bool DrawableNode::GetVisibility() const {
  return visible_;
}

void DrawableNode::SetNodeSortWeight(int weight1) {
  if (!controller_)
    return;

  key_.weight[0] = weight1;

  base::LinkNode<DrawableNode>::RemoveFromList();
  controller_->InsertChildNodeInternal(this);
}

void DrawableNode::SetNodeSortWeight(int weight1, int weight2) {
  if (!controller_)
    return;

  key_.weight[0] = weight1;
  key_.weight[1] = weight2;

  base::LinkNode<DrawableNode>::RemoveFromList();
  controller_->InsertChildNodeInternal(this);
}

void DrawableNode::SetNodeSortWeight(int weight1, int weight2, int weight3) {
  if (!controller_)
    return;

  key_.weight[0] = weight1;
  key_.weight[1] = weight2;
  key_.weight[2] = weight3;

  base::LinkNode<DrawableNode>::RemoveFromList();
  controller_->InsertChildNodeInternal(this);
}

DrawableNode* DrawableNode::GetPreviousNode() {
  auto* node = previous();
  return node ? node->value() : nullptr;
}

DrawableNode* DrawableNode::GetNextNode() {
  auto* node = next();
  return node ? node->value() : nullptr;
}

DrawNodeController::DrawNodeController() = default;

DrawNodeController::~DrawNodeController() {
  for (auto* it = children_list_.head(); it != children_list_.end();
       it = it->next()) {
    // Reset child controller association.
    it->value()->controller_ = nullptr;
  }
}

void DrawNodeController::BroadCastNotification(
    DrawableNode::RenderStage nid,
    DrawableNode::RenderControllerParams* params) {
  for (auto* it = children_list_.head(); it != children_list_.end();
       it = it->next()) {
    // Broadcast render job notification
    if (it->value()->visible_)
      it->value()->handler_.Run(nid, params);
  }
}

void DrawNodeController::InsertChildNodeInternal(DrawableNode* node) {
  // From min to max.
  for (auto* it = children_list_.head(); it != children_list_.end();
       it = it->next()) {
    // Insert before the first node with a greater key.
    // This will keep the list sorted in ascending order.
    if (node->key_ < it->value()->key_)
      return node->InsertBefore(it);
  }

  // Max key, append to the end.
  children_list_.Append(node);
}

void DrawableFlashController::Setup(
    const std::optional<base::Vec4>& flash_color,
    int32_t duration) {
  if (duration > 0) {
    duration_ = duration;
    count_ = 0;
    color_ = flash_color.has_value() ? flash_color.value() : base::Vec4();
    alpha_ = color_.w;
    invalid_ = !flash_color.has_value();
  }
}

void DrawableFlashController::Update() {
  if (duration_) {
    if (++count_ > duration_) {
      duration_ = 0;
      invalid_ = false;
      return;
    } else {
      float progress = static_cast<float>(count_) / duration_;
      color_.w = alpha_ * (1.0f - progress);
    }
  }
}

}  // namespace content
