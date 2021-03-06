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
 */
#pragma once

/** \file
 * \ingroup bke
 */

#include "BLI_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct Main;
struct ReportList;
struct bContext;

/* copybuffer (wrapper for BKE_blendfile_write_partial) */
void BKE_copybuffer_copy_begin(struct Main *bmain_src);
void BKE_copybuffer_copy_tag_ID(struct ID *id);
bool BKE_copybuffer_copy_end(struct Main *bmain_src,
                             const char *filename,
                             struct ReportList *reports);
bool BKE_copybuffer_read(struct Main *bmain_dst,
                         const char *libname,
                         struct ReportList *reports,
                         const uint64_t id_types_mask);
int BKE_copybuffer_paste(struct bContext *C,
                         const char *libname,
                         const int flag,
                         struct ReportList *reports,
                         const uint64_t id_types_mask);

#ifdef __cplusplus
}
#endif
