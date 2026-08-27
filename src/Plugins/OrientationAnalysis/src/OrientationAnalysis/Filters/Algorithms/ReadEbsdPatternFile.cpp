#include "OrientationAnalysis/Filters/Algorithms/ReadEbsdPatternFile.hpp"

#include "OrientationAnalysis/utilities/EbsdPatternFileReaderFactory.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace nx::core
{
ReadEbsdPatternFile::ReadEbsdPatternFile(DataStructure& dataStructure, const ReadEbsdPatternFileInputValues& inputValues, const std::atomic_bool& shouldCancel,
                                         const IFilter::MessageHandler& messageHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

ReadEbsdPatternFile::~ReadEbsdPatternFile() noexcept = default;

Result<> ReadEbsdPatternFile::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }

  auto readerResult = CreateEbsdPatternFileReader(m_InputValues.inputFile);
  if(readerResult.invalid())
  {
    return ConvertResult(std::move(readerResult));
  }

  auto fileInfoResult = readerResult.value()->readFileInfo();
  if(fileInfoResult.invalid())
  {
    return ConvertResult(std::move(fileInfoResult));
  }

  auto& outputArrayRef = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues.outputArrayPath);
  if(m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues.outputArrayPath.getParent()) == nullptr)
  {
    ShapeType expectedTupleShape;
    if(fileInfoResult.value().headerVersion == 1 && m_InputValues.setScanDimensions)
    {
      expectedTupleShape = {static_cast<usize>(m_InputValues.numberOfRows), static_cast<usize>(m_InputValues.numberOfColumns)};
    }
    else if(fileInfoResult.value().headerVersion == 1)
    {
      expectedTupleShape = {static_cast<usize>(fileInfoResult.value().numberOfPatterns)};
    }
    else
    {
      expectedTupleShape = {static_cast<usize>(fileInfoResult.value().numberOfRows.value()), static_cast<usize>(fileInfoResult.value().numberOfColumns.value())};
    }

    if(outputArrayRef.getTupleShape() != expectedTupleShape)
    {
      return MakeErrorResult(
          -78045, fmt::format("Output DataArray '{}' has tuple shape [{}], but the current metadata in EBSD pattern file '{}' requires tuple shape [{}]. The file may have changed since "
                              "preflight.",
                              m_InputValues.outputArrayPath.toString(), fmt::join(outputArrayRef.getTupleShape(), ", "), m_InputValues.inputFile.string(), fmt::join(expectedTupleShape, ", ")));
    }
  }
  Result<> readResult = readerResult.value()->readPatternData(fileInfoResult.value(), outputArrayRef, m_ShouldCancel, m_MessageHandler);
  readResult.warnings().insert(readResult.warnings().begin(), fileInfoResult.warnings().begin(), fileInfoResult.warnings().end());
  return readResult;
}
} // namespace nx::core
