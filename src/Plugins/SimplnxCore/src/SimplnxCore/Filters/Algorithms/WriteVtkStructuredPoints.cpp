#include "WriteVtkStructuredPoints.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/utils/VtkUtilities.hpp"

using namespace nx::core;

namespace
{

} // namespace

// -----------------------------------------------------------------------------
WriteVtkStructuredPoints::WriteVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   WriteVtkStructuredPointsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteVtkStructuredPoints::~WriteVtkStructuredPoints() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteVtkStructuredPoints::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteVtkStructuredPoints::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 spacing = imageGeom.getSpacing();
  FloatVec3 origin = imageGeom.getOrigin();

  std::string vtkOutPath = m_InputValues->OutputFile;
  std::ofstream outStrm(vtkOutPath, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
  if(!outStrm.is_open())
  {
    return MakeErrorResult(-66667, fmt::format("Output file could not be opened for writing: '{}'", m_InputValues->OutputFile.string()));
  }
  outStrm << "# vtk DataFile Version 3.0\n";
  outStrm << "vtk output\n";

  if(m_InputValues->WriteBinaryFile)
  {
    outStrm << "BINARY\n";
  }
  else
  {
    outStrm << "ASCII\n";
  }
  outStrm << "DATASET STRUCTURED_POINTS\n";

  outStrm << fmt::format("DIMENSIONS {} {} {}\n", dims[0] + 1, dims[1] + 1, dims[2] + 1);
  outStrm << fmt::format("SPACING {} {} {}\n", spacing[0], spacing[1], spacing[2]);
  outStrm << fmt::format("ORIGIN {} {} {}\n", origin[0], origin[1], origin[2]);

  outStrm << fmt::format("CELL_DATA {}\n", dims[0] * dims[1] * dims[2]);
  Result<> result;

  for(const auto& arrayPath : m_InputValues->SelectedDataArrayPaths)
  {
    m_MessageHandler({nx::core::IFilter::Message::Type::Info, fmt::format("Writing {}", arrayPath.toString())});
    IDataArray& dataArray = m_DataStructure.getDataRefAs<IDataArray>(arrayPath);
    auto dataType = dataArray.getDataType();

    switch(dataType)
    {
    case DataType::int8:
      MergeResults(result, writeVtkData<int8>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    case DataType::uint8:
      MergeResults(result, writeVtkData<uint8>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    case DataType::int16:
      MergeResults(result, writeVtkData<int16>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    case DataType::uint16:
      MergeResults(result, writeVtkData<uint16>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    case DataType::int32:
      MergeResults(result, writeVtkData<int32>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    case DataType::uint32:
      MergeResults(result, writeVtkData<uint32>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    case DataType::int64:
      MergeResults(result, writeVtkData<int64>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    case DataType::uint64:
      MergeResults(result, writeVtkData<uint64>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    case DataType::float32:
      MergeResults(result, writeVtkData<float32>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    case DataType::float64:
      MergeResults(result, writeVtkData<float64>(outStrm, m_DataStructure, arrayPath, m_InputValues->WriteBinaryFile, m_MessageHandler, m_ShouldCancel));
      break;
    default:
      MergeResults(result, MakeErrorResult(-666666, "The chosen scalar type is not supported by this filter."));
    }
  }

  return result;
}
