#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/AlignSections.hpp"

#include <vector>

namespace complex
{

/**
 * @brief
 */
struct ORIENTATIONANALYSIS_EXPORT AlignSectionsMisorientationInputValues
{
  bool writeAlignmentShifts;
  FileSystemPathParameter::ValueType alignmentShiftFileName;
  float32 misorientationTolerance;
  bool useGoodVoxels;
  DataPath inputImageGeometry;
  DataPath cellDataGroupPath;
  DataPath quatsArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath goodVoxelsArrayPath;
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
  Result<> m_Result;
};
} // namespace complex
