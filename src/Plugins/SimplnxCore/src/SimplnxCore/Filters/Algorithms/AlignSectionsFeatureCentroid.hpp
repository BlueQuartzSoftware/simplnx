#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT AlignSectionsFeatureCentroidInputValues
{
  DataPath ImageGeometryPath;
  DataPath MaskArrayPath;

  bool UseReferenceSlice;
  int32 ReferenceSlice;
  //  DataPath cellDataGroupPath;

  bool StoreAlignmentShifts;
  DataPath AlignmentAMPath;
  DataPath SlicesArrayPath;
  DataPath RelativeShiftsArrayPath;
  DataPath CumulativeShiftsArrayPath;
  DataPath CentroidsArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT AlignSectionsFeatureCentroid : public AlignSections
{
public:
  AlignSectionsFeatureCentroid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignSectionsFeatureCentroidInputValues* inputValues);
  ~AlignSectionsFeatureCentroid() noexcept override;

  AlignSectionsFeatureCentroid(const AlignSectionsFeatureCentroid&) = delete;
  AlignSectionsFeatureCentroid(AlignSectionsFeatureCentroid&&) noexcept = delete;
  AlignSectionsFeatureCentroid& operator=(const AlignSectionsFeatureCentroid&) = delete;
  AlignSectionsFeatureCentroid& operator=(AlignSectionsFeatureCentroid&&) noexcept = delete;

  Result<> operator()();

protected:
  /**
   * @brief This method finds the slice to slice shifts and should be implemented by subclasses
   * @param xShifts
   * @param yShifts
   * @return Whether the x and y shifts were successfully found
   */
  Result<> findShifts(std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) override;

private:
  DataStructure& m_DataStructure;
  const AlignSectionsFeatureCentroidInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
