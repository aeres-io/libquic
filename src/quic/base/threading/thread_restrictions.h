// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_THREADING_THREAD_RESTRICTIONS_H_
#define BASE_THREADING_THREAD_RESTRICTIONS_H_

#include "base/base_export.h"
#include "base/macros.h"



class BrowserProcessImpl;
class HistogramSynchronizer;
class NativeBackendKWallet;
class KeyStorageLinux;

namespace android_webview {
class AwFormDatabaseService;
class CookieManager;
}

namespace cc {
class CompletionEvent;
class SingleThreadTaskGraphRunner;
}
namespace chromeos {
class BlockingMethodCaller;
namespace system {
class StatisticsProviderImpl;
}
}
namespace chrome_browser_net {
class Predictor;
}
namespace content {
class BrowserGpuChannelHostFactory;
class BrowserGpuMemoryBufferManager;
class BrowserMainLoop;
class BrowserShutdownProfileDumper;
class BrowserSurfaceViewManager;
class BrowserTestBase;
class CategorizedWorkerPool;
class NestedMessagePumpAndroid;
class ScopedAllowWaitForAndroidLayoutTests;
class ScopedAllowWaitForDebugURL;
class SoftwareOutputDeviceMus;
class SynchronousCompositor;
class SynchronousCompositorBrowserFilter;
class SynchronousCompositorHost;
class TextInputClientMac;
}  // namespace content
namespace cronet {
class CronetPrefsManager;
class CronetURLRequestContextAdapter;
}  // namespace cronet
namespace dbus {
class Bus;
}
namespace disk_cache {
class BackendImpl;
class InFlightIO;
}
namespace gpu {
class GpuChannelHost;
}
namespace leveldb {
class LevelDBMojoProxy;
}
namespace media {
class BlockingUrlProtocol;
}
namespace mojo {
class SyncCallRestrictions;
namespace edk {
class ScopedIPCSupport;
}
}
namespace rlz_lib {
class FinancialPing;
}
namespace ui {
class CommandBufferClientImpl;
class CommandBufferLocal;
class GpuState;
}
namespace net {
class MultiThreadedCertVerifierScopedAllowBaseSyncPrimitives;
class NetworkChangeNotifierMac;
namespace internal {
class AddressTrackerLinux;
}
}

namespace remoting {
class AutoThread;
}

namespace resource_coordinator {
class TabManagerDelegate;
}

namespace shell_integration {
class LaunchXdgUtilityScopedAllowBaseSyncPrimitives;
}

namespace ui {
class WindowResizeHelperMac;
}

namespace views {
class ScreenMus;
}

namespace viz {
class ServerGpuMemoryBufferManager;
}

namespace base {

namespace android {
class JavaHandlerThread;
}

namespace internal {
class TaskTracker;
}

class GetAppOutputScopedAllowBaseSyncPrimitives;
class SimpleThread;
class StackSamplingProfiler;
class Thread;
class ThreadTestHelper;

// A "blocking call" refers to any call that causes the calling thread to wait
// off-CPU. It includes but is not limited to calls that wait on synchronous
// file I/O operations: read or write a file from disk, interact with a pipe or
// a socket, rename or delete a file, enumerate files in a directory, etc.
// Acquiring a low contention lock is not considered a blocking call.

// Asserts that blocking calls are allowed in the current scope.
//
// Style tip: It's best if you put AssertBlockingAllowed() checks as close to
// the blocking call as possible. For example:
//
// void ReadFile() {
//   PreWork();
//
//   base::AssertBlockingAllowed();
//   fopen(...);
//
//   PostWork();
// }
//
// void Bar() {
//   ReadFile();
// }
//
// void Foo() {
//   Bar();
// }
inline void AssertBlockingAllowed() {}

// Disallows blocking on the current thread.
inline void DisallowBlocking() {}

// Disallows blocking calls within its scope.
class BASE_EXPORT ScopedDisallowBlocking {
 public:
   ScopedDisallowBlocking() {}
   ~ScopedDisallowBlocking() {}

