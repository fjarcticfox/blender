/*
 * Copyright 2011-2013 Blender Foundation
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

#include "kernel/kernel_light.h"
#include "kernel/kernel_montecarlo.h"
#include "kernel/kernel_path_state.h"
#include "kernel/kernel_shader.h"
#include "kernel/kernel_shadow.h"

CCL_NAMESPACE_BEGIN

/* Direction Emission */
ccl_device_noinline_cpu float3 direct_emissive_eval(const KernelGlobals *kg,
                                                    ShaderData *emission_sd,
                                                    LightSample *ls,
                                                    ccl_addr_space PathState *state,
                                                    float3 I,
                                                    differential3 dI,
                                                    float t,
                                                    float time)
{
  /* setup shading at emitter */
  float3 eval = make_float3(0.0f, 0.0f, 0.0f);

  if (shader_constant_emission_eval(kg, ls->shader, &eval)) {
    if ((ls->prim != PRIM_NONE) && dot(ls->Ng, I) < 0.0f) {
      ls->Ng = -ls->Ng;
    }
  }
  else {
    /* Setup shader data and call shader_eval_surface once, better
     * for GPU coherence and compile times. */
#ifdef __BACKGROUND_MIS__
    if (ls->type == LIGHT_BACKGROUND) {
      Ray ray;
      ray.D = ls->D;
      ray.P = ls->P;
      ray.t = 1.0f;
      ray.time = time;
      ray.dP = differential3_zero();
      ray.dD = dI;

      shader_setup_from_background(kg, emission_sd, &ray);
    }
    else
#endif
    {
      shader_setup_from_sample(kg,
                               emission_sd,
                               ls->P,
                               ls->Ng,
                               I,
                               ls->shader,
                               ls->object,
                               ls->prim,
                               ls->u,
                               ls->v,
                               t,
                               time,
                               false,
                               ls->lamp);

      ls->Ng = emission_sd->Ng;
    }

    /* No proper path flag, we're evaluating this for all closures. that's
     * weak but we'd have to do multiple evaluations otherwise. */
    path_state_modify_bounce(state, true);
    shader_eval_surface(kg, emission_sd, state, NULL, PATH_RAY_EMISSION);
    path_state_modify_bounce(state, false);

    /* Evaluate closures. */
#ifdef __BACKGROUND_MIS__
    if (ls->type == LIGHT_BACKGROUND) {
      eval = shader_background_eval(emission_sd);
    }
    else
#endif
    {
      eval = shader_emissive_eval(emission_sd);
    }
  }

  eval *= ls->eval_fac;

  if (ls->lamp != LAMP_NONE) {
    const ccl_global KernelLight *klight = &kernel_tex_fetch(__lights, ls->lamp);
    eval *= make_float3(klight->strength[0], klight->strength[1], klight->strength[2]);
  }

  return eval;
}

