#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Common/Types.hpp"

namespace nx::core::UnitTest
{
/**
 * @namespace nx::core::UnitTest
 * @brief Contains OrientationAnalysis algorithm-dispatch test support.
 */

/**
 * @enum AlgorithmDispatchPath
 * @brief Identifies the path selected by a dispatch probe.
 */
enum class AlgorithmDispatchPath : uint8
{
  Unknown, ///< Identifies a failed probe dispatch.
  Direct,  ///< Identifies the direct execution path.
  Scanline ///< Identifies the scanline execution path.
};

/**
 * @brief Gets the path selected inside the OrientationAnalysis plugin.
 * @return The selected direct or scanline path, or Unknown when the probe fails.
 *
 * The empty target list isolates force-flag dispatch selection from storage
 * residency. The function crosses the plugin boundary for the test witness.
 */
ORIENTATIONANALYSIS_EXPORT AlgorithmDispatchPath GetAlgorithmDispatchPathFromOrientationAnalysisPlugin();
} // namespace nx::core::UnitTest
