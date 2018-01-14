// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/ssl/ssl_client_session_cache.h"

#include <utility>

#include "base/containers/flat_set.h"
#include "base/strings/stringprintf.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"

namespace net {

SSLClientSessionCache::SSLClientSessionCache(const Config& config)
    : clock_(base::DefaultClock::GetInstance()),
      config_(config),
      cache_(config.max_entries),
      lookups_since_flush_(0) {
}

SSLClientSessionCache::~SSLClientSessionCache() {
  Flush();
}

size_t SSLClientSessionCache::size() const {
  return cache_.size();
}

bssl::UniquePtr<SSL_SESSION> SSLClientSessionCache::Lookup(
    const std::string& cache_key) {
  base::AutoLock lock(lock_);

  // Expire stale sessions.
  lookups_since_flush_++;
  if (lookups_since_flush_ >= config_.expiration_check_count) {
    lookups_since_flush_ = 0;
    FlushExpiredSessions();
  }

  auto iter = cache_.Get(cache_key);
  if (iter == cache_.end())
    return nullptr;

  time_t now = clock_->Now().ToTimeT();
  bssl::UniquePtr<SSL_SESSION> session = iter->second.Pop();
  if (iter->second.ExpireSessions(now))
    cache_.Erase(iter);

  if (IsExpired(session.get(), now))
    session = nullptr;
  return session;
}

void SSLClientSessionCache::ResetLookupCount(const std::string& cache_key) {
  base::AutoLock lock(lock_);

  // It's possible that the cached session for this key was deleted after the
  // Lookup. If that's the case, don't do anything.
  auto iter = cache_.Get(cache_key);
  if (iter == cache_.end())
    return;
}

void SSLClientSessionCache::Insert(const std::string& cache_key,
                                   SSL_SESSION* session) {
  base::AutoLock lock(lock_);

  SSL_SESSION_up_ref(session);
  auto iter = cache_.Get(cache_key);
  if (iter == cache_.end())
    iter = cache_.Put(cache_key, Entry());
  iter->second.Push(bssl::UniquePtr<SSL_SESSION>(session));
}

void SSLClientSessionCache::Flush() {
  base::AutoLock lock(lock_);

  cache_.Clear();
}

void SSLClientSessionCache::SetClockForTesting(base::Clock* clock) {
  clock_ = clock;
}

bool SSLClientSessionCache::IsExpired(SSL_SESSION* session, time_t now) {
  if (now < 0)
    return true;
  uint64_t now_u64 = static_cast<uint64_t>(now);
  return now_u64 < SSL_SESSION_get_time(session) ||
         now_u64 >=
             SSL_SESSION_get_time(session) + SSL_SESSION_get_timeout(session);
}

SSLClientSessionCache::Entry::Entry() = default;
SSLClientSessionCache::Entry::Entry(Entry&&) = default;
SSLClientSessionCache::Entry::~Entry() = default;

void SSLClientSessionCache::Entry::Push(bssl::UniquePtr<SSL_SESSION> session) {
  if (sessions[0] != nullptr &&
      SSL_SESSION_should_be_single_use(sessions[0].get())) {
    sessions[1] = std::move(sessions[0]);
  }
  sessions[0] = std::move(session);
}

bssl::UniquePtr<SSL_SESSION> SSLClientSessionCache::Entry::Pop() {
  if (sessions[0] == nullptr)
    return nullptr;
  SSL_SESSION* session = sessions[0].get();
  SSL_SESSION_up_ref(session);
  if (SSL_SESSION_should_be_single_use(session)) {
    sessions[0] = std::move(sessions[1]);
    sessions[1] = nullptr;
  }
  return bssl::UniquePtr<SSL_SESSION>(session);
}

bool SSLClientSessionCache::Entry::ExpireSessions(time_t now) {
  if (sessions[0] == nullptr)
    return true;

  if (SSLClientSessionCache::IsExpired(sessions[0].get(), now)) {
    return true;
  }

  if (sessions[1] != nullptr &&
      SSLClientSessionCache::IsExpired(sessions[1].get(), now)) {
    sessions[1] = nullptr;
  }

  return false;
}

void SSLClientSessionCache::FlushExpiredSessions() {
  time_t now = clock_->Now().ToTimeT();
  auto iter = cache_.begin();
  while (iter != cache_.end()) {
    if (iter->second.ExpireSessions(now)) {
      iter = cache_.Erase(iter);
    } else {
      ++iter;
    }
  }
}

}  // namespace net
