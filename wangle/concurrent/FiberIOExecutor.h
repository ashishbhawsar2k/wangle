/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <folly/experimental/fibers/FiberManagerMap.h>
#include <wangle/concurrent/IOExecutor.h>

namespace folly { namespace fibers {

/**
 * @class FiberIOExecutor
 * @brief An IOExecutor that executes funcs under mapped fiber context
 *
 * A FiberIOExecutor wraps an IOExecutor, but executes funcs on the FiberManager
 * mapped to the underlying IOExector's event base.
 */
class FiberIOExecutor : public folly::wangle::IOExecutor {
 public:
  explicit FiberIOExecutor(
      const std::shared_ptr<folly::wangle::IOExecutor>& ioExecutor)
      : ioExecutor_(ioExecutor) {}

  virtual void add(std::function<void()> f) override {
    auto eventBase = ioExecutor_->getEventBase();
    getFiberManager(*eventBase).add(std::move(f));
  }

  virtual EventBase* getEventBase() override {
    return ioExecutor_->getEventBase();
  }

 private:
  std::shared_ptr<folly::wangle::IOExecutor> ioExecutor_;
};

}}
