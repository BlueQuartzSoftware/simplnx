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

Result<> ReadEbsdPatternFile::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }

  auto readerResult = CreateEbsdPatternFileReader(m_InputValues.input_file);
  if(readerResult.invalid())
  {
    return ConvertResult(std::move(readerResult));
  }

  auto fileInfoResult = readerResult.value()->readFileInfo();
  if(fileInfoResult.invalid())
  {
    return ConvertResult(std::move(fileInfoResult));
  }

  auto& outputArrayRef = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues.output_array_path);
  if(m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues.output_array_path.getParent()) == nullptr)
  {
    ShapeType expectedTupleShape;
    if(fileInfoResult.value().header_version == 1 && m_InputValues.set_scan_dimensions)
    {
      expectedTupleShape = {static_cast<usize>(m_InputValues.number_of_rows), static_cast<usize>(m_InputValues.number_of_columns)};
    }
    else if(fileInfoResult.value().header_version == 1)
    {
      expectedTupleShape = {static_cast<usize>(fileInfoResult.value().number_of_patterns)};
    }
    else
    {
      expectedTupleShape = {static_cast<usize>(fileInfoResult.value().number_of_rows.value()), static_cast<usize>(fileInfoResult.value().number_of_columns.value())};
    }

    if(outputArrayRef.getTupleShape() != expectedTupleShape)
    {
      return MakeErrorResult(
          -78045, fmt::format("Output DataArray '{}' has tuple shape [{}], but the current metadata in EBSD pattern file '{}' requires tuple shape [{}]. The file may have changed since "
                              "preflight.",
                              m_InputValues.output_array_path.toString(), fmt::join(outputArrayRef.getTupleShape(), ", "), m_InputValues.input_file.string(), fmt::join(expectedTupleShape, ", ")));
    }
  }
  Result<> readResult = readerResult.value()->readPatternData(fileInfoResult.value(), outputArrayRef, m_ShouldCancel, m_MessageHandler);
  readResult.warnings().insert(readResult.warnings().begin(), fileInfoResult.warnings().begin(), fileInfoResult.warnings().end());
  return readResult;
}
} // namespace nx::core
