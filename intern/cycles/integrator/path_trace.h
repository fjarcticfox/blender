/*
 * Copyright 2011-2021 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "device/device_queue.h"
#include "integrator/work_scheduler.h"
#include "render/buffers.h"
#include "util/util_function.h"
#include "util/util_unique_ptr.h"
#include "util/util_vector.h"

CCL_NAMESPACE_BEGIN

class Device;
class DeviceQueue;
class RenderBuffers;

/* PathTrace class takes care of kernel graph and scheduling on a (multi)device. It takes care of
 * all the common steps of path tracing which are not device-specific. The list of tasks includes
 * but is not limited to:
 *  - Kernel graph.
 *  - Scheduling logic.
 *  - Queues management.
 *  - Adaptive stopping. */
class PathTrace {
 public:
  /* `full_buffer_params` denotes parameters of the entire big tile which is to be path traced.
   *
   * TODO(sergey): Streamline terminology. Maybe it should be `big_tile_buffer_params`? */
  PathTrace(Device *device, const BufferParams &full_buffer_params);

  /* Request render of the given number of tiles.
   *
   * TODO(sergey): Decide and document whether it is a blocking or asynchronous call. */
  void render_samples(int samples_num);

  /* Callback which is used top check whether user requested to cancel rendering.
   * If this callback is not assigned the path tracing procfess can not be cancelled and it will be
   * finished when it fully sampled all requested samples. */
  function<bool(void)> get_cancel_cb;

  /* Callback which communicates an updates state of the render buffer.
   * Is called during path tracing to communicate work-in-progress state of the final buffer.
   *
   * The samples indicates how many samples the buffer contains. */
  function<void(RenderBuffers *render_buffers, int sample)> update_cb;

  /* The update callback will never be run more often that this interval, avoiding overhead of
   * data communication on a simple renders.  */
  double update_interval_in_seconds = 1.0;

  /* Callback which communicates final rendered buffer. Is called after pathtracing is done.
   *
   * The samples indicates how many samples the buffer contains. */
  function<void(RenderBuffers *render_buffers, int sample)> write_cb;

 protected:
  /* Run full render pipeline on all devices to add the given number of samples to the render
   * result.
   *
   * There are no update callbacks or cancellation checks are done form here, for the performance
   * reasons.
   *
   * This call advances number of samples stored in the render status. */
  void render_samples_full_pipeline(int samples_num);

  /* This is a worker thread's "run" function which polls for a work to be rendered and renders
   * the work. */
  void render_samples_full_pipeline(DeviceQueue *queue);

  /* Core path tracing routine. Renders given work time on the given queue. */
  void render_samples_full_pipeline(DeviceQueue *queue, const DeviceWorkTile &work_tile);

  /* Check whether user requested to cancel rendering, so that path tracing is to be finished as
   * soon as possible. */
  bool is_cancel_requested();

  /* Run an update callback if needed.
   * This call which check whether an update callback is configured, and do other optimization
   * checks. For example, the update will not be communicated if update happens too often, so that
   * the overhead of update does not degrade rendering performance. */
  void update_if_needed();

  /* Write the big tile render buffer via the write callback. */
  void write();

  /* Pointer to a device which is configured to be used for path tracing. If multiple devices are
   * configured this is a `MultiDevice`. */
  Device *device_ = nullptr;

  /* Scheduler which gives work to path tracing threads. */
  WorkScheduler work_scheduler_;

  /* Per-compute device path tracing contexts. */
  vector<unique_ptr<DeviceQueue>> integrator_queues_;

  /* Render buffer which corresponds to the big tile.
   * It is used to accumulate work from all rendering devices, and to communicate render result
   * to the render session.
   *
   * TODO(sergey): This is actually a subject for reconsideration when multi-device support will be
   * added. */
  unique_ptr<RenderBuffers> full_render_buffers_;

  /* Global path tracing status. */
  /* TODO(sergey): state vs. status. */
  struct RenderStatus {
    /* Reset status before new render begins. */
    void reset();

    /* Number of samples in the render buffer. */
    int rendered_samples_num;
  };
  RenderStatus render_status_;

  /* Status for the update reporting.
   * Is used to avoid updates being sent too often. */
  struct UpdateStatus {
    /* Used before path tracing begins, so that all updates can happen as user expects them. */
    void reset();

    /* Denotes whether update callback was ever called during the current path tracing process. */
    bool has_update;
    /* Timestamp of when the update callback was last call (only valid if `has_update` is true.) */
    double last_update_time;
  };
  UpdateStatus update_status_;
};

CCL_NAMESPACE_END
