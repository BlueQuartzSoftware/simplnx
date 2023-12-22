#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT AlignSectionsMutualInformationInputValues
{
  bool WriteAlignmentShifts;
  FileSystemPathParameter::ValueType AlignmentShiftFileName;
  float32 MisorientationTolerance;
  bool UseMask;
  DataPath ImageGeometryPath;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath MaskArrayPath;
  DataPath CrystalStructuresArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT AlignSectionsMutualInformation : public AlignSections
{
public:
  AlignSectionsMutualInformation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 AlignSectionsMutualInformationInputValues* inputValues);
  ~AlignSectionsMutualInformation() noexcept override;

  AlignSectionsMutualInformation(const AlignSectionsMutualInformation&) = delete;
  AlignSectionsMutualInformation(AlignSectionsMutualInformation&&) noexcept = delete;
  AlignSectionsMutualInformation& operator=(const AlignSectionsMutualInformation&) = delete;
  AlignSectionsMutualInformation& operator=(AlignSectionsMutualInformation&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts) override;

  std::vector<DataPath> getSelectedDataPaths() const override;

  void formFeaturesSections(std::vector<int32>& miFeatureIds, std::vector<int32>& featureCounts);

private:
  DataStructure& m_DataStructure;
  const AlignSectionsMutualInformationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  Result<> m_Result = {};
};

} // namespace nx::core
