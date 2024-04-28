#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT AlignSectionsListInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  bool DREAM3DAlignmentFile;
  DataPath ImageGeometryPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT AlignSectionsList : public AlignSections
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
  Result<> findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts) override;

  std::vector<DataPath> getSelectedDataPaths() const override;

private:
  DataStructure& m_DataStructure;
  const AlignSectionsListInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  Result<> m_Result;
};

} // namespace nx::core
