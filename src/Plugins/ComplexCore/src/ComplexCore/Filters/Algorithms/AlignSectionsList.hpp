#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/AlignSections.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT AlignSectionsListInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  bool DREAM3DAlignmentFile;
  DataPath ImageGeometryPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT AlignSectionsList : public AlignSections
{
public:
  AlignSectionsList(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignSectionsListInputValues* inputValues);
  ~AlignSectionsList() noexcept;

  AlignSectionsList(const AlignSectionsList&) = delete;
  AlignSectionsList(AlignSectionsList&&) noexcept = delete;
  AlignSectionsList& operator=(const AlignSectionsList&) = delete;
  AlignSectionsList& operator=(AlignSectionsList&&) noexcept = delete;

  Result<> operator()();

protected:
  void find_shifts(std::vector<int64>& xshifts, std::vector<int64>& yshifts) override;

  std::vector<DataPath> getSelectedDataPaths() const override;

private:
  DataStructure& m_DataStructure;
  const AlignSectionsListInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  Result<> m_Result;
};

} // namespace complex
