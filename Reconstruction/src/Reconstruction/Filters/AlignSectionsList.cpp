#include "AlignSectionsList.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string AlignSectionsList::name() const
{
  return FilterTraits<AlignSectionsList>::name.str();
}

std::string AlignSectionsList::className() const
{
  return FilterTraits<AlignSectionsList>::className;
}

Uuid AlignSectionsList::uuid() const
{
  return FilterTraits<AlignSectionsList>::uuid;
}

std::string AlignSectionsList::humanName() const
{
  return "Align Sections (List)";
}

Parameters AlignSectionsList::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<BoolParameter>(k_DREAM3DAlignmentFile_Key, "DREAM3D Alignment File Format", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix", "", DataPath{}));

  return params;
}

IFilter::UniquePointer AlignSectionsList::clone() const
{
  return std::make_unique<AlignSectionsList>();
}

Result<OutputActions> AlignSectionsList::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDREAM3DAlignmentFileValue = filterArgs.value<bool>(k_DREAM3DAlignmentFile_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AlignSectionsListAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> AlignSectionsList::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDREAM3DAlignmentFileValue = filterArgs.value<bool>(k_DREAM3DAlignmentFile_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
