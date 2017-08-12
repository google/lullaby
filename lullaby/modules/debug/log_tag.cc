/*
Copyright 2017 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lullaby/modules/debug/log_tag.h"

#include <array>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "lullaby/util/logging.h"

namespace lull {
namespace debug {

class TagTree {
 public:
  TagTree();

  void SetEnabled(string_view tag_name, bool enabled);
  void SetEnabledBranch(string_view tag_name, bool enabled);
  bool IsEnabled(string_view tag_name);

 private:
  static constexpr const int kTagDepth = 6;

  struct TagNode {
    using TagNodePtr = std::unique_ptr<TagNode>;
    using TagNodeMap = std::unordered_map<HashValue, TagNodePtr>;

    explicit TagNode(Tag tag) : tag(tag), enabled(false) {}

    TagNode(Tag tag, bool enabled) : tag(tag), enabled(enabled) {}

    // Returns a specified child. Creates and returns one if it doesn't exist.
    TagNode* GetOrAddChild(Tag child);

    Tag tag;
    bool enabled;
    TagNodeMap children;
  };

  using TagNodePtr = TagNode::TagNodePtr;
  using TagNodeMap = TagNode::TagNodeMap;
  using Lock = std::unique_lock<std::mutex>;

  // Helper for setting current tag to enabled status.
  TagTree::TagNode* SetEnabledHelper(string_view tag_name, bool enabled);
  // Helper for setting current tag and all of its children to enabled status.
  void SetEnabledBranchHelper(TagTree::TagNode* current, bool enabled);

  std::mutex mutex_;
  TagNodePtr root_;
};

std::unique_ptr<TagTree> gTagTree(new TagTree());

TagTree::TagNode* TagTree::TagNode::GetOrAddChild(Tag child) {
  auto res = children.emplace(child.value, nullptr);
  if (res.second) {
    res.first->second.reset(new TagNode(child));
  }
  return res.first->second.get();
}

TagTree::TagTree() { root_.reset(new TagNode(Tag("."), true)); }

void TagTree::SetEnabled(string_view tag_name, bool enabled) {
  TagNode* current = SetEnabledHelper(tag_name, enabled);
  current->enabled = enabled;
}

TagTree::TagNode* TagTree::SetEnabledHelper(string_view tag_name,
                                            bool enabled) {
  std::array<Tag, kTagDepth> sub_tags;
  const size_t num = SplitTag(tag_name, sub_tags.data(), sub_tags.size());
  TagNode* current = root_.get();
  Lock lock(mutex_);
  for (size_t i = 0; i < num; ++i) {
    current = current->GetOrAddChild(sub_tags[i]);
    if (!current->enabled) {
      current->enabled = enabled;
    }
  }
  return current;
}

bool TagTree::IsEnabled(string_view tag_name) {
  std::array<Tag, kTagDepth> sub_tags;
  const size_t num = SplitTag(tag_name, sub_tags.data(), sub_tags.size());
  TagNode* current = root_.get();
  Lock lock(mutex_);
  for (size_t i = 0; i < num; ++i) {
    if (!current->enabled) {
      return false;
    }
    current = current->GetOrAddChild(sub_tags[i]);
  }
  return current->enabled;
}

void TagTree::SetEnabledBranch(string_view tag_name, bool enabled) {
  TagNode* current = SetEnabledHelper(tag_name, enabled);
  SetEnabledBranchHelper(current, enabled);
}

void TagTree::SetEnabledBranchHelper(TagTree::TagNode* current, bool enabled) {
  current->enabled = enabled;
  TagNodeMap* children = &current->children;
  for (auto& it : *children) {
    SetEnabledBranchHelper(it.second.get(), enabled);
  }
}

static size_t kMaxStringSize = 128;

static bool isValid(char c) {
  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') || c == '_' || c == '.') {
    return true;
  }
  return false;
}

size_t SplitTag(string_view tag_name, Tag* out, size_t num) {
  if (tag_name.length() == 0) {
    return 0;
  }
  size_t start = 0;
  int index = 0;
  for (size_t i = 0; i < tag_name.length(); ++i) {
    if (static_cast<size_t>(index) >= num || i > kMaxStringSize) {
      return index;
    }
    if (!isValid(tag_name[i])) {
      return 0;
    }
    if (tag_name[i] == '.') {
      if (i != start) {
        out[index++] = Tag(tag_name.substr(start, i - start));
        start = i + 1;
      } else {
        start++;
      }
    }
  }
  if (start != tag_name.length()) {
    out[index++] = Tag(tag_name.substr(start, tag_name.length() - start));
  }
  return index;
}

void InitializeLogTag() { gTagTree.reset(new TagTree()); }

void ShutdownLogTag() { gTagTree.reset(); }

void Enable(string_view tag) {
  if (gTagTree) {
    gTagTree->SetEnabled(tag, true);
  }
}

void EnableBranch(string_view tag) {
  if (gTagTree) {
    gTagTree->SetEnabledBranch(tag, true);
  }
}

void Disable(string_view tag) {
  if (gTagTree) {
    gTagTree->SetEnabled(tag, false);
  }
}

void DisableBranch(string_view tag) {
  if (gTagTree) {
    gTagTree->SetEnabledBranch(tag, false);
  }
}

bool IsEnabled(string_view tag) {
  if (gTagTree) {
    return gTagTree->IsEnabled(tag);
  }
  LOG(WARNING) << "Tagging not initialized. Please call InitializeLogTag().";
  return false;
}

}  // namespace debug
}  // namespace lull
