/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Copyright 2011, Blender Foundation.
 */

#include "COM_MapValueOperation.h"

namespace blender::compositor {

MapValueOperation::MapValueOperation()
{
  this->add_input_socket(DataType::Value);
  this->add_output_socket(DataType::Value);
  input_operation_ = nullptr;
  flags_.can_be_constant = true;
}

void MapValueOperation::init_execution()
{
  input_operation_ = this->get_input_socket_reader(0);
}

void MapValueOperation::execute_pixel_sampled(float output[4],
                                              float x,
                                              float y,
                                              PixelSampler sampler)
{
  float src[4];
  input_operation_->read_sampled(src, x, y, sampler);
  TexMapping *texmap = settings_;
  float value = (src[0] + texmap->loc[0]) * texmap->size[0];
  if (texmap->flag & TEXMAP_CLIP_MIN) {
    if (value < texmap->min[0]) {
      value = texmap->min[0];
    }
  }
  if (texmap->flag & TEXMAP_CLIP_MAX) {
    if (value > texmap->max[0]) {
      value = texmap->max[0];
    }
  }

  output[0] = value;
}

void MapValueOperation::deinit_execution()
{
  input_operation_ = nullptr;
}

void MapValueOperation::update_memory_buffer_partial(MemoryBuffer *output,
                                                     const rcti &area,
                                                     Span<MemoryBuffer *> inputs)
{
  for (BuffersIterator<float> it = output->iterate_with(inputs, area); !it.is_end(); ++it) {
    const float input = *it.in(0);
    TexMapping *texmap = settings_;
    float value = (input + texmap->loc[0]) * texmap->size[0];
    if (texmap->flag & TEXMAP_CLIP_MIN) {
      if (value < texmap->min[0]) {
        value = texmap->min[0];
      }
    }
    if (texmap->flag & TEXMAP_CLIP_MAX) {
      if (value > texmap->max[0]) {
        value = texmap->max[0];
      }
    }

    it.out[0] = value;
  }
}

}  // namespace blender::compositor
