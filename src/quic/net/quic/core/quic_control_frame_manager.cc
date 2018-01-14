// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/core/quic_control_frame_manager.h"

#include "net/quic/core/quic_constants.h"
#include "net/quic/platform/api/quic_map_util.h"

namespace net {

QuicControlFrameManager::QuicControlFrameManager() : least_unacked_(1) {}

QuicControlFrameManager::~QuicControlFrameManager() {
  while (!control_frames_.empty()) {
    DeleteFrame(&control_frames_.front());
    control_frames_.pop_front();
  }
}

void QuicControlFrameManager::OnControlFrameSent(const QuicFrame& frame) {
  QuicControlFrameId id = GetControlFrameId(frame);
  if (id == kInvalidControlFrameId) {
    return;
  }
  if (id == least_unacked_ + control_frames_.size()) {
    // This is a newly sent control frame. Save a copy of this frame.
    switch (frame.type) {
      case RST_STREAM_FRAME:
        control_frames_.emplace_back(
            QuicFrame(new QuicRstStreamFrame(*frame.rst_stream_frame)));
        return;
      case GOAWAY_FRAME:
        control_frames_.emplace_back(
            QuicFrame(new QuicGoAwayFrame(*frame.goaway_frame)));
        return;
      case WINDOW_UPDATE_FRAME:
        control_frames_.emplace_back(
            QuicFrame(new QuicWindowUpdateFrame(*frame.window_update_frame)));
        return;
      case BLOCKED_FRAME:
        control_frames_.emplace_back(
            QuicFrame(new QuicBlockedFrame(*frame.blocked_frame)));
        return;
      case PING_FRAME:
        control_frames_.emplace_back(
            QuicFrame(QuicPingFrame(frame.ping_frame.control_frame_id)));
        return;
      default:
        return;
    }
  }
  if (QuicContainsKey(pending_retransmissions_, id)) {
    // This is retransmitted control frame.
    pending_retransmissions_.erase(id);
    return;
  }
}

void QuicControlFrameManager::OnControlFrameAcked(const QuicFrame& frame) {
  QuicControlFrameId id = GetControlFrameId(frame);
  if (id == kInvalidControlFrameId) {
    // Frame does not have a valid control frame ID, ignore it.
    return;
  }
  if (id < least_unacked_) {
    // This frame has already been acked.
    return;
  }
  if (id >= least_unacked_ + control_frames_.size()) {
    return;
  }

  // Set control frame ID of acked frames to 0.
  SetControlFrameId(kInvalidControlFrameId,
                    &control_frames_.at(id - least_unacked_));
  // Remove acked control frames from pending retransmissions.
  pending_retransmissions_.erase(id);
  // Clean up control frames queue and increment least_unacked_.
  while (!control_frames_.empty() &&
         GetControlFrameId(control_frames_.front()) == kInvalidControlFrameId) {
    DeleteFrame(&control_frames_.front());
    control_frames_.pop_front();
    ++least_unacked_;
  }
}

void QuicControlFrameManager::OnControlFrameLost(const QuicFrame& frame) {
  QuicControlFrameId id = GetControlFrameId(frame);
  if (id == kInvalidControlFrameId) {
    // Frame does not have a valid control frame ID, ignore it.
    return;
  }
  if (id < least_unacked_ ||
      GetControlFrameId(control_frames_.at(id - least_unacked_)) ==
          kInvalidControlFrameId) {
    // This frame has already been acked.
    return;
  }
  if (id >= least_unacked_ + control_frames_.size()) {
    return;
  }
  if (!QuicContainsKey(pending_retransmissions_, id)) {
    pending_retransmissions_[id] = true;
  }
}

bool QuicControlFrameManager::IsControlFrameOutstanding(
    const QuicFrame& frame) const {
  QuicControlFrameId id = GetControlFrameId(frame);
  if (id == kInvalidControlFrameId) {
    // Frame without a control frame ID should not be retransmitted.
    return false;
  }
  if (id >= least_unacked_ + control_frames_.size()) {
    return false;
  }
  return id >= least_unacked_ &&
         GetControlFrameId(control_frames_.at(id - least_unacked_)) !=
             kInvalidControlFrameId;
}

bool QuicControlFrameManager::HasPendingRetransmission() const {
  return !pending_retransmissions_.empty();
}

QuicFrame QuicControlFrameManager::NextPendingRetransmission() const {
  QuicControlFrameId id = pending_retransmissions_.begin()->first;
  return control_frames_.at(id - least_unacked_);
}

size_t QuicControlFrameManager::size() const {
  return control_frames_.size();
}

}  // namespace net
