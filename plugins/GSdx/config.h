/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

//#define ENABLE_VTUNE
//#define ENABLE_PCRTC_DEBUG
//#define ENABLE_ACCURATE_BUFFER_EMULATION
#define ENABLE_JIT_RASTERIZER

//#define DISABLE_HW_TEXTURE_CACHE // Slow but fixes a lot of bugs

//#define DISABLE_BITMASKING

//#define DISABLE_COLCLAMP

//#define DISABLE_DATE


#if !defined(NDEBUG) || defined(_DEBUG) || defined(_DEVEL)
#define ENABLE_OGL_DEBUG   // Create a debug context and check opengl command status. Allow also to dump various textures/states.
//#define ENABLE_OGL_DEBUG_FENCE
//#define ENABLE_OGL_DEBUG_MEM_BW // compute the quantity of data transfered (debug purpose)
//#define ENABLE_TRACE_REG // print GS reg write
//#define ENABLE_EXTRA_LOG // print extra log
#endif

#if defined(__unix__) && !(defined(_DEBUG) || defined(_DEVEL))
#define DISABLE_PERF_MON // Burn cycle for nothing in release mode
#endif
