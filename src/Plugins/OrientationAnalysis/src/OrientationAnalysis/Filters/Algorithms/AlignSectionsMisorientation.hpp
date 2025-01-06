#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

#include <vector>

namespace nx::core
{

/**
 * @brief
 */
struct ORIENTATIONANALYSIS_EXPORT AlignSectionsMisorientationInputValues
{
  DataPath ImageGeometryPath;
  bool UseMask;
  DataPath MaskArrayPath;
  bool writeAlignmentShifts;
  FileSystemPathParameter::ValueType AlignmentShiftFileName;

  float32 misorientationTolerance;
  DataPath cellDataGroupPath;
  DataPath quatsArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath crystalStructuresArrayPath;
};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT AlignSectionsMisorientation : public AlignSections
{
public:
  AlignSectionsMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignSectionsMisorientationInputValues* inputValues);
  ~AlignSectionsMisorientation() noexcept override;

  AlignSectionsMisorientation(const AlignSectionsMisorientation&) = delete;
  AlignSectionsMisorientation(AlignSectionsMisorientation&&) noexcept = delete;
  AlignSectionsMisorientation& operator=(const AlignSectionsMisorientation&) = delete;
  AlignSectionsMisorientation& operator=(AlignSectionsMisorientation&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> findShifts(std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) override;

  std::vector<DataPath> getSelectedDataPaths() const override;

private:
  DataStructure& m_DataStructure;
  const AlignSectionsMisorientationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
