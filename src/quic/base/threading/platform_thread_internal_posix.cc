// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/platform_thread_internal_posix.h"

#include "base/containers/adapters.h"

namespace base {

namespace internal {

int ThreadPriorityToNiceValue(ThreadPriority priority) {
  return 0;
}

ThreadPriority NiceValueToThreadPriority(int nice_value) {
  return ThreadPriority::BACKGROUND;
}

}  // namespace internal

}  // namespace base
