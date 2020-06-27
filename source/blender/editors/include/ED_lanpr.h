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
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup editors
 */

#ifndef __ED_LANPR_H__
#define __ED_LANPR_H__

#ifndef WITH_LINEART
#  error LANPR code included in non-LANPR-enabled build
#endif

#include "BLI_linklist.h"
#include "BLI_listbase.h"
#include "BLI_math.h"
#include "BLI_threads.h"

#include "DNA_lanpr_types.h"

#include <math.h>
#include <string.h>

typedef struct eLineArtStaticMemPoolNode {
  Link item;
  int used_byte;
  /* User memory starts here */
} eLineArtStaticMemPoolNode;

typedef struct eLineArtStaticMemPool {
  int each_size;
  ListBase pools;
  SpinLock lock_mem;
} eLineArtStaticMemPool;

typedef struct eLineArtRenderTriangle {
  struct eLineArtRenderTriangle *next, *prev;
  struct eLineArtRenderVert *v[3];
  struct eLineArtRenderLine *rl[3];
  double gn[3];
  double gc[3];
  /*  struct BMFace *F; */
  short material_id;
  ListBase intersecting_verts;
  char cull_status;
  /**  Should be testing** , Use testing[NumOfThreads] to access. */
  struct eLineArtRenderTriangle *testing;
} eLineArtRenderTriangle;

typedef struct eLineArtRenderTriangleThread {
  struct eLineArtRenderTriangle base;
  struct eLineArtRenderLine *testing[127];
} eLineArtRenderTriangleThread;

typedef struct eLineArtRenderElementLinkNode {
  struct eLineArtRenderElementLinkNode *next, *prev;
  void *pointer;
  int element_count;
  void *object_ref;
  char additional;
} eLineArtRenderElementLinkNode;

typedef struct eLineArtRenderLineSegment {
  struct eLineArtRenderLineSegment *next, *prev;
  /** at==0: left  at==1: right  (this is in 2D projected space) */
  double at;
  /** Occlusion level after "at" point */
  unsigned char occlusion;
  /** For determining lines beind a glass window material. (TODO: implement this) */
  short material_mask_mark;
} eLineArtRenderLineSegment;

typedef struct eLineArtRenderVert {
  struct eLineArtRenderVert *next, *prev;
  double gloc[4];
  double fbcoord[4];
  int fbcoordi[2];
  /**  Used as "r" when intersecting */
  struct BMVert *v;
  struct eLineArtRenderLine *intersecting_line;
  struct eLineArtRenderLine *intersecting_line2;
  struct eLineArtRenderTriangle *intersecting_with;

  /** This will used in future acceleration for intersection processing. */
  char edge_used;
} eLineArtRenderVert;

typedef enum eLineArtEdgeFlag {
  LRT_EDGE_FLAG_EDGE_MARK = (1 << 0),
  LRT_EDGE_FLAG_CONTOUR = (1 << 1),
  LRT_EDGE_FLAG_CREASE = (1 << 2),
  LRT_EDGE_FLAG_MATERIAL = (1 << 3),
  LRT_EDGE_FLAG_INTERSECTION = (1 << 4),
  /**  floating edge, unimplemented yet */
  LRT_EDGE_FLAG_FLOATING = (1 << 5),
  LRT_EDGE_FLAG_CHAIN_PICKED = (1 << 6),
} eLineArtEdgeFlag;

#define LRT_EDGE_FLAG_ALL_TYPE 0x3f

typedef struct eLineArtRenderLine {
  struct eLineArtRenderLine *next, *prev;
  struct eLineArtRenderVert *l, *r;
  struct eLineArtRenderTriangle *tl, *tr;
  ListBase segments;
  char min_occ;

  /**  Also for line type determination on chainning */
  char flags;

  /**  Still need this entry because culled lines will not add to object reln node */
  struct Object *object_ref;

  /**  For gpencil stroke modifier */
  int edge_idx;
} eLineArtRenderLine;

typedef struct eLineArtRenderLineChain {
  struct eLineArtRenderLineChain *next, *prev;
  ListBase chain;

  /**  Calculated before draw cmd. */
  float length;

  /**  Used when re-connecting and gp stroke generation */
  char picked;
  char level;

  /** Chain now only contains one type of segments */
  int type;
  struct Object *object_ref;
} eLineArtRenderLineChain;

typedef struct eLineArtRenderLineChainItem {
  struct eLineArtRenderLineChainItem *next, *prev;
  /** Need z value for fading */
  float pos[3];
  /** For restoring position to 3d space */
  float gpos[3];
  float normal[3];
  char line_type;
  char occlusion;
} eLineArtRenderLineChainItem;

