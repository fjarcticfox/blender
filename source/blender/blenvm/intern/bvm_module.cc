/*
 * ***** BEGIN GPL LICENSE BLOCK *****
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
 *
 * The Original Code is Copyright (C) Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Lukas Toenne
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file bvm_module.cc
 *  \ingroup bvm
 */

#include <cstdlib> /* needed for BLI_assert */

extern "C" {
#include "BLI_utildefines.h"
#include "BLI_ghash.h"
#include "BLI_string.h"
}

#include "MEM_guardedalloc.h"

#include "bvm_function.h"
#include "bvm_module.h"

namespace bvm {

Module::Module()
{
}

Module::~Module()
{
}

Function *Module::create_function(const string &/*name*/)
{
	// TODO
	return NULL;
}

bool Module::remove_function(const string &/*name*/)
{
	// TODO
	return true;
}

} /* namespace bvm */
