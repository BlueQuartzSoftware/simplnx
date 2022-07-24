#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/AlignSections.hpp"

namespace complex
{

struct CORE_EXPORT AlignSectionsFeatureCentroidInputValues
{
  bool WriteAlignmentShifts;
  FileSystemPathParameter::ValueType AlignmentShiftFileName;
  bool UseReferenceSlice;
  int32 ReferenceSlice;
  DataPath GoodVoxelsArrayPath;
  DataPath inputImageGeometry;
  DataPath cellDataGroupPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT AlignSectionsFeatureCentroid : public AlignSections
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
  void find_shifts(std::vector<int64_t>& xshifts, std::vector<int64_t>& yshifts) override;

  std::vector<DataPath> getSelectedDataPaths() const override;

private:
  DataStructure& m_DataStructure;
  const AlignSectionsFeatureCentroidInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  Result<> m_Result;
};

} // namespace complex
