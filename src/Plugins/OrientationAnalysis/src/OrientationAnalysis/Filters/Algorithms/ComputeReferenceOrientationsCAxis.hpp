#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeReferenceOrientationsCAxisInputValues
{
  std::vector<float32> ReferenceDir;
  DataPath AvgCAxesArrayPath;
  DataPath MisorientationArrayPath;
};

/**
 * @class ComputeReferenceOrientationsCAxis
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT ComputeReferenceOrientationsCAxis
{
public:
  ComputeReferenceOrientationsCAxis(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                    ComputeReferenceOrientationsCAxisInputValues* inputValues);
  ~ComputeReferenceOrientationsCAxis() noexcept;

  ComputeReferenceOrientationsCAxis(const ComputeReferenceOrientationsCAxis&) = delete;
  ComputeReferenceOrientationsCAxis(ComputeReferenceOrientationsCAxis&&) noexcept = delete;
  ComputeReferenceOrientationsCAxis& operator=(const ComputeReferenceOrientationsCAxis&) = delete;
  ComputeReferenceOrientationsCAxis& operator=(ComputeReferenceOrientationsCAxis&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeReferenceOrientationsCAxisInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
