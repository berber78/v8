// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HEAP_ARRAY_BUFFER_TRACKER_H_
#define V8_HEAP_ARRAY_BUFFER_TRACKER_H_

#include <map>

#include "src/allocation.h"
#include "src/base/platform/mutex.h"
#include "src/globals.h"

namespace v8 {
namespace internal {

class Heap;
class JSArrayBuffer;
class Page;

class ArrayBufferTracker : public AllStatic {
 public:
  enum ProcessingMode {
    kUpdateForwardedRemoveOthers,
    kUpdateForwardedKeepOthers,
  };

  // The following methods are used to track raw C++ pointers to externally
  // allocated memory used as backing store in live array buffers.

  // Register/unregister a new JSArrayBuffer |buffer| for tracking.
  static void RegisterNew(Heap* heap, JSArrayBuffer* buffer);
  static void Unregister(Heap* heap, JSArrayBuffer* buffer);

  // Frees all backing store pointers for dead JSArrayBuffers in new space.
  static void FreeDeadInNewSpace(Heap* heap);

  // Frees all backing store pointers for dead JSArrayBuffer on a given page.
  // Requires marking information to be present.
  static void FreeDead(Page* page);

  // Processes all array buffers on a given page. |mode| specifies the action
  // to perform on the buffers.
  static void ProcessBuffers(Page* page, ProcessingMode mode);

  // Returns whether a buffer is currently tracked.
  static bool IsTracked(JSArrayBuffer* buffer);
};

// LocalArrayBufferTracker tracks internalized array buffers.
//
// Never use directly but instead always call through |ArrayBufferTracker|.
class LocalArrayBufferTracker {
 public:
  typedef std::pair<void*, size_t> Value;
  typedef JSArrayBuffer* Key;

  enum CallbackResult { kKeepEntry, kUpdateEntry, kRemoveEntry };

  explicit LocalArrayBufferTracker(Heap* heap) : heap_(heap) {}
  ~LocalArrayBufferTracker();

  void Add(Key key, const Value& value);
  Value Remove(Key key);

  // Frees up any buffers that are currently not marked.
  void FreeDead();

  // Processes buffers one by one. The CallbackResult of the callback decides
  // what action to take on the buffer.
  //
  // Callback should be of type:
  //   CallbackResult fn(JSArrayBuffer* buffer, JSArrayBuffer** new_buffer);
  template <typename Callback>
  inline void Process(Callback callback);

  bool IsEmpty() { return array_buffers_.empty(); }

  bool IsTracked(Key key) {
    return array_buffers_.find(key) != array_buffers_.end();
  }

 private:
  // TODO(mlippautz): Switch to unordered_map once it is supported on all
  // platforms.
  typedef std::map<Key, Value> TrackingMap;

  Heap* heap_;
  TrackingMap array_buffers_;
};

}  // namespace internal
}  // namespace v8
#endif  // V8_HEAP_ARRAY_BUFFER_TRACKER_H_
