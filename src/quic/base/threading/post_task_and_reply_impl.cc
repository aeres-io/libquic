// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/post_task_and_reply_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace base {

namespace {

// This relay class remembers the sequence that it was created on, and ensures
// that both the |task| and |reply| Closures are deleted on this same sequence.
// Also, |task| is guaranteed to be deleted before |reply| is run or deleted.
//
// If RunReplyAndSelfDestruct() doesn't run because the originating execution
// context is no longer available, then the |task| and |reply| Closures are
// leaked. Leaking is considered preferable to having a thread-safetey
// violations caused by invoking the Closure destructor on the wrong sequence.
class PostTaskAndReplyRelay {
 public:
  PostTaskAndReplyRelay(OnceClosure task,
                        OnceClosure reply)
      : sequence_checker_(),
        origin_task_runner_(SequencedTaskRunnerHandle::Get()),
        reply_(std::move(reply)),
        task_(std::move(task)) {}

  ~PostTaskAndReplyRelay() {
  }

  void RunTaskAndPostReply() {
    std::move(task_).Run();
    origin_task_runner_->PostTask(
        BindOnce(&PostTaskAndReplyRelay::RunReplyAndSelfDestruct,
                             base::Unretained(this)));
  }

 private:
  void RunReplyAndSelfDestruct() {
    std::move(reply_).Run();

    // Cue mission impossible theme.
    delete this;
  }

  const SequenceChecker sequence_checker_;
  const scoped_refptr<SequencedTaskRunner> origin_task_runner_;
  OnceClosure reply_;
  OnceClosure task_;
};

}  // namespace

namespace internal {

bool PostTaskAndReplyImpl::PostTaskAndReply(OnceClosure task,
                                            OnceClosure reply) {
  PostTaskAndReplyRelay* relay =
      new PostTaskAndReplyRelay(std::move(task), std::move(reply));
  if (!PostTask(BindOnce(&PostTaskAndReplyRelay::RunTaskAndPostReply,
                                    Unretained(relay)))) {
    delete relay;
    return false;
  }

  return true;
}

}  // namespace internal

}  // namespace base
