// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_local.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"

namespace base {

namespace {

LazyInstance<ThreadLocalPointer<RunLoop::Delegate>>::Leaky tls_delegate =
    LAZY_INSTANCE_INITIALIZER;

// Runs |closure| immediately if this is called on |task_runner|, otherwise
// forwards |closure| to it.
void ProxyToTaskRunner(scoped_refptr<SequencedTaskRunner> task_runner,
                       OnceClosure closure) {
  if (task_runner->RunsTasksInCurrentSequence()) {
    std::move(closure).Run();
    return;
  }
  task_runner->PostTask(std::move(closure));
}

}  // namespace

RunLoop::Delegate::Delegate()
    : should_quit_when_idle_callback_(base::BindRepeating(
          [](Delegate* self) {
            return self->active_run_loops_.top()->quit_when_idle_received_;
          },
          Unretained(this))) {
}

RunLoop::Delegate::~Delegate() {
  // A RunLoop::Delegate may be destroyed before it is bound, if so it may still
  // be on its creation thread (e.g. a Thread that fails to start) and
  // shouldn't disrupt that thread's state.
  if (bound_)
    tls_delegate.Get().Set(nullptr);
}

bool RunLoop::Delegate::ShouldQuitWhenIdle() {
  return should_quit_when_idle_callback_.Run();
}

// static
void RunLoop::RegisterDelegateForCurrentThread(Delegate* delegate) {
  tls_delegate.Get().Set(delegate);
  delegate->bound_ = true;
}

// static
RunLoop::Delegate* RunLoop::OverrideDelegateForCurrentThreadForTesting(
    Delegate* delegate,
    Delegate::ShouldQuitWhenIdleCallback
        overriding_should_quit_when_idle_callback) {
  // Override the current Delegate (there must be one).
  Delegate* overridden_delegate = tls_delegate.Get().Get();
  overridden_delegate->should_quit_when_idle_callback_ =
      std::move(overriding_should_quit_when_idle_callback);
  tls_delegate.Get().Set(delegate);
  delegate->bound_ = true;

  return overridden_delegate;
}

RunLoop::RunLoop(Type type)
    : delegate_(tls_delegate.Get().Get()),
      type_(type),
      origin_task_runner_(ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
}

RunLoop::~RunLoop() {
}

void RunLoop::Run() {
  if (!BeforeRun())
    return;

  // It is okay to access this RunLoop from another sequence while Run() is
  // active as this RunLoop won't touch its state until after that returns (if
  // the RunLoop's state is accessed while processing Run(), it will be re-bound
  // to the accessing sequence for the remainder of that Run() -- accessing from
  // multiple sequences is still disallowed).
  DETACH_FROM_SEQUENCE(sequence_checker_);

  const bool application_tasks_allowed =
      delegate_->active_run_loops_.size() == 1U ||
      type_ == Type::kNestableTasksAllowed;
  delegate_->Run(application_tasks_allowed);

  // Rebind this RunLoop to the current thread after Run().
  DETACH_FROM_SEQUENCE(sequence_checker_);

  AfterRun();
}

void RunLoop::RunUntilIdle() {
  quit_when_idle_received_ = true;
  Run();
}

void RunLoop::Quit() {
  // Thread-safe.

  // This can only be hit if run_loop->Quit() is called directly (QuitClosure()
  // proxies through ProxyToTaskRunner() as it can only deref its WeakPtr on
  // |origin_task_runner_|).
  if (!origin_task_runner_->RunsTasksInCurrentSequence()) {
    origin_task_runner_->PostTask(
        base::BindOnce(&RunLoop::Quit, Unretained(this)));
    return;
  }

  quit_called_ = true;
  if (running_ && delegate_->active_run_loops_.top() == this) {
    // This is the inner-most RunLoop, so quit now.
    delegate_->Quit();
  }
}

void RunLoop::QuitWhenIdle() {
  // Thread-safe.

  // This can only be hit if run_loop->QuitWhenIdle() is called directly
  // (QuitWhenIdleClosure() proxies through ProxyToTaskRunner() as it can only
  // deref its WeakPtr on |origin_task_runner_|).
  if (!origin_task_runner_->RunsTasksInCurrentSequence()) {
    origin_task_runner_->PostTask(
        base::BindOnce(&RunLoop::QuitWhenIdle, Unretained(this)));
    return;
  }

  quit_when_idle_received_ = true;
}

base::Closure RunLoop::QuitClosure() {
  // Need to use ProxyToTaskRunner() as WeakPtrs vended from
  // |weak_factory_| may only be accessed on |origin_task_runner_|.
  // TODO(gab): It feels wrong that QuitClosure() is bound to a WeakPtr.
  return base::Bind(&ProxyToTaskRunner, origin_task_runner_,
                    base::Bind(&RunLoop::Quit, weak_factory_.GetWeakPtr()));
}

base::Closure RunLoop::QuitWhenIdleClosure() {
  // Need to use ProxyToTaskRunner() as WeakPtrs vended from
  // |weak_factory_| may only be accessed on |origin_task_runner_|.
  // TODO(gab): It feels wrong that QuitWhenIdleClosure() is bound to a WeakPtr.
  return base::Bind(
      &ProxyToTaskRunner, origin_task_runner_,
      base::Bind(&RunLoop::QuitWhenIdle, weak_factory_.GetWeakPtr()));
}

// static
bool RunLoop::IsRunningOnCurrentThread() {
  Delegate* delegate = tls_delegate.Get().Get();
  return delegate && !delegate->active_run_loops_.empty();
}

// static
bool RunLoop::IsNestedOnCurrentThread() {
  Delegate* delegate = tls_delegate.Get().Get();
  return delegate && delegate->active_run_loops_.size() > 1;
}

// static
void RunLoop::AddNestingObserverOnCurrentThread(NestingObserver* observer) {
  Delegate* delegate = tls_delegate.Get().Get();
  delegate->nesting_observers_.AddObserver(observer);
}

// static
void RunLoop::RemoveNestingObserverOnCurrentThread(NestingObserver* observer) {
  Delegate* delegate = tls_delegate.Get().Get();
  delegate->nesting_observers_.RemoveObserver(observer);
}

// static
bool RunLoop::IsNestingAllowedOnCurrentThread() {
  return tls_delegate.Get().Get()->allow_nesting_;
}

// static
void RunLoop::DisallowNestingOnCurrentThread() {
  tls_delegate.Get().Get()->allow_nesting_ = false;
}

// static
void RunLoop::QuitCurrentDeprecated() {
  tls_delegate.Get().Get()->active_run_loops_.top()->Quit();
}

// static
void RunLoop::QuitCurrentWhenIdleDeprecated() {
  tls_delegate.Get().Get()->active_run_loops_.top()->QuitWhenIdle();
}

// Defined out of line so that the compiler doesn't inline these and realize
// the scope has no effect and then throws an "unused variable" warning in
// non-dcheck builds.
RunLoop::ScopedDisallowRunningForTesting::ScopedDisallowRunningForTesting() =
    default;
RunLoop::ScopedDisallowRunningForTesting::~ScopedDisallowRunningForTesting() =
    default;

bool RunLoop::BeforeRun() {

  // Allow Quit to be called before Run.
  if (quit_called_)
    return false;

  auto& active_run_loops_ = delegate_->active_run_loops_;
  active_run_loops_.push(this);

  const bool is_nested = active_run_loops_.size() > 1;

  if (is_nested) {
    for (auto& observer : delegate_->nesting_observers_)
      observer.OnBeginNestedRunLoop();
    if (type_ == Type::kNestableTasksAllowed)
      delegate_->EnsureWorkScheduled();
  }

  running_ = true;
  return true;
}

void RunLoop::AfterRun() {

  running_ = false;

  auto& active_run_loops_ = delegate_->active_run_loops_;
  active_run_loops_.pop();

  RunLoop* previous_run_loop =
      active_run_loops_.empty() ? nullptr : active_run_loops_.top();

  // Execute deferred Quit, if any:
  if (previous_run_loop && previous_run_loop->quit_called_)
    delegate_->Quit();
}

}  // namespace base
