/* ============================================================================
 * Copyright (c) 2020 BlueQuartz Software, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the names of any of the BlueQuartz Software contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once

#include "complex/Common/Result.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace complex::FileUtilities
{
//-----------------------------------------------------------------------------
bool COMPLEX_EXPORT HasWriteAccess(const std::string& path);

//-----------------------------------------------------------------------------
Result<> COMPLEX_EXPORT ValidateInputFile(const fs::path& path);

//-----------------------------------------------------------------------------
Result<> COMPLEX_EXPORT ValidateInputDir(const fs::path& path);

//-----------------------------------------------------------------------------
Result<> COMPLEX_EXPORT ValidateDirectoryWritePermission(const fs::path& path, bool isFile);

//-----------------------------------------------------------------------------
Result<> COMPLEX_EXPORT ValidateOutputFile(const fs::path& path);

//-----------------------------------------------------------------------------
Result<> COMPLEX_EXPORT ValidateOutputDir(const fs::path& path);

//-----------------------------------------------------------------------------
Result<> COMPLEX_EXPORT ValidateCSVFile(const std::string& filePath);
} // namespace complex::FileUtilities
