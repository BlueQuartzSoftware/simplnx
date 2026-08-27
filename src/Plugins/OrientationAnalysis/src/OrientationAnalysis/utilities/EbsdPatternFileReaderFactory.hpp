#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/IEbsdPatternFileReader.hpp"

#include <filesystem>
#include <memory>

namespace nx::core
{
inline constexpr int32 k_UnsupportedEbsdPatternExtensionError = -78020;

ORIENTATIONANALYSIS_EXPORT Result<std::unique_ptr<IEbsdPatternFileReader>> CreateEbsdPatternFileReader(const std::filesystem::path& filePath);
} // namespace nx::core
