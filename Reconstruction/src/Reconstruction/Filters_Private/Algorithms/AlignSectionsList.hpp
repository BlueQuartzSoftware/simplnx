#pragma once

#include "Reconstruction/Reconstruction_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AlignSectionsListInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.DREAM3DAlignmentFile = filterArgs.value<bool>(k_DREAM3DAlignmentFile_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  return AlignSectionsList(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct RECONSTRUCTION_EXPORT AlignSectionsListInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  bool DREAM3DAlignmentFile;
  DataPath CellAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class RECONSTRUCTION_EXPORT AlignSectionsList
{
public:
  AlignSectionsList(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignSectionsListInputValues* inputValues);
  ~AlignSectionsList() noexcept;

  AlignSectionsList(const AlignSectionsList&) = delete;
  AlignSectionsList(AlignSectionsList&&) noexcept = delete;
  AlignSectionsList& operator=(const AlignSectionsList&) = delete;
  AlignSectionsList& operator=(AlignSectionsList&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AlignSectionsListInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
