// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/load_timing_info.h"

namespace net {

LoadTimingInfo::ConnectTiming::ConnectTiming() = default;

LoadTimingInfo::ConnectTiming::~ConnectTiming() = default;

LoadTimingInfo::LoadTimingInfo()
    : socket_reused(false) {}

LoadTimingInfo::LoadTimingInfo(const LoadTimingInfo& other) = default;

LoadTimingInfo::~LoadTimingInfo() = default;

}  // namespace net
