// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/scoped_com_initializer.h"


namespace base {
namespace win {

ScopedCOMInitializer::ScopedCOMInitializer() {
  Initialize(COINIT_APARTMENTTHREADED);
}

ScopedCOMInitializer::ScopedCOMInitializer(SelectMTA mta) {
  Initialize(COINIT_MULTITHREADED);
}

ScopedCOMInitializer::~ScopedCOMInitializer() {
  if (Succeeded())
    CoUninitialize();
}

bool ScopedCOMInitializer::Succeeded() const {
  return SUCCEEDED(hr_);
}

void ScopedCOMInitializer::Initialize(COINIT init) {
  hr_ = CoInitializeEx(NULL, init);
}

}  // namespace win
}  // namespace base
