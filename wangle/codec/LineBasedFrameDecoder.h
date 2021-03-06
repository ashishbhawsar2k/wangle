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

#include <wangle/codec/ByteToMessageCodec.h>
#include <folly/io/Cursor.h>

namespace folly { namespace wangle {

/**
 * A decoder that splits the received IOBufQueue on line endings.
 *
 * Both "\n" and "\r\n" are handled, or optionally reqire only
 * one or the other.
 */
class LineBasedFrameDecoder : public ByteToMessageCodec {
 public:
  enum class TerminatorType {
    BOTH,
    NEWLINE,
    CARRIAGENEWLINE
  };

  LineBasedFrameDecoder(uint32_t maxLength = UINT_MAX,
                        bool stripDelimiter = true,
                        TerminatorType terminatorType =
                        TerminatorType::BOTH);

  std::unique_ptr<IOBuf> decode(Context* ctx, IOBufQueue& buf, size_t&);

 private:

  int64_t findEndOfLine(IOBufQueue& buf);

  void fail(Context* ctx, std::string len);

  uint32_t maxLength_;
  bool stripDelimiter_;

  bool discarding_{false};
  uint32_t discardedBytes_{0};

  TerminatorType terminatorType_;
};

}} // namespace