typedef struct eLineArtChainRegisterEntry {
  struct eLineArtChainRegisterEntry *next, *prev;
  eLineArtRenderLineChain *rlc;
  eLineArtRenderLineChainItem *rlci;
  char picked;

  /** left/right mark.
   * Because we revert list in chaining so we need the flag. */
  char is_left;
} eLineArtChainRegisterEntry;

typedef struct eLineArtRenderBuffer {
  struct eLineArtRenderBuffer *prev, *next;

  /** For render. */
  int is_copied;

  int w, h;
  int tile_size_w, tile_size_h;
  int tile_count_x, tile_count_y;
  double width_per_tile, height_per_tile;
  double view_projection[4][4];

  int output_mode;
  int output_aa_level;

  struct eLineArtBoundingArea *initial_bounding_areas;
  unsigned int bounding_area_count;

  ListBase vertex_buffer_pointers;
  ListBase line_buffer_pointers;
  ListBase triangle_buffer_pointers;
  ListBase all_render_lines;

  ListBase intersecting_vertex_buffer;
  /** Use the one comes with LANPR. */
  eLineArtStaticMemPool render_data_pool;

  struct Material *material_pointers[2048];

  /*  Render status */
  double view_vector[3];

  int triangle_size;

  unsigned int contour_count;
  unsigned int contour_processed;
  LinkData *contour_managed;
  ListBase contours;

  unsigned int intersection_count;
  unsigned int intersection_processed;
  LinkData *intersection_managed;
  ListBase intersection_lines;

  unsigned int crease_count;
  unsigned int crease_processed;
  LinkData *crease_managed;
  ListBase crease_lines;

  unsigned int material_line_count;
  unsigned int material_processed;
  LinkData *material_managed;
  ListBase material_lines;

  unsigned int edge_mark_count;
  unsigned int edge_mark_processed;
  LinkData *edge_mark_managed;
  ListBase edge_marks;

  ListBase chains;

  /** For managing calculation tasks for multiple threads. */
  SpinLock lock_task;

  /*  settings */

  int max_occlusion_level;
  double crease_angle;
  double crease_cos;
  int thread_count;

  int draw_material_preview;
  double material_transparency;

  int show_line;
  int show_fast;
  int show_material;
  int override_display;

  int use_intersections;
  int _pad;

  /** Keep an copy of these data so the scene can be freed when lineart is runnning. */
  char cam_is_persp;
  float cam_obmat[4][4];
  double camera_pos[3];
  double near_clip, far_clip;
  double shift_x, shift_y;
  double chaining_image_threshold;
  double chaining_geometry_threshold;
} eLineArtRenderBuffer;

typedef enum eLineArtRenderStatus {
  LRT_RENDER_IDLE = 0,
  LRT_RENDER_RUNNING = 1,
  LRT_RENDER_INCOMPELTE = 2,
  LRT_RENDER_FINISHED = 3,
} eLineArtRenderStatus;

typedef enum eLineArtInitStatus {
  LRT_INIT_ENGINE = (1 << 0),
  LRT_INIT_LOCKS = (1 << 1),
} eLineArtInitStatus;

typedef struct eLineArtSharedResource {

  /* We only allocate once for all */
  eLineArtRenderBuffer *render_buffer_shared;

  /* cache */
  struct BLI_mempool *mp_sample;
  struct BLI_mempool *mp_line_strip;
  struct BLI_mempool *mp_line_strip_point;
  struct BLI_mempool *mp_batch_list;

  struct TaskPool *background_render_task;

  eLineArtInitStatus init_complete;

  /** To bypass or cancel rendering.
   * This status flag should be kept in lineart_share not render_buffer,
   * because render_buffer will get re-initialized every frame.
   */
  SpinLock lock_render_status;
  eLineArtRenderStatus flag_render_status;

  /** Geometry loading is done in the worker thread,
   * Lock the render thread until loading is done, so that
   * we can avoid depsgrapgh deleting the scene before
   * LANPR finishes loading. Also keep this in lineart_share.
   */
  SpinLock lock_loader;

  /** When drawing in the viewport, use the following values. */
  /** Set to override to -1 before creating lineart render buffer to use scene camera. */
  int viewport_camera_override;
  char camera_is_persp;
  float camera_pos[3];
  float near_clip, far_clip;
  float viewinv[4][4];
  float persp[4][4];
  float viewquat[4];
} eLineArtSharedResource;

#define DBL_TRIANGLE_LIM 1e-8
#define DBL_EDGE_LIM 1e-9

