/*
 * Copyright 2013, Blender Foundation.
 *
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

#ifndef PTC_ALEMBIC_H
#define PTC_ALEMBIC_H

#include <string>

#include "reader.h"
#include "writer.h"

#include "PTC_api.h"

struct Object;
struct ClothModifierData;
struct DynamicPaintSurface;
struct PointCacheModifierData;
struct DerivedMesh;
struct ParticleSystem;
struct RigidBodyWorld;
struct SmokeDomainSettings;
struct SoftBody;

namespace PTC {

WriterArchive *abc_writer_archive(Scene *scene, const std::string &filename, ErrorHandler *error_handler);
ReaderArchive *abc_reader_archive(Scene *scene, const std::string &filename, ErrorHandler *error_handler);

/* Particles */
Writer *abc_writer_particles(WriterArchive *archive, Object *ob, ParticleSystem *psys);
Reader *abc_reader_particles(ReaderArchive *archive, Object *ob, ParticleSystem *psys);
Writer *abc_writer_particle_paths(WriterArchive *archive, Object *ob, ParticleSystem *psys);
Reader *abc_reader_particle_paths(ReaderArchive *archive, Object *ob, ParticleSystem *psys, eParticlePathsMode mode);

/* Cloth */
Writer *abc_writer_cloth(WriterArchive *archive, Object *ob, ClothModifierData *clmd);
Reader *abc_reader_cloth(ReaderArchive *archive, Object *ob, ClothModifierData *clmd);

/* Modifier Stack */
Writer *abc_writer_derived_mesh(WriterArchive *archive, const std::string &name, Object *ob, DerivedMesh **dm_ptr);
Reader *abc_reader_derived_mesh(ReaderArchive *archive, const std::string &name, Object *ob);
Writer *abc_writer_point_cache(WriterArchive *archive, Object *ob, PointCacheModifierData *pcmd);
Reader *abc_reader_point_cache(ReaderArchive *archive, Object *ob, PointCacheModifierData *pcmd);

} /* namespace PTC */

#endif  /* PTC_EXPORT_H */