 private:
};

// ScopedAllowBlocking(ForTesting) allow blocking calls within a scope where
// they are normally disallowed.
//
// Avoid using this. Prefer making blocking calls from tasks posted to
// base::TaskScheduler with base::MayBlock().
class BASE_EXPORT ScopedAllowBlocking {
 private:
  friend class cronet::CronetPrefsManager;
  friend class cronet::CronetURLRequestContextAdapter;
  friend class resource_coordinator::TabManagerDelegate;  // crbug.com/778703
  friend class ScopedAllowBlockingForTesting;

  ScopedAllowBlocking() {}
  ~ScopedAllowBlocking() {}
};

class ScopedAllowBlockingForTesting {
 public:
  ScopedAllowBlockingForTesting() {}
  ~ScopedAllowBlockingForTesting() {}

 private:
};

// "Waiting on a //base sync primitive" refers to calling one of these methods:
// - base::WaitableEvent::*Wait*
// - base::ConditionVariable::*Wait*
// - base::Process::WaitForExit*

// Disallows waiting on a //base sync primitive on the current thread.
inline void DisallowBaseSyncPrimitives() {}

// ScopedAllowBaseSyncPrimitives(ForTesting)(OutsideBlockingScope) allow waiting
// on a //base sync primitive within a scope where this is normally disallowed.
//
// Avoid using this.
//
// Instead of waiting on a WaitableEvent or a ConditionVariable, put the work
// that should happen after the wait in a callback and post that callback from
// where the WaitableEvent or ConditionVariable would have been signaled. If
// something needs to be scheduled after many tasks have executed, use
// base::BarrierClosure.
//
// On Windows, join processes asynchronously using base::win::ObjectWatcher.

// This can only be used in a scope where blocking is allowed.
class BASE_EXPORT ScopedAllowBaseSyncPrimitives {
 private:
  // This can only be instantiated by friends. Use
  // ScopedAllowBaseSyncPrimitivesForTesting in unit tests to avoid the friend
  // requirement.
 

  friend class base::GetAppOutputScopedAllowBaseSyncPrimitives;
  friend class leveldb::LevelDBMojoProxy;
  friend class media::BlockingUrlProtocol;
  friend class net::MultiThreadedCertVerifierScopedAllowBaseSyncPrimitives;
  friend class rlz_lib::FinancialPing;
  friend class shell_integration::LaunchXdgUtilityScopedAllowBaseSyncPrimitives;

  ScopedAllowBaseSyncPrimitives() {}
  ~ScopedAllowBaseSyncPrimitives() {}
};

// This can be used in a scope where blocking is disallowed.
class BASE_EXPORT ScopedAllowBaseSyncPrimitivesOutsideBlockingScope {
 private:
  friend class ::KeyStorageLinux;

  ScopedAllowBaseSyncPrimitivesOutsideBlockingScope() {};
  ~ScopedAllowBaseSyncPrimitivesOutsideBlockingScope() {};
};

// This can be used in tests without being a friend of
// ScopedAllowBaseSyncPrimitives(OutsideBlockingScope).
class BASE_EXPORT ScopedAllowBaseSyncPrimitivesForTesting {
 public:
   ScopedAllowBaseSyncPrimitivesForTesting() {}
   ~ScopedAllowBaseSyncPrimitivesForTesting() {}

 private:
};

namespace internal {

// Asserts that waiting on a //base sync primitive is allowed in the current
// scope.
inline void AssertBaseSyncPrimitivesAllowed() {}

// Resets all thread restrictions on the current thread.
inline void ResetThreadRestrictionsForTesting() {}

}  // namespace internal

class BASE_EXPORT ThreadRestrictions {
 public:
  // Constructing a ScopedAllowIO temporarily allows IO for the current
  // thread.  Doing this is almost certainly always incorrect.
  //
  // DEPRECATED. Use ScopedAllowBlocking(ForTesting).
  class BASE_EXPORT ScopedAllowIO {
   public:
    ScopedAllowIO() { previous_value_ = SetIOAllowed(true); }
    ~ScopedAllowIO() { SetIOAllowed(previous_value_); }
   private:
    // Whether IO is allowed when the ScopedAllowIO was constructed.
    bool previous_value_;

    DISALLOW_COPY_AND_ASSIGN(ScopedAllowIO);
  };

  // Inline the empty definitions of these functions so that they can be
  // compiled out.
  static bool SetIOAllowed(bool allowed) { return true; }
  static bool SetSingletonAllowed(bool allowed) { return true; }
  static void AssertSingletonAllowed() {}
  static void DisallowWaiting() {}

