/**
miniIsosurface miniapp License Version 1.0
========================================================================
Copyright (c) 2017
National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
certain rights in this software.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of Kitware nor the names of am_NY contributors may
  be used to endorse or promote products derived from this software
  without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
========================================================================

Original Author: dbourge
Created: Feb 17, 2017
*/

#pragma once

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/complex_export.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace util
{

// clang-format off
const complex::uint8 numTris[256] =
    {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 2,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
        2, 3, 3, 2, 3, 4, 4, 3, 3, 4, 4, 3, 4, 5, 5, 2,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4,
        2, 3, 3, 4, 3, 4, 2, 3, 3, 4, 4, 5, 4, 5, 3, 2,
        3, 4, 4, 3, 4, 5, 3, 2, 4, 5, 5, 4, 5, 2, 4, 1,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 2, 4, 3, 4, 3, 5, 2,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4,
        3, 4, 4, 3, 4, 5, 5, 4, 4, 3, 5, 2, 5, 4, 2, 1,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 2, 3, 3, 2,
        3, 4, 4, 5, 4, 5, 5, 2, 4, 3, 5, 4, 3, 2, 4, 1,
        3, 4, 4, 5, 4, 5, 3, 4, 4, 5, 5, 2, 3, 4, 2, 1,
        2, 3, 3, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2, 1, 1, 0
};

const bool isCut[256][12] =
{
    {false, false, false, false, false, false, false, false, false, false, false, false},
    {true, false, false, true, false, false, false, false, true, false, false, false},
    {false, true, false, true, false, false, false, false, true, true, false, false},
    {false, true, true, false, false, false, false, false, false, false, false, true},
    {true, true, true, true, false, false, false, false, true, false, false, true},
    {true, true, false, false, false, false, false, false, false, true, false, false},
    {true, false, true, false, false, false, false, false, false, true, false, true},
    {false, false, true, true, false, false, false, false, true, true, false, true},
    {false, false, true, true, false, false, false, false, false, false, true, false},
    {true, false, true, false, false, false, false, false, true, false, true, false},
    {true, true, true, true, false, false, false, false, false, true, true, false},
    {false, true, true, false, false, false, false, false, true, true, true, false},
    {false, true, false, true, false, false, false, false, false, false, true, true},
    {true, true, false, false, false, false, false, false, true, false, true, true},
    {true, false, false, true, false, false, false, false, false, true, true, true},
    {false, false, false, false, false, false, false, false, true, true, true, true},
    {false, false, false, false, true, false, false, true, true, false, false, false},
    {true, false, false, true, true, false, false, true, false, false, false, false},
    {true, true, false, false, true, false, false, true, true, true, false, false},
    {false, true, false, true, true, false, false, true, false, true, false, false},
    {false, true, true, false, true, false, false, true, true, false, false, true},
    {true, true, true, true, true, false, false, true, false, false, false, true},
    {true, false, true, false, true, false, false, true, true, true, false, true},
    {false, false, true, true, true, false, false, true, false, true, false, true},
    {false, false, true, true, true, false, false, true, true, false, true, false},
    {true, false, true, false, true, false, false, true, false, false, true, false},
    {true, true, true, true, true, false, false, true, true, true, true, false},
    {false, true, true, false, true, false, false, true, false, true, true, false},
    {false, true, false, true, true, false, false, true, true, false, true, true},
    {true, true, false, false, true, false, false, true, false, false, true, true},
    {true, false, false, true, true, false, false, true, true, true, true, true},
    {false, false, false, false, true, false, false, true, false, true, true, true},
    {false, false, false, false, true, true, false, false, false, true, false, false},
    {true, false, false, true, true, true, false, false, true, true, false, false},
    {true, true, false, false, true, true, false, false, false, false, false, false},
    {false, true, false, true, true, true, false, false, true, false, false, false},
    {false, true, true, false, true, true, false, false, false, true, false, true},
    {true, true, true, true, true, true, false, false, true, true, false, true},
    {true, false, true, false, true, true, false, false, false, false, false, true},
    {false, false, true, true, true, true, false, false, true, false, false, true},
    {false, false, true, true, true, true, false, false, false, true, true, false},
    {true, false, true, false, true, true, false, false, true, true, true, false},
    {true, true, true, true, true, true, false, false, false, false, true, false},
    {false, true, true, false, true, true, false, false, true, false, true, false},
    {false, true, false, true, true, true, false, false, false, true, true, true},
    {true, true, false, false, true, true, false, false, true, true, true, true},
    {true, false, false, true, true, true, false, false, false, false, true, true},
    {false, false, false, false, true, true, false, false, true, false, true, true},
    {false, false, false, false, false, true, false, true, true, true, false, false},
    {true, false, false, true, false, true, false, true, false, true, false, false},
    {true, true, false, false, false, true, false, true, true, false, false, false},
    {false, true, false, true, false, true, false, true, false, false, false, false},
    {false, true, true, false, false, true, false, true, true, true, false, true},
    {true, true, true, true, false, true, false, true, false, true, false, true},
    {true, false, true, false, false, true, false, true, true, false, false, true},
    {false, false, true, true, false, true, false, true, false, false, false, true},
    {false, false, true, true, false, true, false, true, true, true, true, false},
    {true, false, true, false, false, true, false, true, false, true, true, false},
    {true, true, true, true, false, true, false, true, true, false, true, false},
    {false, true, true, false, false, true, false, true, false, false, true, false},
    {false, true, false, true, false, true, false, true, true, true, true, true},
    {true, true, false, false, false, true, false, true, false, true, true, true},
    {true, false, false, true, false, true, false, true, true, false, true, true},
    {false, false, false, false, false, true, false, true, false, false, true, true},
    {false, false, false, false, false, true, true, false, false, false, false, true},
    {true, false, false, true, false, true, true, false, true, false, false, true},
    {true, true, false, false, false, true, true, false, false, true, false, true},
    {false, true, false, true, false, true, true, false, true, true, false, true},
    {false, true, true, false, false, true, true, false, false, false, false, false},
    {true, true, true, true, false, true, true, false, true, false, false, false},
    {true, false, true, false, false, true, true, false, false, true, false, false},
    {false, false, true, true, false, true, true, false, true, true, false, false},
    {false, false, true, true, false, true, true, false, false, false, true, true},
    {true, false, true, false, false, true, true, false, true, false, true, true},
    {true, true, true, true, false, true, true, false, false, true, true, true},
    {false, true, true, false, false, true, true, false, true, true, true, true},
    {false, true, false, true, false, true, true, false, false, false, true, false},
    {true, true, false, false, false, true, true, false, true, false, true, false},
    {true, false, false, true, false, true, true, false, false, true, true, false},
    {false, false, false, false, false, true, true, false, true, true, true, false},
    {false, false, false, false, true, true, true, true, true, false, false, true},
    {true, false, false, true, true, true, true, true, false, false, false, true},
    {true, true, false, false, true, true, true, true, true, true, false, true},
    {false, true, false, true, true, true, true, true, false, true, false, true},
    {false, true, true, false, true, true, true, true, true, false, false, false},
    {true, true, true, true, true, true, true, true, false, false, false, false},
    {true, false, true, false, true, true, true, true, true, true, false, false},
    {false, false, true, true, true, true, true, true, false, true, false, false},
    {false, false, true, true, true, true, true, true, true, false, true, true},
    {true, false, true, false, true, true, true, true, false, false, true, true},
    {true, true, true, true, true, true, true, true, true, true, true, true},
    {false, true, true, false, true, true, true, true, false, true, true, true},
    {false, true, false, true, true, true, true, true, true, false, true, false},
    {true, true, false, false, true, true, true, true, false, false, true, false},
    {true, false, false, true, true, true, true, true, true, true, true, false},
    {false, false, false, false, true, true, true, true, false, true, true, false},
    {false, false, false, false, true, false, true, false, false, true, false, true},
    {true, false, false, true, true, false, true, false, true, true, false, true},
    {true, true, false, false, true, false, true, false, false, false, false, true},
    {false, true, false, true, true, false, true, false, true, false, false, true},
    {false, true, true, false, true, false, true, false, false, true, false, false},
    {true, true, true, true, true, false, true, false, true, true, false, false},
    {true, false, true, false, true, false, true, false, false, false, false, false},
    {false, false, true, true, true, false, true, false, true, false, false, false},
    {false, false, true, true, true, false, true, false, false, true, true, true},
    {true, false, true, false, true, false, true, false, true, true, true, true},
    {true, true, true, true, true, false, true, false, false, false, true, true},
    {false, true, true, false, true, false, true, false, true, false, true, true},
    {false, true, false, true, true, false, true, false, false, true, true, false},
    {true, true, false, false, true, false, true, false, true, true, true, false},
    {true, false, false, true, true, false, true, false, false, false, true, false},
    {false, false, false, false, true, false, true, false, true, false, true, false},
    {false, false, false, false, false, false, true, true, true, true, false, true},
    {true, false, false, true, false, false, true, true, false, true, false, true},
    {true, true, false, false, false, false, true, true, true, false, false, true},
    {false, true, false, true, false, false, true, true, false, false, false, true},
    {false, true, true, false, false, false, true, true, true, true, false, false},
    {true, true, true, true, false, false, true, true, false, true, false, false},
    {true, false, true, false, false, false, true, true, true, false, false, false},
    {false, false, true, true, false, false, true, true, false, false, false, false},
    {false, false, true, true, false, false, true, true, true, true, true, true},
    {true, false, true, false, false, false, true, true, false, true, true, true},
    {true, true, true, true, false, false, true, true, true, false, true, true},
    {false, true, true, false, false, false, true, true, false, false, true, true},
    {false, true, false, true, false, false, true, true, true, true, true, false},
    {true, true, false, false, false, false, true, true, false, true, true, false},
    {true, false, false, true, false, false, true, true, true, false, true, false},
    {false, false, false, false, false, false, true, true, false, false, true, false},
    {false, false, false, false, false, false, true, true, false, false, true, false},
    {true, false, false, true, false, false, true, true, true, false, true, false},
    {true, true, false, false, false, false, true, true, false, true, true, false},
    {false, true, false, true, false, false, true, true, true, true, true, false},
    {false, true, true, false, false, false, true, true, false, false, true, true},
    {true, true, true, true, false, false, true, true, true, false, true, true},
    {true, false, true, false, false, false, true, true, false, true, true, true},
    {false, false, true, true, false, false, true, true, true, true, true, true},
    {false, false, true, true, false, false, true, true, false, false, false, false},
    {true, false, true, false, false, false, true, true, true, false, false, false},
    {true, true, true, true, false, false, true, true, false, true, false, false},
    {false, true, true, false, false, false, true, true, true, true, false, false},
    {false, true, false, true, false, false, true, true, false, false, false, true},
    {true, true, false, false, false, false, true, true, true, false, false, true},
    {true, false, false, true, false, false, true, true, false, true, false, true},
    {false, false, false, false, false, false, true, true, true, true, false, true},
    {false, false, false, false, true, false, true, false, true, false, true, false},
    {true, false, false, true, true, false, true, false, false, false, true, false},
    {true, true, false, false, true, false, true, false, true, true, true, false},
    {false, true, false, true, true, false, true, false, false, true, true, false},
    {false, true, true, false, true, false, true, false, true, false, true, true},
    {true, true, true, true, true, false, true, false, false, false, true, true},
    {true, false, true, false, true, false, true, false, true, true, true, true},
    {false, false, true, true, true, false, true, false, false, true, true, true},
    {false, false, true, true, true, false, true, false, true, false, false, false},
    {true, false, true, false, true, false, true, false, false, false, false, false},
    {true, true, true, true, true, false, true, false, true, true, false, false},
    {false, true, true, false, true, false, true, false, false, true, false, false},
    {false, true, false, true, true, false, true, false, true, false, false, true},
    {true, true, false, false, true, false, true, false, false, false, false, true},
    {true, false, false, true, true, false, true, false, true, true, false, true},
    {false, false, false, false, true, false, true, false, false, true, false, true},
    {false, false, false, false, true, true, true, true, false, true, true, false},
    {true, false, false, true, true, true, true, true, true, true, true, false},
    {true, true, false, false, true, true, true, true, false, false, true, false},
    {false, true, false, true, true, true, true, true, true, false, true, false},
    {false, true, true, false, true, true, true, true, false, true, true, true},
    {true, true, true, true, true, true, true, true, true, true, true, true},
    {true, false, true, false, true, true, true, true, false, false, true, true},
    {false, false, true, true, true, true, true, true, true, false, true, true},
    {false, false, true, true, true, true, true, true, false, true, false, false},
    {true, false, true, false, true, true, true, true, true, true, false, false},
    {true, true, true, true, true, true, true, true, false, false, false, false},
    {false, true, true, false, true, true, true, true, true, false, false, false},
    {false, true, false, true, true, true, true, true, false, true, false, true},
    {true, true, false, false, true, true, true, true, true, true, false, true},
    {true, false, false, true, true, true, true, true, false, false, false, true},
    {false, false, false, false, true, true, true, true, true, false, false, true},
    {false, false, false, false, false, true, true, false, true, true, true, false},
    {true, false, false, true, false, true, true, false, false, true, true, false},
    {true, true, false, false, false, true, true, false, true, false, true, false},
    {false, true, false, true, false, true, true, false, false, false, true, false},
    {false, true, true, false, false, true, true, false, true, true, true, true},
    {true, true, true, true, false, true, true, false, false, true, true, true},
    {true, false, true, false, false, true, true, false, true, false, true, true},
    {false, false, true, true, false, true, true, false, false, false, true, true},
    {false, false, true, true, false, true, true, false, true, true, false, false},
    {true, false, true, false, false, true, true, false, false, true, false, false},
    {true, true, true, true, false, true, true, false, true, false, false, false},
    {false, true, true, false, false, true, true, false, false, false, false, false},
    {false, true, false, true, false, true, true, false, true, true, false, true},
    {true, true, false, false, false, true, true, false, false, true, false, true},
    {true, false, false, true, false, true, true, false, true, false, false, true},
    {false, false, false, false, false, true, true, false, false, false, false, true},
    {false, false, false, false, false, true, false, true, false, false, true, true},
    {true, false, false, true, false, true, false, true, true, false, true, true},
    {true, true, false, false, false, true, false, true, false, true, true, true},
    {false, true, false, true, false, true, false, true, true, true, true, true},
    {false, true, true, false, false, true, false, true, false, false, true, false},
    {true, true, true, true, false, true, false, true, true, false, true, false},
    {true, false, true, false, false, true, false, true, false, true, true, false},
    {false, false, true, true, false, true, false, true, true, true, true, false},
    {false, false, true, true, false, true, false, true, false, false, false, true},
    {true, false, true, false, false, true, false, true, true, false, false, true},
    {true, true, true, true, false, true, false, true, false, true, false, true},
    {false, true, true, false, false, true, false, true, true, true, false, true},
    {false, true, false, true, false, true, false, true, false, false, false, false},
    {true, true, false, false, false, true, false, true, true, false, false, false},
    {true, false, false, true, false, true, false, true, false, true, false, false},
    {false, false, false, false, false, true, false, true, true, true, false, false},
    {false, false, false, false, true, true, false, false, true, false, true, true},
    {true, false, false, true, true, true, false, false, false, false, true, true},
    {true, true, false, false, true, true, false, false, true, true, true, true},
    {false, true, false, true, true, true, false, false, false, true, true, true},
    {false, true, true, false, true, true, false, false, true, false, true, false},
    {true, true, true, true, true, true, false, false, false, false, true, false},
    {true, false, true, false, true, true, false, false, true, true, true, false},
    {false, false, true, true, true, true, false, false, false, true, true, false},
    {false, false, true, true, true, true, false, false, true, false, false, true},
    {true, false, true, false, true, true, false, false, false, false, false, true},
    {true, true, true, true, true, true, false, false, true, true, false, true},
    {false, true, true, false, true, true, false, false, false, true, false, true},
    {false, true, false, true, true, true, false, false, true, false, false, false},
    {true, true, false, false, true, true, false, false, false, false, false, false},
    {true, false, false, true, true, true, false, false, true, true, false, false},
    {false, false, false, false, true, true, false, false, false, true, false, false},
    {false, false, false, false, true, false, false, true, false, true, true, true},
    {true, false, false, true, true, false, false, true, true, true, true, true},
    {true, true, false, false, true, false, false, true, false, false, true, true},
    {false, true, false, true, true, false, false, true, true, false, true, true},
    {false, true, true, false, true, false, false, true, false, true, true, false},
    {true, true, true, true, true, false, false, true, true, true, true, false},
    {true, false, true, false, true, false, false, true, false, false, true, false},
    {false, false, true, true, true, false, false, true, true, false, true, false},
    {false, false, true, true, true, false, false, true, false, true, false, true},
    {true, false, true, false, true, false, false, true, true, true, false, true},
    {true, true, true, true, true, false, false, true, false, false, false, true},
    {false, true, true, false, true, false, false, true, true, false, false, true},
    {false, true, false, true, true, false, false, true, false, true, false, false},
    {true, true, false, false, true, false, false, true, true, true, false, false},
    {true, false, false, true, true, false, false, true, false, false, false, false},
    {false, false, false, false, true, false, false, true, true, false, false, false},
    {false, false, false, false, false, false, false, false, true, true, true, true},
    {true, false, false, true, false, false, false, false, false, true, true, true},
    {true, true, false, false, false, false, false, false, true, false, true, true},
    {false, true, false, true, false, false, false, false, false, false, true, true},
    {false, true, true, false, false, false, false, false, true, true, true, false},
    {true, true, true, true, false, false, false, false, false, true, true, false},
    {true, false, true, false, false, false, false, false, true, false, true, false},
    {false, false, true, true, false, false, false, false, false, false, true, false},
    {false, false, true, true, false, false, false, false, true, true, false, true},
    {true, false, true, false, false, false, false, false, false, true, false, true},
    {true, true, true, true, false, false, false, false, true, false, false, true},
    {false, true, true, false, false, false, false, false, false, false, false, true},
    {false, true, false, true, false, false, false, false, true, true, false, false},
    {true, true, false, false, false, false, false, false, false, true, false, false},
    {true, false, false, true, false, false, false, false, true, false, false, false},
    {false, false, false, false, false, false, false, false, false, false, false, false}
};

const char caseTriangles[256][16]
    {
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  3,  8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  9,  1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  3,  8,  9,  1,  8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  11, 2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  3,  8,  1,  11, 2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9,  11, 2,  0,  9,  2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2,  3,  8,  2,  8,  11, 11, 8,  9,  -1, -1, -1, -1, -1, -1, -1},
        {3,  2,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  2,  10, 8,  0,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  0,  9,  2,  10, 3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  2,  10, 1,  10, 9,  9,  10, 8,  -1, -1, -1, -1, -1, -1, -1},
        {3,  1,  11, 10, 3,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  1,  11, 0,  11, 8,  8,  11, 10, -1, -1, -1, -1, -1, -1, -1},
        {3,  0,  9,  3,  9,  10, 10, 9,  11, -1, -1, -1, -1, -1, -1, -1},
        {9,  11, 8,  11, 10, 8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4,  8,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4,  0,  3,  7,  4,  3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  9,  1,  8,  7,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4,  9,  1,  4,  1,  7,  7,  1,  3,  -1, -1, -1, -1, -1, -1, -1},
        {1,  11, 2,  8,  7,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3,  7,  4,  3,  4,  0,  1,  11, 2,  -1, -1, -1, -1, -1, -1, -1},
        {9,  11, 2,  9,  2,  0,  8,  7,  4,  -1, -1, -1, -1, -1, -1, -1},
        {2,  9,  11, 2,  7,  9,  2,  3,  7,  7,  4,  9,  -1, -1, -1, -1},
        {8,  7,  4,  3,  2,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {10, 7,  4,  10, 4,  2,  2,  4,  0,  -1, -1, -1, -1, -1, -1, -1},
        {9,  1,  0,  8,  7,  4,  2,  10, 3,  -1, -1, -1, -1, -1, -1, -1},
        {4,  10, 7,  9,  10, 4,  9,  2,  10, 9,  1,  2,  -1, -1, -1, -1},
        {3,  1,  11, 3,  11, 10, 7,  4,  8,  -1, -1, -1, -1, -1, -1, -1},
        {1,  11, 10, 1,  10, 4,  1,  4,  0,  7,  4,  10, -1, -1, -1, -1},
        {4,  8,  7,  9,  10, 0,  9,  11, 10, 10, 3,  0,  -1, -1, -1, -1},
        {4,  10, 7,  4,  9,  10, 9,  11, 10, -1, -1, -1, -1, -1, -1, -1},
        {9,  4,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9,  4,  5,  0,  3,  8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  4,  5,  1,  0,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8,  4,  5,  8,  5,  3,  3,  5,  1,  -1, -1, -1, -1, -1, -1, -1},
        {1,  11, 2,  9,  4,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3,  8,  0,  1,  11, 2,  4,  5,  9,  -1, -1, -1, -1, -1, -1, -1},
        {5,  11, 2,  5,  2,  4,  4,  2,  0,  -1, -1, -1, -1, -1, -1, -1},
        {2,  5,  11, 3,  5,  2,  3,  4,  5,  3,  8,  4,  -1, -1, -1, -1},
        {9,  4,  5,  2,  10, 3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  2,  10, 0,  10, 8,  4,  5,  9,  -1, -1, -1, -1, -1, -1, -1},
        {0,  4,  5,  0,  5,  1,  2,  10, 3,  -1, -1, -1, -1, -1, -1, -1},
        {2,  5,  1,  2,  8,  5,  2,  10, 8,  4,  5,  8,  -1, -1, -1, -1},
        {11, 10, 3,  11, 3,  1,  9,  4,  5,  -1, -1, -1, -1, -1, -1, -1},
        {4,  5,  9,  0,  1,  8,  8,  1,  11, 8,  11, 10, -1, -1, -1, -1},
        {5,  0,  4,  5,  10, 0,  5,  11, 10, 10, 3,  0,  -1, -1, -1, -1},
        {5,  8,  4,  5,  11, 8,  11, 10, 8,  -1, -1, -1, -1, -1, -1, -1},
        {9,  8,  7,  5,  9,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9,  0,  3,  9,  3,  5,  5,  3,  7,  -1, -1, -1, -1, -1, -1, -1},
        {0,  8,  7,  0,  7,  1,  1,  7,  5,  -1, -1, -1, -1, -1, -1, -1},
        {1,  3,  5,  3,  7,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9,  8,  7,  9,  7,  5,  11, 2,  1,  -1, -1, -1, -1, -1, -1, -1},
        {11, 2,  1,  9,  0,  5,  5,  0,  3,  5,  3,  7,  -1, -1, -1, -1},
        {8,  2,  0,  8,  5,  2,  8,  7,  5,  11, 2,  5,  -1, -1, -1, -1},
        {2,  5,  11, 2,  3,  5,  3,  7,  5,  -1, -1, -1, -1, -1, -1, -1},
        {7,  5,  9,  7,  9,  8,  3,  2,  10, -1, -1, -1, -1, -1, -1, -1},
        {9,  7,  5,  9,  2,  7,  9,  0,  2,  2,  10, 7,  -1, -1, -1, -1},
        {2,  10, 3,  0,  8,  1,  1,  8,  7,  1,  7,  5,  -1, -1, -1, -1},
        {10, 1,  2,  10, 7,  1,  7,  5,  1,  -1, -1, -1, -1, -1, -1, -1},
        {9,  8,  5,  8,  7,  5,  11, 3,  1,  11, 10, 3,  -1, -1, -1, -1},
        {5,  0,  7,  5,  9,  0,  7,  0,  10, 1,  11, 0,  10, 0,  11, -1},
        {10, 0,  11, 10, 3,  0,  11, 0,  5,  8,  7,  0,  5,  0,  7,  -1},
        {10, 5,  11, 7,  5,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 5,  6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  3,  8,  5,  6,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9,  1,  0,  5,  6,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  3,  8,  1,  8,  9,  5,  6,  11, -1, -1, -1, -1, -1, -1, -1},
        {1,  5,  6,  2,  1,  6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  5,  6,  1,  6,  2,  3,  8,  0,  -1, -1, -1, -1, -1, -1, -1},
        {9,  5,  6,  9,  6,  0,  0,  6,  2,  -1, -1, -1, -1, -1, -1, -1},
        {5,  8,  9,  5,  2,  8,  5,  6,  2,  3,  8,  2,  -1, -1, -1, -1},
        {2,  10, 3,  11, 5,  6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {10, 8,  0,  10, 0,  2,  11, 5,  6,  -1, -1, -1, -1, -1, -1, -1},
        {0,  9,  1,  2,  10, 3,  5,  6,  11, -1, -1, -1, -1, -1, -1, -1},
        {5,  6,  11, 1,  2,  9,  9,  2,  10, 9,  10, 8,  -1, -1, -1, -1},
        {6,  10, 3,  6,  3,  5,  5,  3,  1,  -1, -1, -1, -1, -1, -1, -1},
        {0,  10, 8,  0,  5,  10, 0,  1,  5,  5,  6,  10, -1, -1, -1, -1},
        {3,  6,  10, 0,  6,  3,  0,  5,  6,  0,  9,  5,  -1, -1, -1, -1},
        {6,  9,  5,  6,  10, 9,  10, 8,  9,  -1, -1, -1, -1, -1, -1, -1},
        {5,  6,  11, 4,  8,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4,  0,  3,  4,  3,  7,  6,  11, 5,  -1, -1, -1, -1, -1, -1, -1},
        {1,  0,  9,  5,  6,  11, 8,  7,  4,  -1, -1, -1, -1, -1, -1, -1},
        {11, 5,  6,  1,  7,  9,  1,  3,  7,  7,  4,  9,  -1, -1, -1, -1},
        {6,  2,  1,  6,  1,  5,  4,  8,  7,  -1, -1, -1, -1, -1, -1, -1},
        {1,  5,  2,  5,  6,  2,  3,  4,  0,  3,  7,  4,  -1, -1, -1, -1},
        {8,  7,  4,  9,  5,  0,  0,  5,  6,  0,  6,  2,  -1, -1, -1, -1},
        {7,  9,  3,  7,  4,  9,  3,  9,  2,  5,  6,  9,  2,  9,  6,  -1},
        {3,  2,  10, 7,  4,  8,  11, 5,  6,  -1, -1, -1, -1, -1, -1, -1},
        {5,  6,  11, 4,  2,  7,  4,  0,  2,  2,  10, 7,  -1, -1, -1, -1},
        {0,  9,  1,  4,  8,  7,  2,  10, 3,  5,  6,  11, -1, -1, -1, -1},
        {9,  1,  2,  9,  2,  10, 9,  10, 4,  7,  4,  10, 5,  6,  11, -1},
        {8,  7,  4,  3,  5,  10, 3,  1,  5,  5,  6,  10, -1, -1, -1, -1},
        {5,  10, 1,  5,  6,  10, 1,  10, 0,  7,  4,  10, 0,  10, 4,  -1},
        {0,  9,  5,  0,  5,  6,  0,  6,  3,  10, 3,  6,  8,  7,  4,  -1},
        {6,  9,  5,  6,  10, 9,  4,  9,  7,  7,  9,  10, -1, -1, -1, -1},
        {11, 9,  4,  6,  11, 4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4,  6,  11, 4,  11, 9,  0,  3,  8,  -1, -1, -1, -1, -1, -1, -1},
        {11, 1,  0,  11, 0,  6,  6,  0,  4,  -1, -1, -1, -1, -1, -1, -1},
        {8,  1,  3,  8,  6,  1,  8,  4,  6,  6,  11, 1,  -1, -1, -1, -1},
        {1,  9,  4,  1,  4,  2,  2,  4,  6,  -1, -1, -1, -1, -1, -1, -1},
        {3,  8,  0,  1,  9,  2,  2,  9,  4,  2,  4,  6,  -1, -1, -1, -1},
        {0,  4,  2,  4,  6,  2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8,  2,  3,  8,  4,  2,  4,  6,  2,  -1, -1, -1, -1, -1, -1, -1},
        {11, 9,  4,  11, 4,  6,  10, 3,  2,  -1, -1, -1, -1, -1, -1, -1},
        {0,  2,  8,  2,  10, 8,  4,  11, 9,  4,  6,  11, -1, -1, -1, -1},
        {3,  2,  10, 0,  6,  1,  0,  4,  6,  6,  11, 1,  -1, -1, -1, -1},
        {6,  1,  4,  6,  11, 1,  4,  1,  8,  2,  10, 1,  8,  1,  10, -1},
        {9,  4,  6,  9,  6,  3,  9,  3,  1,  10, 3,  6,  -1, -1, -1, -1},
        {8,  1,  10, 8,  0,  1,  10, 1,  6,  9,  4,  1,  6,  1,  4,  -1},
        {3,  6,  10, 3,  0,  6,  0,  4,  6,  -1, -1, -1, -1, -1, -1, -1},
        {6,  8,  4,  10, 8,  6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7,  6,  11, 7,  11, 8,  8,  11, 9,  -1, -1, -1, -1, -1, -1, -1},
        {0,  3,  7,  0,  7,  11, 0,  11, 9,  6,  11, 7,  -1, -1, -1, -1},
        {11, 7,  6,  1,  7,  11, 1,  8,  7,  1,  0,  8,  -1, -1, -1, -1},
        {11, 7,  6,  11, 1,  7,  1,  3,  7,  -1, -1, -1, -1, -1, -1, -1},
        {1,  6,  2,  1,  8,  6,  1,  9,  8,  8,  7,  6,  -1, -1, -1, -1},
        {2,  9,  6,  2,  1,  9,  6,  9,  7,  0,  3,  9,  7,  9,  3,  -1},
        {7,  0,  8,  7,  6,  0,  6,  2,  0,  -1, -1, -1, -1, -1, -1, -1},
        {7,  2,  3,  6,  2,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2,  10, 3,  11, 8,  6,  11, 9,  8,  8,  7,  6,  -1, -1, -1, -1},
        {2,  7,  0,  2,  10, 7,  0,  7,  9,  6,  11, 7,  9,  7,  11, -1},
        {1,  0,  8,  1,  8,  7,  1,  7,  11, 6,  11, 7,  2,  10, 3,  -1},
        {10, 1,  2,  10, 7,  1,  11, 1,  6,  6,  1,  7,  -1, -1, -1, -1},
        {8,  6,  9,  8,  7,  6,  9,  6,  1,  10, 3,  6,  1,  6,  3,  -1},
        {0,  1,  9,  10, 7,  6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7,  0,  8,  7,  6,  0,  3,  0,  10, 10, 0,  6,  -1, -1, -1, -1},
        {7,  6,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7,  10, 6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3,  8,  0,  10, 6,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  9,  1,  10, 6,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8,  9,  1,  8,  1,  3,  10, 6,  7,  -1, -1, -1, -1, -1, -1, -1},
        {11, 2,  1,  6,  7,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  11, 2,  3,  8,  0,  6,  7,  10, -1, -1, -1, -1, -1, -1, -1},
        {2,  0,  9,  2,  9,  11, 6,  7,  10, -1, -1, -1, -1, -1, -1, -1},
        {6,  7,  10, 2,  3,  11, 11, 3,  8,  11, 8,  9,  -1, -1, -1, -1},
        {7,  3,  2,  6,  7,  2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7,  8,  0,  7,  0,  6,  6,  0,  2,  -1, -1, -1, -1, -1, -1, -1},
        {2,  6,  7,  2,  7,  3,  0,  9,  1,  -1, -1, -1, -1, -1, -1, -1},
        {1,  2,  6,  1,  6,  8,  1,  8,  9,  8,  6,  7,  -1, -1, -1, -1},
        {11, 6,  7,  11, 7,  1,  1,  7,  3,  -1, -1, -1, -1, -1, -1, -1},
        {11, 6,  7,  1,  11, 7,  1,  7,  8,  1,  8,  0,  -1, -1, -1, -1},
        {0,  7,  3,  0,  11, 7,  0,  9,  11, 6,  7,  11, -1, -1, -1, -1},
        {7,  11, 6,  7,  8,  11, 8,  9,  11, -1, -1, -1, -1, -1, -1, -1},
        {6,  4,  8,  10, 6,  8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3,  10, 6,  3,  6,  0,  0,  6,  4,  -1, -1, -1, -1, -1, -1, -1},
        {8,  10, 6,  8,  6,  4,  9,  1,  0,  -1, -1, -1, -1, -1, -1, -1},
        {9,  6,  4,  9,  3,  6,  9,  1,  3,  10, 6,  3,  -1, -1, -1, -1},
        {6,  4,  8,  6,  8,  10, 2,  1,  11, -1, -1, -1, -1, -1, -1, -1},
        {1,  11, 2,  3,  10, 0,  0,  10, 6,  0,  6,  4,  -1, -1, -1, -1},
        {4,  8,  10, 4,  10, 6,  0,  9,  2,  2,  9,  11, -1, -1, -1, -1},
        {11, 3,  9,  11, 2,  3,  9,  3,  4,  10, 6,  3,  4,  3,  6,  -1},
        {8,  3,  2,  8,  2,  4,  4,  2,  6,  -1, -1, -1, -1, -1, -1, -1},
        {0,  2,  4,  4,  2,  6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  0,  9,  2,  4,  3,  2,  6,  4,  4,  8,  3,  -1, -1, -1, -1},
        {1,  4,  9,  1,  2,  4,  2,  6,  4,  -1, -1, -1, -1, -1, -1, -1},
        {8,  3,  1,  8,  1,  6,  8,  6,  4,  6,  1,  11, -1, -1, -1, -1},
        {11, 0,  1,  11, 6,  0,  6,  4,  0,  -1, -1, -1, -1, -1, -1, -1},
        {4,  3,  6,  4,  8,  3,  6,  3,  11, 0,  9,  3,  11, 3,  9,  -1},
        {11, 4,  9,  6,  4,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4,  5,  9,  7,  10, 6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  3,  8,  4,  5,  9,  10, 6,  7,  -1, -1, -1, -1, -1, -1, -1},
        {5,  1,  0,  5,  0,  4,  7,  10, 6,  -1, -1, -1, -1, -1, -1, -1},
        {10, 6,  7,  8,  4,  3,  3,  4,  5,  3,  5,  1,  -1, -1, -1, -1},
        {9,  4,  5,  11, 2,  1,  7,  10, 6,  -1, -1, -1, -1, -1, -1, -1},
        {6,  7,  10, 1,  11, 2,  0,  3,  8,  4,  5,  9,  -1, -1, -1, -1},
        {7,  10, 6,  5,  11, 4,  4,  11, 2,  4,  2,  0,  -1, -1, -1, -1},
        {3,  8,  4,  3,  4,  5,  3,  5,  2,  11, 2,  5,  10, 6,  7,  -1},
        {7,  3,  2,  7,  2,  6,  5,  9,  4,  -1, -1, -1, -1, -1, -1, -1},
        {9,  4,  5,  0,  6,  8,  0,  2,  6,  6,  7,  8,  -1, -1, -1, -1},
        {3,  2,  6,  3,  6,  7,  1,  0,  5,  5,  0,  4,  -1, -1, -1, -1},
        {6,  8,  2,  6,  7,  8,  2,  8,  1,  4,  5,  8,  1,  8,  5,  -1},
        {9,  4,  5,  11, 6,  1,  1,  6,  7,  1,  7,  3,  -1, -1, -1, -1},
        {1,  11, 6,  1,  6,  7,  1,  7,  0,  8,  0,  7,  9,  4,  5,  -1},
        {4,  11, 0,  4,  5,  11, 0,  11, 3,  6,  7,  11, 3,  11, 7,  -1},
        {7,  11, 6,  7,  8,  11, 5,  11, 4,  4,  11, 8,  -1, -1, -1, -1},
        {6,  5,  9,  6,  9,  10, 10, 9,  8,  -1, -1, -1, -1, -1, -1, -1},
        {3,  10, 6,  0,  3,  6,  0,  6,  5,  0,  5,  9,  -1, -1, -1, -1},
        {0,  8,  10, 0,  10, 5,  0,  5,  1,  5,  10, 6,  -1, -1, -1, -1},
        {6,  3,  10, 6,  5,  3,  5,  1,  3,  -1, -1, -1, -1, -1, -1, -1},
        {1,  11, 2,  9,  10, 5,  9,  8,  10, 10, 6,  5,  -1, -1, -1, -1},
        {0,  3,  10, 0,  10, 6,  0,  6,  9,  5,  9,  6,  1,  11, 2,  -1},
        {10, 5,  8,  10, 6,  5,  8,  5,  0,  11, 2,  5,  0,  5,  2,  -1},
        {6,  3,  10, 6,  5,  3,  2,  3,  11, 11, 3,  5,  -1, -1, -1, -1},
        {5,  9,  8,  5,  8,  2,  5,  2,  6,  3,  2,  8,  -1, -1, -1, -1},
        {9,  6,  5,  9,  0,  6,  0,  2,  6,  -1, -1, -1, -1, -1, -1, -1},
        {1,  8,  5,  1,  0,  8,  5,  8,  6,  3,  2,  8,  6,  8,  2,  -1},
        {1,  6,  5,  2,  6,  1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  6,  3,  1,  11, 6,  3,  6,  8,  5,  9,  6,  8,  6,  9,  -1},
        {11, 0,  1,  11, 6,  0,  9,  0,  5,  5,  0,  6,  -1, -1, -1, -1},
        {0,  8,  3,  5,  11, 6,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 6,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {10, 11, 5,  7,  10, 5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {10, 11, 5,  10, 5,  7,  8,  0,  3,  -1, -1, -1, -1, -1, -1, -1},
        {5,  7,  10, 5,  10, 11, 1,  0,  9,  -1, -1, -1, -1, -1, -1, -1},
        {11, 5,  7,  11, 7,  10, 9,  1,  8,  8,  1,  3,  -1, -1, -1, -1},
        {10, 2,  1,  10, 1,  7,  7,  1,  5,  -1, -1, -1, -1, -1, -1, -1},
        {0,  3,  8,  1,  7,  2,  1,  5,  7,  7,  10, 2,  -1, -1, -1, -1},
        {9,  5,  7,  9,  7,  2,  9,  2,  0,  2,  7,  10, -1, -1, -1, -1},
        {7,  2,  5,  7,  10, 2,  5,  2,  9,  3,  8,  2,  9,  2,  8,  -1},
        {2,  11, 5,  2,  5,  3,  3,  5,  7,  -1, -1, -1, -1, -1, -1, -1},
        {8,  0,  2,  8,  2,  5,  8,  5,  7,  11, 5,  2,  -1, -1, -1, -1},
        {9,  1,  0,  5,  3,  11, 5,  7,  3,  3,  2,  11, -1, -1, -1, -1},
        {9,  2,  8,  9,  1,  2,  8,  2,  7,  11, 5,  2,  7,  2,  5,  -1},
        {1,  5,  3,  3,  5,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  7,  8,  0,  1,  7,  1,  5,  7,  -1, -1, -1, -1, -1, -1, -1},
        {9,  3,  0,  9,  5,  3,  5,  7,  3,  -1, -1, -1, -1, -1, -1, -1},
        {9,  7,  8,  5,  7,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {5,  4,  8,  5,  8,  11, 11, 8,  10, -1, -1, -1, -1, -1, -1, -1},
        {5,  4,  0,  5,  0,  10, 5,  10, 11, 10, 0,  3,  -1, -1, -1, -1},
        {0,  9,  1,  8,  11, 4,  8,  10, 11, 11, 5,  4,  -1, -1, -1, -1},
        {11, 4,  10, 11, 5,  4,  10, 4,  3,  9,  1,  4,  3,  4,  1,  -1},
        {2,  1,  5,  2,  5,  8,  2,  8,  10, 4,  8,  5,  -1, -1, -1, -1},
        {0,  10, 4,  0,  3,  10, 4,  10, 5,  2,  1,  10, 5,  10, 1,  -1},
        {0,  5,  2,  0,  9,  5,  2,  5,  10, 4,  8,  5,  10, 5,  8,  -1},
        {9,  5,  4,  2,  3,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2,  11, 5,  3,  2,  5,  3,  5,  4,  3,  4,  8,  -1, -1, -1, -1},
        {5,  2,  11, 5,  4,  2,  4,  0,  2,  -1, -1, -1, -1, -1, -1, -1},
        {3,  2,  11, 3,  11, 5,  3,  5,  8,  4,  8,  5,  0,  9,  1,  -1},
        {5,  2,  11, 5,  4,  2,  1,  2,  9,  9,  2,  4,  -1, -1, -1, -1},
        {8,  5,  4,  8,  3,  5,  3,  1,  5,  -1, -1, -1, -1, -1, -1, -1},
        {0,  5,  4,  1,  5,  0,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8,  5,  4,  8,  3,  5,  9,  5,  0,  0,  5,  3,  -1, -1, -1, -1},
        {9,  5,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4,  7,  10, 4,  10, 9,  9,  10, 11, -1, -1, -1, -1, -1, -1, -1},
        {0,  3,  8,  4,  7,  9,  9,  7,  10, 9,  10, 11, -1, -1, -1, -1},
        {1,  10, 11, 1,  4,  10, 1,  0,  4,  7,  10, 4,  -1, -1, -1, -1},
        {3,  4,  1,  3,  8,  4,  1,  4,  11, 7,  10, 4,  11, 4,  10, -1},
        {4,  7,  10, 9,  4,  10, 9,  10, 2,  9,  2,  1,  -1, -1, -1, -1},
        {9,  4,  7,  9,  7,  10, 9,  10, 1,  2,  1,  10, 0,  3,  8,  -1},
        {10, 4,  7,  10, 2,  4,  2,  0,  4,  -1, -1, -1, -1, -1, -1, -1},
        {10, 4,  7,  10, 2,  4,  8,  4,  3,  3,  4,  2,  -1, -1, -1, -1},
        {2,  11, 9,  2,  9,  7,  2,  7,  3,  7,  9,  4,  -1, -1, -1, -1},
        {9,  7,  11, 9,  4,  7,  11, 7,  2,  8,  0,  7,  2,  7,  0,  -1},
        {3,  11, 7,  3,  2,  11, 7,  11, 4,  1,  0,  11, 4,  11, 0,  -1},
        {1,  2,  11, 8,  4,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4,  1,  9,  4,  7,  1,  7,  3,  1,  -1, -1, -1, -1, -1, -1, -1},
        {4,  1,  9,  4,  7,  1,  0,  1,  8,  8,  1,  7,  -1, -1, -1, -1},
        {4,  3,  0,  7,  3,  4,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4,  7,  8,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9,  8,  11, 11, 8,  10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3,  9,  0,  3,  10, 9,  10, 11, 9,  -1, -1, -1, -1, -1, -1, -1},
        {0,  11, 1,  0,  8,  11, 8,  10, 11, -1, -1, -1, -1, -1, -1, -1},
        {3,  11, 1,  10, 11, 3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  10, 2,  1,  9,  10, 9,  8,  10, -1, -1, -1, -1, -1, -1, -1},
        {3,  9,  0,  3,  10, 9,  1,  9,  2,  2,  9,  10, -1, -1, -1, -1},
        {0,  10, 2,  8,  10, 0,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3,  10, 2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2,  8,  3,  2,  11, 8,  11, 9,  8,  -1, -1, -1, -1, -1, -1, -1},
        {9,  2,  11, 0,  2,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2,  8,  3,  2,  11, 8,  0,  8,  1,  1,  8,  11, -1, -1, -1, -1},
        {1,  2,  11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1,  8,  3,  9,  8,  1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  1,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0,  8,  3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
    };

const complex::uint8 edgeVertices[12][2] = { {0,1}, {1,2}, {3,2},
                                   {0,3}, {4,5}, {5,6},
                                   {7,6}, {4,7}, {0,4},
                                   {1,5}, {3,7}, {2,6} };
// clang-format on
} // namespace util

namespace complex
{
template <typename T>
class COMPLEX_EXPORT FlyingEdgesAlgorithm
{
  using cube = std::array<std::array<T, 3>, 8>;
  using TCube = std::array<T, 8>;

public:
  FlyingEdgesAlgorithm(const ImageGeom& image, const IDataArray& iDataArray, const T isoVal, TriangleGeom& triangleGeom, Float32Array& normals)
  : m_Image(image)
  , m_DataArray(dynamic_cast<const DataArray<T>&>(iDataArray))
  , m_IsoVal(isoVal)
  , m_TriangleGeom(triangleGeom)
  , m_NX(image.getDimensions()[0])
  , m_NY(image.getDimensions()[1])
  , m_NZ(image.getDimensions()[2])
  , m_GridEdges(m_NY * m_NZ)
  , m_TriCounter((m_NY - 1) * (m_NZ - 1))
  , m_EdgeCases((m_NX - 1) * m_NY * m_NZ)
  , m_CubeCases((m_NX - 1) * (m_NY - 1) * (m_NZ - 1))
  , m_Tris(m_TriangleGeom.getFacesRef())
  , m_Points(m_TriangleGeom.getVerticesRef())
  , m_Normals(normals)
  {
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Pass 1 of the algorithm
  ///////////////////////////////////////////////////////////////////////////////
  void pass1()
  {
    // For each (j, k):
    //  - for each edge i along fixed (j, k) gridEdge, fill m_EdgeCases with
    //    cut information.
    //  - find the locations for computational trimming, xl and xr
    //  To properly find xl and xr, have to check along the x-axis,
    //  the y-axis and the z-axis!
    for(usize k = 0; k != m_NZ; ++k)
    {
      for(usize j = 0; j != m_NY; ++j)
      {
        auto curEdgeCases = m_EdgeCases.begin() + (m_NX - 1) * (k * m_NY + j);
        T curPointValue = getData(0, j, k);

        std::array<bool, 2> isGE = {};
        isGE[0] = (curPointValue >= m_IsoVal);
        for(int i = 1; i != m_NX; ++i)
        {
          isGE[i % 2] = (getData(i, j, k) >= m_IsoVal);

          curEdgeCases[i - 1] = calcCaseEdge(isGE[(i + 1) % 2], isGE[i % 2]);
        }
      }
    }

    for(usize k = 0; k != m_NZ; ++k)
    {
      for(usize j = 0; j != m_NY; ++j)
      {
        gridEdge& curGridEdge = m_GridEdges[k * m_NY + j];
        curGridEdge.xl = m_NX;
        for(int i = 1; i != m_NX; ++i)
        {
          // If the edge is cut
          if(isCutEdge(i - 1, j, k))
          {
            if(curGridEdge.xl == m_NX)
            {
              curGridEdge.xl = i - 1;
            }

            curGridEdge.xr = i;
          }
        }
      }
    }
  }
  ///////////////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////////////
  // Pass 2 of the algorithm
  ///////////////////////////////////////////////////////////////////////////////
  void pass2()
  {
    // For each (j, k):
    //  - for each cube (i, j, k) calculate caseId and number of gridEdge cuts
    //    in the x, y and z direction.
    for(usize k = 0; k != m_NZ - 1; ++k)
    {
      for(usize j = 0; j != m_NY - 1; ++j)
      {
        // find adjusted trim values
        usize xl, xr;
        calcTrimValues(xl, xr, j, k); // xl, xr set in this function

        // ge0 is owned by this (i, j, k). ge1, ge2 and ge3 are only used for
        // boundary cells.
        gridEdge& ge0 = m_GridEdges[k * m_NY + j];
        gridEdge& ge1 = m_GridEdges[k * m_NY + j + 1];
        gridEdge& ge2 = m_GridEdges[(k + 1) * m_NY + j];
        gridEdge& ge3 = m_GridEdges[(k + 1) * m_NY + j + 1];

        // ec0, ec1, ec2 and ec3 were set in pass 1. They are used
        // to calculate the cell caseId.
        auto const& ec0 = m_EdgeCases.begin() + (m_NX - 1) * (k * m_NY + j);
        auto const& ec1 = m_EdgeCases.begin() + (m_NX - 1) * (k * m_NY + j + 1);
        auto const& ec2 = m_EdgeCases.begin() + (m_NX - 1) * ((k + 1) * m_NY + j);
        auto const& ec3 = m_EdgeCases.begin() + (m_NX - 1) * ((k + 1) * m_NY + j + 1);

        // Count the number of triangles along this row of cubes.
        usize& curTriCounter = *(m_TriCounter.begin() + k * (m_NY - 1) + static_cast<int64>(j));

        auto curCubeCaseIds = m_CubeCases.begin() + (m_NX - 1) * (k * (m_NY - 1) + j);

        bool isYEnd = (j == m_NY - 2);
        bool isZEnd = (k == m_NZ - 2);

        for(usize i = xl; i != xr; ++i)
        {
          bool isXEnd = (i == m_NX - 2);

          // using m_EdgeCases from pass 2, compute m_CubeCases for this cube
          uint8 caseId = calcCubeCase(ec0[static_cast<int64>(i)], ec1[static_cast<int64>(i)], ec2[static_cast<int64>(i)], ec3[static_cast<int64>(i)]);

          curCubeCaseIds[static_cast<int64>(i)] = caseId;

          // If the cube has no triangles through it
          if(caseId == 0 || caseId == 255)
          {
            continue;
          }

          curTriCounter += util::numTris[caseId];

          const bool* isCut = util::isCut[caseId]; // size 12

          ge0.xstart += isCut[0];
          ge0.ystart += isCut[3];
          ge0.zstart += isCut[8];

          // Note: Each 'gridCell' contains four m_GridEdges running along it,
          //       ge0, ge1, ge2 and ge3. Each gridCell can access its own
          //       ge0 but ge1, ge2 and ge3 are owned by other gridCells.
          //       Accessing ge1, ge2 and ge3 leads to a race condition
          //       unless gridCell is along the boundary of the image.
          //
          //       To really make sense of the indices, it helps to draw
          //       out the following picture of a cube with the appropriate
          //       labels:
          //         v0 is at (i,   j,   k)
          //         v1       (i+1, j,   k)
          //         v2       (i+1, j+1, k)
          //         v3       (i,   j+1, k)
          //         v4       (i,   j,   k+1)
          //         v5       (i+1, j,   k+1)
          //         v6       (i+1, j+1, k+1)
          //         v7       (i,   j+1, k+1)
          //         e0  connects v0 to v1 and is parallel to the x-axis
          //         e1           v1    v2                        y
          //         e2           v2    v3                        x
          //         e3           v0    v3                        y
          //         e4           v4    v5                        x
          //         e5           v5    v6                        y
          //         e6           v6    v7                        x
          //         e7           v4    v7                        y
          //         e8           v0    v4                        z
          //         e9           v1    v5                        z
          //         e10          v3    v7                        z
          //         e11          v2    v6                        z

          // Handle cubes along the edge of the image
          if(isXEnd)
          {
            ge0.ystart += isCut[1];
            ge0.zstart += isCut[9];
          }
          if(isYEnd)
          {
            ge1.xstart += isCut[2];
            ge1.zstart += isCut[10];
          }
          if(isZEnd)
          {
            ge2.xstart += isCut[4];
            ge2.ystart += isCut[7];
          }

          if(isXEnd and isYEnd)
          {
            ge1.zstart += isCut[11];
          }
          if(isXEnd and isZEnd)
          {
            ge2.ystart += isCut[5];
          }
          if(isYEnd and isZEnd)
          {
            ge3.xstart += isCut[6];
          }
        }
      }
    }
  }
  ///////////////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////////////
  // Pass 3 of the algorithm
  ///////////////////////////////////////////////////////////////////////////////
  void pass3()
  {
    // Accumulate triangles into triCounter
    usize tmp;
    usize triAccum = 0;
    for(usize k = 0; k != m_NZ - 1; ++k)
    {
      for(usize j = 0; j != m_NY - 1; ++j)
      {
        usize& curTriCounter = m_TriCounter[k * (m_NY - 1) + j];

        tmp = curTriCounter;
        curTriCounter = triAccum;
        triAccum += tmp;
      }
    }

    // accumulate points, filling out starting locations of each gridEdge
    // in the process.
    usize pointAccum = 0;
    for(usize k = 0; k != m_NZ; ++k)
    {
      for(usize j = 0; j != m_NY; ++j)
      {
        gridEdge& curGridEdge = m_GridEdges[k * m_NY + j];

        tmp = curGridEdge.xstart;
        curGridEdge.xstart = pointAccum;
        pointAccum += tmp;

        tmp = curGridEdge.ystart;
        curGridEdge.ystart = pointAccum;
        pointAccum += tmp;

        tmp = curGridEdge.zstart;
        curGridEdge.zstart = pointAccum;
        pointAccum += tmp;
      }
    }

    /* Saving jic. Same thing as above just scanned in different order
     *
        usize pointAccum = 0;
        for(usize k = 0; k != m_NZ; ++k) {
        for(usize j = 0; j != m_NY; ++j)
        {
            gridEdge& curGridEdge = m_GridEdges[k*m_NY + j];

            tmp = curGridEdge.xstart;
            curGridEdge.xstart = pointAccum;
            pointAccum += tmp;
        }}

        for(usize k = 0; k != m_NZ; ++k) {
        for(usize j = 0; j != m_NY; ++j)
        {
            gridEdge& curGridEdge = m_GridEdges[k*m_NY + j];

            tmp = curGridEdge.ystart;
            curGridEdge.ystart = pointAccum;
            pointAccum += tmp;
        }}

        for(usize k = 0; k != m_NZ; ++k) {
        for(usize j = 0; j != m_NY; ++j)
        {
            gridEdge& curGridEdge = m_GridEdges[k*m_NY + j];

            tmp = curGridEdge.zstart;
            curGridEdge.zstart = pointAccum;
            pointAccum += tmp;
        }}
    */

    pointAccum *= 3; //flatten arrays
    triAccum *= 3;

    m_TriangleGeom.resizeFaceList(triAccum);
    m_TriangleGeom.resizeVertexList(pointAccum);
    m_Normals.getIDataStoreRef().reshapeTuples({pointAccum});
  }
  ///////////////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////////////
  // Pass 4 of the algorithm
  ///////////////////////////////////////////////////////////////////////////////
  void pass4()
  {
    // For each (j, k):
    //  - For each cube at i, fill out points, normals and triangles owned by
    //    the cube. Each cube is in charge of filling out e0, e3 and e8. Only
    //    in edge cases does it also fill out other edges.
    for(usize k = 0; k != m_NZ - 1; ++k)
    {
      for(usize j = 0; j != m_NY - 1; ++j)
      {
        // find adjusted trim values
        usize xl, xr;
        calcTrimValues(xl, xr, j, k); // xl, xr set in this function

        if(xl == xr)
          continue;

        usize triIdx = m_TriCounter[k * (m_NY - 1) + j] * 3;
        auto curCubeCaseIds = m_CubeCases.begin() + (m_NX - 1) * (k * (m_NY - 1) + j);

        gridEdge const& ge0 = m_GridEdges[k * m_NY + j];
        gridEdge const& ge1 = m_GridEdges[k * m_NY + j + 1];
        gridEdge const& ge2 = m_GridEdges[(k + 1) * m_NY + j];
        gridEdge const& ge3 = m_GridEdges[(k + 1) * m_NY + j + 1];

        usize x0counter = 0;
        usize y0counter = 0;
        usize z0counter = 0;

        usize x1counter = 0;
        usize z1counter = 0;

        usize x2counter = 0;
        usize y2counter = 0;

        usize x3counter = 0;

        bool isYEnd = (j == m_NY - 2);
        bool isZEnd = (k == m_NZ - 2);

        for(usize i = xl; i != xr; ++i)
        {
          bool isXEnd = (i == m_NX - 2);

          uint8 caseId = curCubeCaseIds[static_cast<int64>(i)];

          if(caseId == 0 || caseId == 255)
          {
            continue;
          }

          const bool* isCut = util::isCut[caseId]; // has 12 elements

          // Most of the information contained in pointCube, isoValCube
          // and gradCube will be used--but not necessarily all. It has
          // not been tested whether obtaining only the information
          // needed will provide a significant speedup--but
          // most likely not.
          cube pointCube = getPosCube(i, j, k);
          TCube isoValCube = getValCube(i, j, k);
          cube gradCube = getGradCube(i, j, k);

          // Add Points and normals.
          // Calculate global indices for triangles
          std::array<usize, 12> globalIdxs = {};

          if(isCut[0])
          {
            usize idx = ge0.xstart + x0counter;
            idx *= 3;
            InterpolateIntoArrays(pointCube, gradCube, isoValCube, 0, idx);
            globalIdxs[0] = idx;
            ++x0counter;
          }

          if(isCut[3])
          {
            usize idx = ge0.ystart + y0counter;
            idx *= 3;
            InterpolateIntoArrays(pointCube, gradCube, isoValCube, 3, idx);
            globalIdxs[3] = idx;
            ++y0counter;
          }

          if(isCut[8])
          {
            usize idx = ge0.zstart + z0counter;
            idx *= 3;
            InterpolateIntoArrays(pointCube, gradCube, isoValCube, 8, idx);
            globalIdxs[8] = idx;
            ++z0counter;
          }

          // Note:
          //   e1, e5, e9 and e11 will be visited in the next iteration
          //   when they are e3, e7, e8 and 10 respectively. So don't
          //   increment their counters. When the cube is an edge cube,
          //   their counters don't need to be incremented because they
          //   won't be used again.

          // Manage boundary cases if needed, otherwise just update
          // globalIdx.
          if(isCut[1])
          {
            usize idx = ge0.ystart + y0counter;
            idx *= 3;
            if(isXEnd)
            {
              InterpolateIntoArrays(pointCube, gradCube, isoValCube, 1, idx);
              // y0counter counter doesn't need to be incremented
              // because it won't be used again.
            }
            globalIdxs[1] = idx;
          }

          if(isCut[9])
          {
            usize idx = ge0.zstart + z0counter;
            idx *= 3;
            if(isXEnd)
            {
              InterpolateIntoArrays(pointCube, gradCube, isoValCube, 9, idx);
              // z0counter doesn't need to in incremented.
            }
            globalIdxs[9] = idx;
          }

          if(isCut[2])
          {
            usize idx = ge1.xstart + x1counter;
            idx *= 3;
            if(isYEnd)
            {
              InterpolateIntoArrays(pointCube, gradCube, isoValCube, 2, idx);
            }
            globalIdxs[2] = idx;
            ++x1counter;
          }

          if(isCut[10])
          {
            usize idx = ge1.zstart + z1counter;
            idx *= 3;
            if(isYEnd)
            {
              InterpolateIntoArrays(pointCube, gradCube, isoValCube, 10, idx);
            }
            globalIdxs[10] = idx;
            ++z1counter;
          }

          if(isCut[4])
          {
            usize idx = ge2.xstart + x2counter;
            idx *= 3;
            if(isZEnd)
            {
              InterpolateIntoArrays(pointCube, gradCube, isoValCube, 4, idx);
            }
            globalIdxs[4] = idx;
            ++x2counter;
          }

          if(isCut[7])
          {
            usize idx = ge2.ystart + y2counter;
            idx *= 3;
            if(isZEnd)
            {
              InterpolateIntoArrays(pointCube, gradCube, isoValCube, 7, idx);
            }
            globalIdxs[7] = idx;
            ++y2counter;
          }

          if(isCut[11])
          {
            usize idx = ge1.zstart + z1counter;
            idx *= 3;
            if(isXEnd and isYEnd)
            {
              InterpolateIntoArrays(pointCube, gradCube, isoValCube, 11, idx);
              // z1counter does not need to be incremented.
            }
            globalIdxs[11] = idx;
          }

          if(isCut[5])
          {
            usize idx = ge2.ystart + y2counter;
            idx *= 3;
            if(isXEnd and isZEnd)
            {
              InterpolateIntoArrays(pointCube, gradCube, isoValCube, 5, idx);
              // y2 counter does not need to be incremented.
            }
            globalIdxs[5] = idx;
          }

          if(isCut[6])
          {
            usize idx = ge3.xstart + x3counter;
            idx *= 3;
            if(isYEnd and isZEnd)
            {
              InterpolateIntoArrays(pointCube, gradCube, isoValCube, 6, idx);
            }
            globalIdxs[6] = idx;
            ++x3counter;
          }

          // Add triangles
          const char* caseTri = util::caseTriangles[caseId]; // size 16
          for(int idx = 0; caseTri[idx] != -1; idx += 3)
          {
            m_Tris[triIdx] = globalIdxs[caseTri[idx]];
            m_Tris[triIdx + 1] = globalIdxs[caseTri[idx + 1]];
            m_Tris[triIdx + 2] = globalIdxs[caseTri[idx + 2]];
            triIdx += 3;
          }
        }
      }
    }
  }
  ///////////////////////////////////////////////////////////////////////////////

private:
  ///////////////////// MEMBER VARIABLES /////////////////////
  struct gridEdge
  {
    gridEdge()
    : xl(0)
    , xr(0)
    , xstart(0)
    , ystart(0)
    , zstart(0)
    {
    }

    // trim values
    // set on pass 1
    usize xl;
    usize xr;

    // modified on pass 2
    // set on pass 3
    usize xstart;
    usize ystart;
    usize zstart;
  };

  const ImageGeom& m_Image;
  const DataArray<T>& m_DataArray;
  const T m_IsoVal;
  TriangleGeom& m_TriangleGeom;

  usize const m_NX; //
  usize const m_NY; // for indexing
  usize const m_NZ; //

  std::vector<gridEdge> m_GridEdges; // size of m_NY*m_NZ
  std::vector<usize> m_TriCounter;   // size of (m_NY-1)*(m_NZ-1)

  std::vector<uint8> m_EdgeCases; // size (m_NX-1)*m_NY*m_NZ
  std::vector<uint8> m_CubeCases; // size (m_NX-1)*(m_NY-1)*(m_NZ-1)

  IGeometry::SharedVertexList& m_Points;   //
  IGeometry::SharedTriList& m_Tris; //
  Float32Array& m_Normals;  // The output

  /////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////////////
  // Private helper functions
  ///////////////////////////////////////////////////////////////////////////////

  void InterpolateIntoArrays(cube& pointCube, cube& gradCube, TCube& isoValCube, uint8 edgeNum, usize idx)
  {
    auto pointsArray = interpolateOnCube(pointCube, isoValCube, edgeNum);

    m_Points[idx] = pointsArray[0];
    m_Points[idx + 1] = pointsArray[1];
    m_Points[idx + 2] = pointsArray[2];

    auto normalsArray = interpolateOnCube(gradCube, isoValCube, edgeNum);

    m_Normals[idx] = normalsArray[0];
    m_Normals[idx + 1] = normalsArray[1];
    m_Normals[idx + 2] = normalsArray[2];
  }

  bool isCutEdge(usize const& i, usize const& j, usize const& k) const
  {
    // Assuming m_EdgeCases are all set
    usize edgeCaseIdx = k * (m_NX - 1) * m_NY + j * (m_NX - 1) + i;
    if(m_EdgeCases[edgeCaseIdx] == 1 || m_EdgeCases[edgeCaseIdx] == 2)
    {
      return true;
    }

    if(j != m_NY - 1)
    {
      usize edgeCaseIdxY = k * (m_NX - 1) * m_NY + (j + 1) * (m_NX - 1) + i;

      // If (edgeCaseX, edgeCaseY) is (0, 1), (1, 2), (2, 3), (0, 3)
      //                              (1, 0), (2, 1), (3, 2), (3, 0)
      // and not the other options of (0, 2), (1, 3),
      //                              (2, 0), (3, 1)
      // then the edge along the y-axis is cut.
      // So check to see if edgeCaseX + edgeCaseY is odd.
      if((m_EdgeCases[edgeCaseIdx] + m_EdgeCases[edgeCaseIdxY]) % 2 == 1)
      {
        return true;
      }
    }

    if(k != m_NZ - 1)
    {
      usize edgeCaseIdxZ = (k + 1) * (m_NX - 1) * m_NY + j * (m_NX - 1) + i;

      // Same as above. If it is odd, then there is a cut except this
      // time along the z axis.
      if((m_EdgeCases[edgeCaseIdx] + m_EdgeCases[edgeCaseIdxZ]) % 2 == 1)
      {
        return true;
      }
    }

    return false;
  }

  inline uint8 calcCaseEdge(bool const& prevEdge, bool const& currEdge) const
  {
    // o -- is greater than or equal to
    // case 0: (i-1) o-----o (i) | (_,j,k)
    // case 1: (i-1) x-----o (i) | (_,j+1,k)
    // case 2: (i-1) o-----x (i) | (_,j,k+1)
    // case 3: (i-1) x-----x (i) | (_,j+1,k+1)
    if(prevEdge && currEdge)
      return 0;
    if(!prevEdge && currEdge)
      return 1;
    if(prevEdge && !currEdge)
      return 2;
    else // !prevEdge && !currEdge
      return 3;
  }

  inline uint8 calcCubeCase(uint8 const& ec0, uint8 const& ec1, uint8 const& ec2, uint8 const& ec3) const
  {
    // ec0 | (_,j,k)
    // ec1 | (_,j+1,k)
    // ec2 | (_,j,k+1)
    // ec3 | (_,j+1,k+1)

    uint8 caseId = 0;
    if((ec0 == 0) || (ec0 == 2)) // 0 | (i,j,k)
      caseId |= 1;
    if((ec0 == 0) || (ec0 == 1)) // 1 | (i+1,j,k)
      caseId |= 2;
    if((ec1 == 0) || (ec1 == 1)) // 2 | (i+1,j+1,k)
      caseId |= 4;
    if((ec1 == 0) || (ec1 == 2)) // 3 | (i,j+1,k)
      caseId |= 8;
    if((ec2 == 0) || (ec2 == 2)) // 4 | (i,j,k+1)
      caseId |= 16;
    if((ec2 == 0) || (ec2 == 1)) // 5 | (i+1,j,k+1)
      caseId |= 32;
    if((ec3 == 0) || (ec3 == 1)) // 6 | (i+1,j+1,k+1)
      caseId |= 64;
    if((ec3 == 0) || (ec3 == 2)) // 7 | (i,j+1,k+1)
      caseId |= 128;
    return caseId;
  }

  inline void calcTrimValues(usize& xl, usize& xr, usize const& j, usize const& k) const
  {
    gridEdge const& ge0 = m_GridEdges[k * m_NY + j];
    gridEdge const& ge1 = m_GridEdges[k * m_NY + j + 1];
    gridEdge const& ge2 = m_GridEdges[(k + 1) * m_NY + j];
    gridEdge const& ge3 = m_GridEdges[(k + 1) * m_NY + j + 1];

    xl = usize(std::min({ge0.xl, ge1.xl, ge2.xl, ge3.xl}));
    xr = usize(std::max({ge0.xr, ge1.xr, ge2.xr, ge3.xr}));

    if(xl > xr)
      xl = xr;
  }

  inline std::array<T, 3> interpolateOnCube(cube const& pts, TCube const& isoVals, uint8 const& edge) const
  {
    uint8 i0 = util::edgeVertices[edge][0];
    uint8 i1 = util::edgeVertices[edge][1];

    T weight = (m_IsoVal - isoVals[i0]) / (isoVals[i1] - isoVals[i0]);
    return interpolate(pts[i0], pts[i1], weight);
  }

  inline std::array<T, 3> interpolate(std::array<T, 3> const& a, std::array<T, 3> const& b, T const& weight) const
  {
    std::array<T, 3> ret;
    ret[0] = a[0] + (weight * (b[0] - a[0]));
    ret[1] = a[1] + (weight * (b[1] - a[1]));
    ret[2] = a[2] + (weight * (b[2] - a[2]));
    return ret;
  }

  TCube getValCube(usize i, usize j, usize k) const
  {
    TCube vals;

    vals[0] = getData(i, j, k);
    vals[1] = getData(i + 1, j, k);
    vals[2] = getData(i + 1, j + 1, k);
    vals[3] = getData(i, j + 1, k);
    vals[4] = getData(i, j, k + 1);
    vals[5] = getData(i + 1, j, k + 1);
    vals[6] = getData(i + 1, j + 1, k + 1);
    vals[7] = getData(i, j + 1, k + 1);

    return vals;
  }

  cube getPosCube(usize i, usize j, usize k) const
  {
    cube pos;
    const FloatVec3 zeroPos = m_Image.getOrigin();
    const FloatVec3 spacing = m_Image.getSpacing();

    T xPos = zeroPos[0] + i * spacing[0];
    T yPos = zeroPos[1] + j * spacing[1];
    T zPos = zeroPos[2] + k * spacing[2];

    pos[0][0] = xPos;
    pos[0][1] = yPos;
    pos[0][2] = zPos;

    pos[1][0] = xPos + spacing[0];
    pos[1][1] = yPos;
    pos[1][2] = zPos;

    pos[2][0] = xPos + spacing[0];
    pos[2][1] = yPos + spacing[1];
    pos[2][2] = zPos;

    pos[3][0] = xPos;
    pos[3][1] = yPos + spacing[1];
    pos[3][2] = zPos;

    pos[4][0] = xPos;
    pos[4][1] = yPos;
    pos[4][2] = zPos + spacing[2];

    pos[5][0] = xPos + spacing[0];
    pos[5][1] = yPos;
    pos[5][2] = zPos + spacing[2];

    pos[6][0] = xPos + spacing[0];
    pos[6][1] = yPos + spacing[1];
    pos[6][2] = zPos + spacing[2];

    pos[7][0] = xPos;
    pos[7][1] = yPos + spacing[1];
    pos[7][2] = zPos + spacing[2];

    return pos;
  }

  cube getGradCube(usize i, usize j, usize k) const
  {
    cube grad;

    grad[0] = computeGradient(i, j, k);
    grad[1] = computeGradient(i + 1, j, k);
    grad[2] = computeGradient(i + 1, j + 1, k);
    grad[3] = computeGradient(i, j + 1, k);
    grad[4] = computeGradient(i, j, k + 1);
    grad[5] = computeGradient(i + 1, j, k + 1);
    grad[6] = computeGradient(i + 1, j + 1, k + 1);
    grad[7] = computeGradient(i, j + 1, k + 1);

    return grad;
  }

  inline T getData(usize i, usize j, usize k) const
  {
    auto optional = m_Image.getIndex(static_cast<float64>(i), static_cast<float64>(j), static_cast<float64>(k));
    usize index = optional.has_value() ? optional.value() : std::numeric_limits<usize>::max();
    return m_DataArray[index];
  }

  std::array<T, 3> computeGradient(usize i, usize j, usize k) const
  {
    std::array<std::array<T, 2>, 3> x;
    std::array<T, 3> run;
    FloatVec3 spacing = m_Image.getSpacing();

    auto optional = m_Image.getIndex(static_cast<float64>(i), static_cast<float64>(j), static_cast<float64>(k));
    usize dataIdx = optional.has_value() ? optional.value() : std::numeric_limits<usize>::max();

    if(i == 0)
    {
      x[0][0] = m_DataArray[dataIdx + 1];
      x[0][1] = m_DataArray[dataIdx];
      run[0] = spacing[0];
    }
    else if(i == (m_NX - 1))
    {
      x[0][0] = m_DataArray[dataIdx];
      x[0][1] = m_DataArray[dataIdx - 1];
      run[0] = spacing[0];
    }
    else
    {
      x[0][0] = m_DataArray[dataIdx + 1];
      x[0][1] = m_DataArray[dataIdx - 1];
      run[0] = 2 * spacing[0];
    }

    if(j == 0)
    {
      x[1][0] = m_DataArray[dataIdx + m_NX];
      x[1][1] = m_DataArray[dataIdx];
      run[1] = spacing[1];
    }
    else if(j == (m_NY - 1))
    {
      x[1][0] = m_DataArray[dataIdx];
      x[1][1] = m_DataArray[dataIdx - m_NX];
      run[1] = spacing[1];
    }
    else
    {
      x[1][0] = m_DataArray[dataIdx + m_NX];
      x[1][1] = m_DataArray[dataIdx - m_NX];
      run[1] = 2 * spacing[1];
    }

    if(k == 0)
    {
      x[2][0] = m_DataArray[dataIdx + m_NX * m_NY];
      x[2][1] = m_DataArray[dataIdx];
      run[2] = spacing[2];
    }
    else if(k == (m_NZ - 1))
    {
      x[2][0] = m_DataArray[dataIdx];
      x[2][1] = m_DataArray[dataIdx - m_NX * m_NY];
      run[2] = spacing[2];
    }
    else
    {
      x[2][0] = m_DataArray[dataIdx + m_NX * m_NY];
      x[2][1] = m_DataArray[dataIdx - m_NX * m_NY];
      run[2] = 2 * spacing[2];
    }

    std::array<T, 3> ret;

    ret[0] = (x[0][1] - x[0][0]) / run[0];
    ret[1] = (x[1][1] - x[1][0]) / run[1];
    ret[2] = (x[2][1] - x[2][0]) / run[2];

    return ret;
  }

  ///////////////////////////////////////////////////////////////////////////////
};
} // namespace complex