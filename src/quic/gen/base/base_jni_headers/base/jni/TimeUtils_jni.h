// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is autogenerated by
//     base/android/jni_generator/jni_generator.py
// For
//     org/chromium/base/TimeUtils

#ifndef org_chromium_base_TimeUtils_JNI
#define org_chromium_base_TimeUtils_JNI

#include <jni.h>

#include "../../../../../../../base/android/jni_generator/jni_generator_helper.h"

// Step 1: forward declarations.
JNI_REGISTRATION_EXPORT extern const char
    kClassPath_org_chromium_base_TimeUtils[];
const char kClassPath_org_chromium_base_TimeUtils[] =
    "org/chromium/base/TimeUtils";

// Leaking this jclass as we cannot use LazyInstance from some threads.
JNI_REGISTRATION_EXPORT base::subtle::AtomicWord
    g_org_chromium_base_TimeUtils_clazz = 0;
#ifndef org_chromium_base_TimeUtils_clazz_defined
#define org_chromium_base_TimeUtils_clazz_defined
inline jclass org_chromium_base_TimeUtils_clazz(JNIEnv* env) {
  return base::android::LazyGetClass(env,
      kClassPath_org_chromium_base_TimeUtils,
      &g_org_chromium_base_TimeUtils_clazz);
}
#endif

namespace base {
namespace android {

// Step 2: method stubs.

static jlong GetTimeTicksNowUs(JNIEnv* env, const
    base::android::JavaParamRef<jclass>& jcaller);

JNI_GENERATOR_EXPORT jlong
    Java_org_chromium_base_TimeUtils_nativeGetTimeTicksNowUs(JNIEnv* env, jclass
    jcaller) {
  return GetTimeTicksNowUs(env, base::android::JavaParamRef<jclass>(env,
      jcaller));
}

}  // namespace android
}  // namespace base

#endif  // org_chromium_base_TimeUtils_JNI