 private:
  // DO NOT ADD ANY OTHER FRIEND STATEMENTS, talk to jam or brettw first.
  // BEGIN ALLOWED USAGE.
  friend class android_webview::AwFormDatabaseService;
  friend class android_webview::CookieManager;
  friend class base::StackSamplingProfiler;
  friend class content::BrowserMainLoop;
  friend class content::BrowserShutdownProfileDumper;
  friend class content::BrowserSurfaceViewManager;
  friend class content::BrowserTestBase;
  friend class content::NestedMessagePumpAndroid;
  friend class content::ScopedAllowWaitForAndroidLayoutTests;
  friend class content::ScopedAllowWaitForDebugURL;
  friend class content::SynchronousCompositor;
  friend class content::SynchronousCompositorBrowserFilter;
  friend class content::SynchronousCompositorHost;
  friend class ::HistogramSynchronizer;
  friend class internal::TaskTracker;
  friend class cc::CompletionEvent;
  friend class cc::SingleThreadTaskGraphRunner;
  friend class content::CategorizedWorkerPool;
  friend class remoting::AutoThread;
  friend class ui::WindowResizeHelperMac;
  friend class MessagePumpDefault;
  friend class SimpleThread;
  friend class Thread;
  friend class ThreadTestHelper;
  friend class PlatformThread;
  friend class android::JavaHandlerThread;
  friend class mojo::SyncCallRestrictions;
  friend class mojo::edk::ScopedIPCSupport;
  friend class ui::CommandBufferClientImpl;
  friend class ui::CommandBufferLocal;
  friend class ui::GpuState;

  // END ALLOWED USAGE.
  // BEGIN USAGE THAT NEEDS TO BE FIXED.
  friend class ::chromeos::BlockingMethodCaller;  // http://crbug.com/125360
  friend class ::chromeos::system::StatisticsProviderImpl;  // http://crbug.com/125385
  friend class chrome_browser_net::Predictor;     // http://crbug.com/78451
  friend class
      content::BrowserGpuChannelHostFactory;      // http://crbug.com/125248
  friend class
      content::BrowserGpuMemoryBufferManager;     // http://crbug.com/420368
  friend class content::TextInputClientMac;       // http://crbug.com/121917
  friend class dbus::Bus;                         // http://crbug.com/125222
  friend class disk_cache::BackendImpl;           // http://crbug.com/74623
  friend class disk_cache::InFlightIO;            // http://crbug.com/74623
  friend class gpu::GpuChannelHost;               // http://crbug.com/125264
  friend class net::internal::AddressTrackerLinux;  // http://crbug.com/125097
  friend class net::NetworkChangeNotifierMac;     // http://crbug.com/125097
  friend class ::BrowserProcessImpl;              // http://crbug.com/125207
  friend class ::NativeBackendKWallet;            // http://crbug.com/125331
#if !defined(OFFICIAL_BUILD)
  friend class content::SoftwareOutputDeviceMus;  // Interim non-production code
#endif
  friend class views::ScreenMus;
  friend class viz::ServerGpuMemoryBufferManager;
// END USAGE THAT NEEDS TO BE FIXED.

  static bool SetWaitAllowed(bool allowed) { return true; }

  // Constructing a ScopedAllowWait temporarily allows waiting on the current
  // thread.  Doing this is almost always incorrect, which is why we limit who
  // can use this through friend. If you find yourself needing to use this, find
  // another way. Talk to jam or brettw.
  //
  // DEPRECATED. Use ScopedAllowBaseSyncPrimitives.
  class BASE_EXPORT ScopedAllowWait {
   public:
    ScopedAllowWait() { previous_value_ = SetWaitAllowed(true); }
    ~ScopedAllowWait() { SetWaitAllowed(previous_value_); }
   private:
    // Whether singleton use is allowed when the ScopedAllowWait was
    // constructed.
    bool previous_value_;

    DISALLOW_COPY_AND_ASSIGN(ScopedAllowWait);
  };

  DISALLOW_IMPLICIT_CONSTRUCTORS(ThreadRestrictions);
};

}  // namespace base

#endif  // BASE_THREADING_THREAD_RESTRICTIONS_H_
