// Copyright 2018-2025 Admenri.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/render/drawable_controller.h"

namespace content {

// Using global pmr allocator.
// ref: https://zhuanlan.zhihu.com/p/359409607
//
static std::array<char, 1024 * 1024 * 8> array_buffer;
static std::pmr::monotonic_buffer_resource buffer_resource{array_buffer.data(),
                                                           array_buffer.size()};
static std::pmr::unsynchronized_pool_resource pool_resource{
    std::pmr::pool_options{32, 256}, &buffer_resource};

static int64_t g_creation_stamp = 0;

SortKey::SortKey() : weight{0, 0, ++g_creation_stamp} {}

SortKey::SortKey(int64_t key1) : weight{key1, 0, ++g_creation_stamp} {}

SortKey::SortKey(int64_t key1, int64_t key2)
    : weight{key1, key2, ++g_creation_stamp} {}

SortKey::SortKey(int64_t key1, int64_t key2, int64_t key3)
    : weight{key1, key2, key3} {}

DrawableNode::DrawableNode(const SortKey& default_sort_weight)
    : key_(default_sort_weight), node_visibility_(true), controller_(nullptr) {}

DrawableNode::~DrawableNode() {
  DisposeNode();
}

void DrawableNode::RegisterEventHandler(const NotificationHandler& handler) {
  DCHECK(!handler.is_null());
  DCHECK(handler_.is_null());

  handler_ = handler;
}

void DrawableNode::RebindController(DrawNodeController* controller) {
  // Rebind associated node link
  base::LinkNode<DrawableNode>::RemoveFromList();
  controller->associated_nodes_.Append(this);

  // Bind to new controller and unbind with old controller.
  if (controller_) {
    auto node = controller_->nodes_.extract(key_);
    DCHECK(!node.empty());

    // Bind to new contrainer
    controller->nodes_.insert(std::move(node));
  } else {
    // First bind to new controller
    controller_ = controller;
    controller_->nodes_.emplace(key_, this);
  }
}

void DrawableNode::DisposeNode() {
  base::LinkNode<DrawableNode>::RemoveFromList();

  if (controller_) {
    controller_->nodes_.erase(key_);
    controller_ = nullptr;
  }
}

void DrawableNode::SetNodeVisibility(bool visible) {
  node_visibility_ = visible;
}

bool DrawableNode::GetVisibility() const {
  return node_visibility_;
}

void DrawableNode::SetNodeSortWeight(int weight1) {
  if (!controller_)
    return;

  auto current_node = controller_->nodes_.extract(key_);
  if (current_node.empty())
    return;

  key_.weight[0] = weight1;

  current_node.key() = key_;
  controller_->nodes_.insert(std::move(current_node));
}

void DrawableNode::SetNodeSortWeight(int weight1, int weight2) {
  if (!controller_)
    return;

  auto current_node = controller_->nodes_.extract(key_);
  if (current_node.empty())
    return;

  key_.weight[0] = weight1;
  key_.weight[1] = weight2;

  current_node.key() = key_;
  controller_->nodes_.insert(std::move(current_node));
}

void DrawableNode::SetNodeSortWeight(int weight1, int weight2, int weight3) {
  if (!controller_)
    return;

  auto current_node = controller_->nodes_.extract(key_);
  if (current_node.empty())
    return;

  key_.weight[0] = weight1;
  key_.weight[1] = weight2;
  key_.weight[2] = weight3;

  current_node.key() = key_;
  controller_->nodes_.insert(std::move(current_node));
}

DrawNodeController::DrawNodeController() : nodes_(&pool_resource) {}

DrawNodeController::~DrawNodeController() {
  for (auto& it : nodes_)
    it.second->controller_ = nullptr;
}

void DrawNodeController::BroadCastNotification(
    DrawableNode::RenderStage nid,
    DrawableNode::RenderControllerParams* params) {
  if (nid == DrawableNode::RenderStage::kOnRendering) {
    for (auto& it : nodes_) {
      // Filter notification request
      if (!it.second->node_visibility_)
        continue;

      // Broadcast render notification
      it.second->handler_.Run(nid, params);
    }
  } else {
    for (auto* it = associated_nodes_.head(); it != associated_nodes_.end();
         it = it->next()) {
      // Broadcast non-render notification
      if (it->value()->node_visibility_)
        it->value()->handler_.Run(nid, params);
    }
  }
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
