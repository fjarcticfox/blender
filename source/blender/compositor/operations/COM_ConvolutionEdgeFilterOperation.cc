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

#include "COM_ConvolutionEdgeFilterOperation.h"

namespace blender::compositor {

void ConvolutionEdgeFilterOperation::executePixel(float output[4], int x, int y, void * /*data*/)
{
  float in1[4], in2[4], res1[4] = {0.0}, res2[4] = {0.0};

  int x1 = x - 1;
  int x2 = x;
  int x3 = x + 1;
  int y1 = y - 1;
  int y2 = y;
  int y3 = y + 1;
  CLAMP(x1, 0, getWidth() - 1);
  CLAMP(x2, 0, getWidth() - 1);
  CLAMP(x3, 0, getWidth() - 1);
  CLAMP(y1, 0, getHeight() - 1);
  CLAMP(y2, 0, getHeight() - 1);
  CLAMP(y3, 0, getHeight() - 1);

  float value[4];
  m_inputValueOperation->read(value, x2, y2, nullptr);
  float mval = 1.0f - value[0];

  m_inputOperation->read(in1, x1, y1, nullptr);
  madd_v3_v3fl(res1, in1, m_filter[0]);
  madd_v3_v3fl(res2, in1, m_filter[0]);

  m_inputOperation->read(in1, x2, y1, nullptr);
  madd_v3_v3fl(res1, in1, m_filter[1]);
  madd_v3_v3fl(res2, in1, m_filter[3]);

  m_inputOperation->read(in1, x3, y1, nullptr);
  madd_v3_v3fl(res1, in1, m_filter[2]);
  madd_v3_v3fl(res2, in1, m_filter[6]);

  m_inputOperation->read(in1, x1, y2, nullptr);
  madd_v3_v3fl(res1, in1, m_filter[3]);
  madd_v3_v3fl(res2, in1, m_filter[1]);

  m_inputOperation->read(in2, x2, y2, nullptr);
  madd_v3_v3fl(res1, in2, m_filter[4]);
  madd_v3_v3fl(res2, in2, m_filter[4]);

  m_inputOperation->read(in1, x3, y2, nullptr);
  madd_v3_v3fl(res1, in1, m_filter[5]);
  madd_v3_v3fl(res2, in1, m_filter[7]);

  m_inputOperation->read(in1, x1, y3, nullptr);
  madd_v3_v3fl(res1, in1, m_filter[6]);
  madd_v3_v3fl(res2, in1, m_filter[2]);

  m_inputOperation->read(in1, x2, y3, nullptr);
  madd_v3_v3fl(res1, in1, m_filter[7]);
  madd_v3_v3fl(res2, in1, m_filter[5]);

  m_inputOperation->read(in1, x3, y3, nullptr);
  madd_v3_v3fl(res1, in1, m_filter[8]);
  madd_v3_v3fl(res2, in1, m_filter[8]);

  output[0] = sqrt(res1[0] * res1[0] + res2[0] * res2[0]);
  output[1] = sqrt(res1[1] * res1[1] + res2[1] * res2[1]);
  output[2] = sqrt(res1[2] * res1[2] + res2[2] * res2[2]);

  output[0] = output[0] * value[0] + in2[0] * mval;
  output[1] = output[1] * value[0] + in2[1] * mval;
  output[2] = output[2] * value[0] + in2[2] * mval;

  output[3] = in2[3];

  /* Make sure we don't return negative color. */
  output[0] = MAX2(output[0], 0.0f);
  output[1] = MAX2(output[1], 0.0f);
  output[2] = MAX2(output[2], 0.0f);
  output[3] = MAX2(output[3], 0.0f);
}

void ConvolutionEdgeFilterOperation::update_memory_buffer_partial(MemoryBuffer *output,
                                                                  const rcti &area,
                                                                  Span<MemoryBuffer *> inputs)
{
  const MemoryBuffer *image = inputs[IMAGE_INPUT_INDEX];
  const int last_x = getWidth() - 1;
  const int last_y = getHeight() - 1;
  for (BuffersIterator<float> it = output->iterate_with(inputs, area); !it.is_end(); ++it) {
    const int left_offset = (it.x == 0) ? 0 : -image->elem_stride;
    const int right_offset = (it.x == last_x) ? 0 : image->elem_stride;
    const int down_offset = (it.y == 0) ? 0 : -image->row_stride;
    const int up_offset = (it.y == last_y) ? 0 : image->row_stride;

    const float *center_color = it.in(IMAGE_INPUT_INDEX);
    float res1[4] = {0};
    float res2[4] = {0};

    const float *color = center_color + down_offset + left_offset;
    madd_v3_v3fl(res1, color, m_filter[0]);
    copy_v3_v3(res2, res1);

    color = center_color + down_offset;
    madd_v3_v3fl(res1, color, m_filter[1]);
    madd_v3_v3fl(res2, color, m_filter[3]);

    color = center_color + down_offset + right_offset;
    madd_v3_v3fl(res1, color, m_filter[2]);
    madd_v3_v3fl(res2, color, m_filter[6]);

    color = center_color + left_offset;
    madd_v3_v3fl(res1, color, m_filter[3]);
    madd_v3_v3fl(res2, color, m_filter[1]);

    {
      float rgb_filtered[3];
      mul_v3_v3fl(rgb_filtered, center_color, m_filter[4]);
      add_v3_v3(res1, rgb_filtered);
      add_v3_v3(res2, rgb_filtered);
    }

    color = center_color + right_offset;
    madd_v3_v3fl(res1, color, m_filter[5]);
    madd_v3_v3fl(res2, color, m_filter[7]);

    color = center_color + up_offset + left_offset;
    madd_v3_v3fl(res1, color, m_filter[6]);
    madd_v3_v3fl(res2, color, m_filter[2]);

    color = center_color + up_offset;
    madd_v3_v3fl(res1, color, m_filter[7]);
    madd_v3_v3fl(res2, color, m_filter[5]);

    {
      color = center_color + up_offset + right_offset;
      float rgb_filtered[3];
      mul_v3_v3fl(rgb_filtered, color, m_filter[8]);
      add_v3_v3(res1, rgb_filtered);
      add_v3_v3(res2, rgb_filtered);
    }

    it.out[0] = sqrt(res1[0] * res1[0] + res2[0] * res2[0]);
    it.out[1] = sqrt(res1[1] * res1[1] + res2[1] * res2[1]);
    it.out[2] = sqrt(res1[2] * res1[2] + res2[2] * res2[2]);

    const float factor = *it.in(FACTOR_INPUT_INDEX);
    const float m_factor = 1.0f - factor;
    it.out[0] = it.out[0] * factor + center_color[0] * m_factor;
    it.out[1] = it.out[1] * factor + center_color[1] * m_factor;
    it.out[2] = it.out[2] * factor + center_color[2] * m_factor;

    it.out[3] = center_color[3];

    /* Make sure we don't return negative color. */
    CLAMP4_MIN(it.out, 0.0f);
  }
}

}  // namespace blender::compositor
