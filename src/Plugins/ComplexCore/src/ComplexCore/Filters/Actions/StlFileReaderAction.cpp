#include "StlFileReaderAction.hpp"


#include <fmt/core.h>

#include "ComplexCore/utils/StlUtilities.hpp"

using namespace complex;

namespace
{
constexpr complex::int32 k_InsertFailureError = -2;
} // namespace

namespace complex
{
StlFileReaderAction::StlFileReaderAction(const fs::path& stlFilePath)
: m_Path(stlFilePath)
{
}

StlFileReaderAction::~StlFileReaderAction() noexcept = default;

Result<> StlFileReaderAction::apply(DataStructure& dataStructure, Mode mode) const
{
  int32_t fileType = StlUtilities::DetermineStlFileType(m_Path);
  if(fileType == 1)
  {
    return MakeErrorResult(StlConstants::k_UnsupportedFileType, fmt::format("The Input STL File is ASCII which is not currently supported. Please convert it to a binary STL file using another program.", m_Path.string()));
  }
  if(fileType < 0)
  {
    return MakeErrorResult(StlConstants::k_ErrorOpeningFile, fmt::format("Error reading the STL file.", m_Path.string()));
  }

  return {};
}



fs::path StlFileReaderAction::path() const
{
  return m_Path;
}
} // namespace complex
