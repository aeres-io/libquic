// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/host_port_pair.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/base/ip_endpoint.h"

namespace net {

HostPortPair::HostPortPair() : port_(0) {}
HostPortPair::HostPortPair(const std::string& in_host, uint16_t in_port)
    : host_(in_host), port_(in_port) {
}


}  // namespace net
