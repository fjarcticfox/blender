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

#include <stdio.h>

#include "BLI_compiler_attrs.h"
#include "BLI_utildefines.h"
#include "DNA_windowmanager_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Reporting Information and Errors
 *
 * These functions also accept NULL in case no error reporting
 * is needed. */

/* report structures are stored in DNA */

void BKE_reports_init(ReportList *reports, int flag);
void BKE_reports_clear(ReportList *reports);

void BKE_report(ReportList *reports, eReportType type, const char *message);
void BKE_reportf(ReportList *reports, eReportType type, const char *format, ...)
    ATTR_PRINTF_FORMAT(3, 4);

void BKE_reports_prepend(ReportList *reports, const char *prepend);
void BKE_reports_prependf(ReportList *reports, const char *prepend, ...) ATTR_PRINTF_FORMAT(2, 3);

eReportType BKE_report_print_level(ReportList *reports);
void BKE_report_print_level_set(ReportList *reports, eReportType level);

eReportType BKE_report_store_level(ReportList *reports);
void BKE_report_store_level_set(ReportList *reports, eReportType level);

char *BKE_reports_string(ReportList *reports, eReportType level);
void BKE_reports_print(ReportList *reports, eReportType level);

Report *BKE_reports_last_displayable(ReportList *reports);

bool BKE_reports_contain(ReportList *reports, eReportType level);

const char *BKE_report_type_str(eReportType type);

bool BKE_report_write_file_fp(FILE *fp, ReportList *reports, const char *header);
bool BKE_report_write_file(const char *filepath, ReportList *reports, const char *header);

#ifdef __cplusplus
}
#endif