#define LRT_MEMORY_POOL_1MB 1048576
#define LRT_MEMORY_POOL_128MB 134217728
#define LRT_MEMORY_POOL_256MB 268435456
#define LRT_MEMORY_POOL_512MB 536870912

typedef enum eLineArtCullState {
  LRT_CULL_DONT_CARE = 0,
  LRT_CULL_USED = 1,
  LRT_CULL_DISCARD = 2,
} eLineArtCullState;

/** Controls how many lines a worker thread is processing at one request.
 * There's no significant performance impact on choosing different values.
 * Don't make it too small so that the worker thread won't request too many times. */
#define LRT_THREAD_LINE_COUNT 10000

typedef struct eLineArtRenderTaskInfo {
  int thread_id;

  LinkData *contour;
  ListBase contour_pointers;

  LinkData *intersection;
  ListBase intersection_pointers;

  LinkData *crease;
  ListBase crease_pointers;

  LinkData *material;
  ListBase material_pointers;

  LinkData *edge_mark;
  ListBase edge_mark_pointers;

} eLineArtRenderTaskInfo;

/** Bounding area diagram:
 *
 * +----+ <----U (Upper edge Y value)
 * |    |
 * +----+ <----B (Bottom edge Y value)
 * ^    ^
 * L    R (Left/Right edge X value)
 *
 * Example structure when subdividing 1 bounding areas:
 * 1 area can be divided into 4 smaller children to
 * accomodate image areas with denser triangle distribution.
 * +--+--+-----+
 * +--+--+     |
 * +--+--+-----+
 * |     |     |
 * +-----+-----+
 * lp/rp/up/bp is the list for
 * storing pointers to adjacent bounding areas.
 */
typedef struct eLineArtBoundingArea {
  double l, r, u, b;
  double cx, cy;

  /** 1,2,3,4 quadrant */
  struct eLineArtBoundingArea *child;

  ListBase lp;
  ListBase rp;
  ListBase up;
  ListBase bp;

  int triangle_count;
  ListBase linked_triangles;
  ListBase linked_lines;

  /** Reserved for image space reduction && multithread chainning */
  ListBase linked_chains;
} eLineArtBoundingArea;

#define TNS_TILE(tile, r, c, CCount) tile[r * CCount + c]

#define TNS_CLAMP(a, Min, Max) a = a < Min ? Min : (a > Max ? Max : a)

#define TNS_MAX3_INDEX(a, b, c) (a > b ? (a > c ? 0 : (b > c ? 1 : 2)) : (b > c ? 1 : 2))

#define TNS_MIN3_INDEX(a, b, c) (a < b ? (a < c ? 0 : (b < c ? 1 : 2)) : (b < c ? 1 : 2))

#define TNS_MAX3_INDEX_ABC(x, y, z) (x > y ? (x > z ? a : (y > z ? b : c)) : (y > z ? b : c))

#define TNS_MIN3_INDEX_ABC(x, y, z) (x < y ? (x < z ? a : (y < z ? b : c)) : (y < z ? b : c))

#define TNS_ABC(index) (index == 0 ? a : (index == 1 ? b : c))

#define TNS_DOUBLE_CLOSE_ENOUGH(a, b) (((a) + DBL_EDGE_LIM) >= (b) && ((a)-DBL_EDGE_LIM) <= (b))

BLI_INLINE double tmat_get_linear_ratio(double l, double r, double from_l);
BLI_INLINE int lineart_LineIntersectTest2d(
    const double *a1, const double *a2, const double *b1, const double *b2, double *aRatio)
{
  double k1, k2;
  double x;
  double y;
  double ratio;
  double x_diff = (a2[0] - a1[0]);
  double x_diff2 = (b2[0] - b1[0]);

  if (x_diff == 0) {
    if (x_diff2 == 0) {
      *aRatio = 0;
      return 0;
    }
    double r2 = tmat_get_linear_ratio(b1[0], b2[0], a1[0]);
    x = interpd(b2[0], b1[0], r2);
    y = interpd(b2[1], b1[1], r2);
    *aRatio = ratio = tmat_get_linear_ratio(a1[1], a2[1], y);
  }
  else {
    if (x_diff2 == 0) {
      ratio = tmat_get_linear_ratio(a1[0], a2[0], b1[0]);
      x = interpd(a2[0], a1[0], ratio);
      *aRatio = ratio;
    }
    else {
      k1 = (a2[1] - a1[1]) / x_diff;
      k2 = (b2[1] - b1[1]) / x_diff2;

      if ((k1 == k2))
        return 0;

      x = (a1[1] - b1[1] - k1 * a1[0] + k2 * b1[0]) / (k2 - k1);

      ratio = (x - a1[0]) / x_diff;

      *aRatio = ratio;
    }
  }

  if (b1[0] == b2[0]) {
    y = interpd(a2[1], a1[1], ratio);
    if (y > MAX2(b1[1], b2[1]) || y < MIN2(b1[1], b2[1]))
      return 0;
  }
  else if (ratio <= 0 || ratio > 1 || (b1[0] > b2[0] && x > b1[0]) ||
           (b1[0] < b2[0] && x < b1[0]) || (b2[0] > b1[0] && x > b2[0]) ||
           (b2[0] < b1[0] && x < b2[0]))
    return 0;

  return 1;
}