ccl_device_noinline_cpu bool direct_emission(const KernelGlobals *kg,
                                             ShaderData *sd,
                                             ShaderData *emission_sd,
                                             LightSample *ls,
                                             ccl_addr_space PathState *state,
                                             Ray *ray,
                                             BsdfEval *eval,
                                             bool *is_lamp,
                                             float rand_terminate)
{
  if (ls->pdf == 0.0f)
    return false;

  /* todo: implement */
  differential3 dD = differential3_zero();

  /* evaluate closure */

  float3 light_eval = direct_emissive_eval(
      kg, emission_sd, ls, state, -ls->D, dD, ls->t, sd->time);

  if (is_zero(light_eval))
    return false;

    /* evaluate BSDF at shading point */

#ifdef __VOLUME__
  if (sd->prim != PRIM_NONE)
    shader_bsdf_eval(kg, sd, ls->D, eval, ls->pdf, ls->shader & SHADER_USE_MIS);
  else {
    float bsdf_pdf;
    shader_volume_phase_eval(kg, sd, ls->D, eval, &bsdf_pdf);
    if (ls->shader & SHADER_USE_MIS) {
      /* Multiple importance sampling. */
      float mis_weight = power_heuristic(ls->pdf, bsdf_pdf);
      light_eval *= mis_weight;
    }
  }
#else
  shader_bsdf_eval(kg, sd, ls->D, eval, ls->pdf, ls->shader & SHADER_USE_MIS);
#endif

  bsdf_eval_mul3(eval, light_eval / ls->pdf);

#ifdef __PASSES__
  /* use visibility flag to skip lights */
  if (ls->shader & SHADER_EXCLUDE_ANY) {
    if (ls->shader & SHADER_EXCLUDE_DIFFUSE)
      eval->diffuse = make_float3(0.0f, 0.0f, 0.0f);
    if (ls->shader & SHADER_EXCLUDE_GLOSSY)
      eval->glossy = make_float3(0.0f, 0.0f, 0.0f);
    if (ls->shader & SHADER_EXCLUDE_TRANSMIT)
      eval->transmission = make_float3(0.0f, 0.0f, 0.0f);
    if (ls->shader & SHADER_EXCLUDE_SCATTER)
      eval->volume = make_float3(0.0f, 0.0f, 0.0f);
  }
#endif

  if (bsdf_eval_is_zero(eval))
    return false;

  if (kernel_data.integrator.light_inv_rr_threshold > 0.0f
#ifdef __SHADOW_TRICKS__
      && (state->flag & PATH_RAY_SHADOW_CATCHER) == 0
#endif
  ) {
    float probability = max3(fabs(bsdf_eval_sum(eval))) *
                        kernel_data.integrator.light_inv_rr_threshold;
    if (probability < 1.0f) {
      if (rand_terminate >= probability) {
        return false;
      }
      bsdf_eval_mul(eval, 1.0f / probability);
    }
  }

  if (ls->shader & SHADER_CAST_SHADOW) {
    /* setup ray */
    bool transmit = (dot(sd->Ng, ls->D) < 0.0f);
    ray->P = ray_offset(sd->P, (transmit) ? -sd->Ng : sd->Ng);

    if (ls->t == FLT_MAX) {
      /* distant light */
      ray->D = ls->D;
      ray->t = ls->t;
    }
    else {
      /* other lights, avoid self-intersection */
      ray->D = ray_offset(ls->P, ls->Ng) - ray->P;
      ray->D = normalize_len(ray->D, &ray->t);
    }

    ray->dP = sd->dP;
    ray->dD = differential3_zero();
  }
  else {
    /* signal to not cast shadow ray */
    ray->t = 0.0f;
  }

  /* return if it's a lamp for shadow pass */
  *is_lamp = (ls->prim == PRIM_NONE && ls->type != LIGHT_BACKGROUND);

  return true;
}

/* Indirect Lamp Emission */

ccl_device_noinline_cpu void indirect_lamp_emission(const KernelGlobals *kg,
                                                    ShaderData *emission_sd,
                                                    ccl_addr_space PathState *state,
                                                    PathRadiance *L,
                                                    Ray *ray,
                                                    float3 throughput)
{
  for (int lamp = 0; lamp < kernel_data.integrator.num_all_lights; lamp++) {
    LightSample ls ccl_optional_struct_init;

    if (!lamp_light_eval(kg, lamp, ray->P, ray->D, ray->t, &ls))
      continue;

#ifdef __PASSES__
    /* use visibility flag to skip lights */
    if (ls.shader & SHADER_EXCLUDE_ANY) {
      if (((ls.shader & SHADER_EXCLUDE_DIFFUSE) && (state->flag & PATH_RAY_DIFFUSE)) ||
          ((ls.shader & SHADER_EXCLUDE_GLOSSY) &&
           ((state->flag & (PATH_RAY_GLOSSY | PATH_RAY_REFLECT)) ==
            (PATH_RAY_GLOSSY | PATH_RAY_REFLECT))) ||
          ((ls.shader & SHADER_EXCLUDE_TRANSMIT) && (state->flag & PATH_RAY_TRANSMIT)) ||
          ((ls.shader & SHADER_EXCLUDE_SCATTER) && (state->flag & PATH_RAY_VOLUME_SCATTER)))
        continue;
    }
#endif

    float3 lamp_L = direct_emissive_eval(
        kg, emission_sd, &ls, state, -ray->D, ray->dD, ls.t, ray->time);

#ifdef __VOLUME__
    if (state->volume_stack[0].shader != SHADER_NONE) {
      /* shadow attenuation */
      Ray volume_ray = *ray;
      volume_ray.t = ls.t;
      float3 volume_tp = make_float3(1.0f, 1.0f, 1.0f);
      kernel_volume_shadow(kg, emission_sd, state, &volume_ray, &volume_tp);
      lamp_L *= volume_tp;
    }
#endif

    if (!(state->flag & PATH_RAY_MIS_SKIP)) {
      /* multiple importance sampling, get regular light pdf,
       * and compute weight with respect to BSDF pdf */
      float mis_weight = power_heuristic(state->ray_pdf, ls.pdf);
      lamp_L *= mis_weight;
    }

    path_radiance_accum_emission(kg, L, state, throughput, lamp_L);
  }
}

CCL_NAMESPACE_END
