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

#include <wangle/service/Service.h>

namespace folly { namespace wangle {

/**
 * A service that rejects all requests after its 'close' method has
 * been invoked.
 */
template <typename Req, typename Resp>
class CloseOnReleaseFilter : public ServiceFilter<Req, Resp> {
 public:
  explicit CloseOnReleaseFilter(std::shared_ptr<Service<Req, Resp>> service)
      : ServiceFilter<Req, Resp>(service) {}

  Future<Resp> operator()(Req req) override {
    if (!released ){
      return (*this->service_)(std::move(req));
    } else {
      return makeFuture<Resp>(
        make_exception_wrapper<std::runtime_error>("Service Closed"));
    }
  }

  Future<void> close() override {
    if (!released.exchange(true)) {
      return this->service_->close();
    } else {
      return makeFuture();
    }
  }
 private:
  std::atomic<bool> released{false};
};

}} // namespace