BLI_INLINE double tmat_get_linear_ratio(double l, double r, double from_l)
{
  double ra = (from_l - l) / (r - l);
  return ra;
}

int ED_lineart_point_inside_triangled(double v[2], double v0[2], double v1[2], double v2[2]);

struct Depsgraph;
struct SceneLANPR;
struct Scene;
struct eLineArtRenderBuffer;

void ED_lineart_init_locks(void);
struct eLineArtRenderBuffer *ED_lineart_create_render_buffer(struct Scene *s);
void ED_lineart_destroy_render_data(void);
void ED_lineart_destroy_render_data_external(void);

int ED_lineart_object_collection_usage_check(struct Collection *c, struct Object *o);

void ED_lineart_NO_THREAD_chain_feature_lines(eLineArtRenderBuffer *rb);
void ED_lineart_split_chains_for_fixed_occlusion(eLineArtRenderBuffer *rb);
void ED_lineart_connect_chains(eLineArtRenderBuffer *rb, const int do_geometry_space);
void ED_lineart_discard_short_chains(eLineArtRenderBuffer *rb, const float threshold);
int ED_lineart_count_chain(const eLineArtRenderLineChain *rlc);
void ED_lineart_chain_clear_picked_flag(struct eLineArtRenderBuffer *rb);

int ED_lineart_count_leveled_edge_segment_count(const ListBase *line_list,
                                                const struct LANPR_LineLayer *ll);
void *ED_lineart_make_leveled_edge_vertex_array(struct eLineArtRenderBuffer *rb,
                                                const ListBase *line_list,
                                                float *vertex_array,
                                                float *normal_array,
                                                float **next_normal,
                                                const LANPR_LineLayer *ll,
                                                const float componet_id);

void ED_lineart_calculation_set_flag(eLineArtRenderStatus flag);
bool ED_lineart_calculation_flag_check(eLineArtRenderStatus flag);

int ED_lineart_compute_feature_lines_internal(struct Depsgraph *depsgraph,
                                              const int instersections_only);

void ED_lineart_compute_feature_lines_background(struct Depsgraph *dg,
                                                 const int intersection_only);

struct Scene;

int ED_lineart_max_occlusion_in_line_layers(struct SceneLANPR *lineart);
LANPR_LineLayer *ED_lineart_new_line_layer(struct SceneLANPR *lineart);

eLineArtBoundingArea *ED_lineart_get_point_bounding_area(eLineArtRenderBuffer *rb,
                                                         double x,
                                                         double y);
eLineArtBoundingArea *ED_lineart_get_point_bounding_area_deep(eLineArtRenderBuffer *rb,
                                                              double x,
                                                              double y);

void ED_lineart_post_frame_update_external(struct Scene *s, struct Depsgraph *dg);

struct SceneLANPR;

void ED_lineart_update_render_progress(const char *text);

void ED_lineart_calculate_normal_object_vector(LANPR_LineLayer *ll,
                                               float *normal_object_direction);

float ED_lineart_compute_chain_length(eLineArtRenderLineChain *rlc);

struct wmOperatorType;

/* Operator types */
void SCENE_OT_lineart_calculate_feature_lines(struct wmOperatorType *ot);
void SCENE_OT_lineart_add_line_layer(struct wmOperatorType *ot);
void SCENE_OT_lineart_delete_line_layer(struct wmOperatorType *ot);
void SCENE_OT_lineart_auto_create_line_layer(struct wmOperatorType *ot);
void SCENE_OT_lineart_move_line_layer(struct wmOperatorType *ot);
void SCENE_OT_lineart_enable_all_line_types(struct wmOperatorType *ot);
void SCENE_OT_lineart_update_gp_strokes(struct wmOperatorType *ot);
void SCENE_OT_lineart_bake_gp_strokes(struct wmOperatorType *ot);

void OBJECT_OT_lineart_update_gp_target(struct wmOperatorType *ot);
void OBJECT_OT_lineart_update_gp_source(struct wmOperatorType *ot);

void ED_operatortypes_lineart(void);

#endif /* __ED_LANPR_H__ */
