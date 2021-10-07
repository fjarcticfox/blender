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

#pragma once

#include "COM_MultiThreadedOperation.h"

struct Scene;

namespace blender::compositor {

/**
 * \brief Compositor output operation
 */
class CompositorOperation : public MultiThreadedOperation {
 private:
  const struct Scene *m_scene;
  /**
   * \brief Scene name, used for getting the render output, includes 'SC' prefix.
   */
  char m_sceneName[MAX_ID_NAME];

  /**
   * \brief local reference to the scene
   */
  const RenderData *m_rd;

  /**
   * \brief reference to the output float buffer
   */
  float *m_outputBuffer;

  /**
   * \brief reference to the output depth float buffer
   */
  float *m_depthBuffer;

  /**
   * \brief local reference to the input image operation
   */
  SocketReader *m_imageInput;

  /**
   * \brief local reference to the input alpha operation
   */
  SocketReader *m_alphaInput;

  /**
   * \brief local reference to the depth operation
   */
  SocketReader *m_depthInput;

  /**
   * \brief Ignore any alpha input
   */
  bool m_useAlphaInput;

  /**
   * \brief operation is active for calculating final compo result
   */
  bool m_active;

  /**
   * \brief View name, used for multiview
   */
  const char *m_viewName;

 public:
  CompositorOperation();
  bool isActiveCompositorOutput() const
  {
    return m_active;
  }
  void executeRegion(rcti *rect, unsigned int tileNumber) override;
  void setScene(const struct Scene *scene)
  {
    m_scene = scene;
  }
  void setSceneName(const char *sceneName)
  {
    BLI_strncpy(m_sceneName, sceneName, sizeof(m_sceneName));
  }
  void setViewName(const char *viewName)
  {
    m_viewName = viewName;
  }
  void setRenderData(const RenderData *rd)
  {
    m_rd = rd;
  }
  bool isOutputOperation(bool /*rendering*/) const override
  {
    return this->isActiveCompositorOutput();
  }
  void initExecution() override;
  void deinitExecution() override;
  eCompositorPriority getRenderPriority() const override
  {
    return eCompositorPriority::Medium;
  }
  void determine_canvas(const rcti &preferred_area, rcti &r_area) override;
  void setUseAlphaInput(bool value)
  {
    m_useAlphaInput = value;
  }
  void setActive(bool active)
  {
    m_active = active;
  }

  void update_memory_buffer_partial(MemoryBuffer *output,
                                    const rcti &area,
                                    Span<MemoryBuffer *> inputs) override;
};

}  // namespace blender::compositor
