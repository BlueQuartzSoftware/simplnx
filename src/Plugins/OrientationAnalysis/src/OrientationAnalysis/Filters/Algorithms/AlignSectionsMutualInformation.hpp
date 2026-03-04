#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

namespace nx::core
{
struct ORIENTATIONANALYSIS_EXPORT AlignSectionsMutualInformationInputValues
{
  DataPath ImageGeometryPath;
  bool UseMask;
  DataPath MaskArrayPath;

  float32 MisorientationTolerance;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;

  bool StoreAlignmentShifts;
  DataPath AlignmentAMPath;
  DataPath SlicesArrayPath;
  DataPath RelativeShiftsArrayPath;
  DataPath CumulativeShiftsArrayPath;
};

/**
 * @class
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

  void formFeaturesSections(std::vector<int32>& miFeatureIds, std::vector<int32>& featureCounts);

private:
  DataStructure& m_DataStructure;
  const AlignSectionsMutualInformationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::unique_ptr<MaskCompareUtilities::IMaskCompare> m_MaskCompare = nullptr;
};

} // namespace nx::core
