#include "WriteVtkRectilinearGrid.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <H5Tpublic.h>

using namespace complex;

namespace
{
static constexpr usize k_BufferDumpVal = 1000000;

// -----------------------------------------------------------------------------
template <typename T>
std::string TypeForPrimitive(T value, const IFilter::MessageHandler& messageHandler)
{
  if constexpr(std::is_same_v<T, float32>)
  {
    return "float";
  }
  if constexpr(std::is_same_v<T, float64>)
  {
    return "double";
  }

  if constexpr(std::is_same_v<T, int8>)
  {
    return "char";
  }
  if constexpr(std::is_same_v<T, uint8>)
  {
    return "unsigned_char";
  }
  if constexpr(std::is_same_v<T, char>)
  {
    return "char";
  }
  if constexpr(std::is_same_v<T, signed char>)
  {
    return "char";
  }
  if constexpr(std::is_same_v<T, unsigned char>)
  {
    return "char";
  }

  if constexpr(std::is_same_v<T, int16>)
  {
    return "short";
  }
  if constexpr(std::is_same_v<T, short>)
  {
    return "short";
  }
  if constexpr(std::is_same_v<T, signed short>)
  {
    return "short";
  }
  if constexpr(std::is_same_v<T, uint16>)
  {
    return "unsigned_short";
  }
  if constexpr(std::is_same_v<T, unsigned short>)
  {
    return "unsigned_short";
  }

  if constexpr(std::is_same_v<T, int32>)
  {
    return "int";
  }
  if constexpr(std::is_same_v<T, uint32>)
  {
    return "unsigned_int";
  }
  if constexpr(std::is_same_v<T, int>)
  {
    return "int";
  }
  if constexpr(std::is_same_v<T, signed int>)
  {
    return "int";
  }
  if constexpr(std::is_same_v<T, unsigned int>)
  {
    return "unsigned_int";
  }

  if constexpr(std::is_same_v<T, long int>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, signed long int>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, unsigned long int>)
  {
    return "unsigned_long";
  }

  if constexpr(std::is_same_v<T, long long int>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, signed long long int>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, unsigned long long int>)
  {
    return "unsigned_long";
  }
  if constexpr(std::is_same_v<T, int64>)
  {
    return "long";
  }
  if constexpr(std::is_same_v<T, uint64>)
  {
    return "unsigned_long";
  }

  if constexpr(std::is_same_v<T, bool>)
  {
    return "char";
  }

  messageHandler(IFilter::Message::Type::Info, fmt::format("Error: TypeForPrimitive - Unknown Type: ", typeid(value).name()));
  if(const char* name = typeid(value).name(); nullptr != name && name[0] == 'l')
  {
    messageHandler(
        IFilter::Message::Type::Info,
        fmt::format(
            "You are using 'long int' as a type which is not 32/64 bit safe. It is suggested you use one of the H5SupportTypes defined in <Common/H5SupportTypes.h> such as int32_t or uint32_t.",
            typeid(value).name()));
  }
  return "";
}

// -----------------------------------------------------------------------------
struct WriteVtkDataArrayFunctor
{
  template <typename T>
  void operator()(FILE* outputFile, bool binary, DataStructure& dataStructure, const DataPath& arrayPath, const IFilter::MessageHandler& messageHandler)
  {
    auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(arrayPath);

    messageHandler(IFilter::Message::Type::Info, fmt::format("Writing Cell Data {}", dataArray.getName()));

    const usize totalElements = dataArray.getSize();
    const int numComps = static_cast<int>(dataArray.getNumberOfComponents());
    std::string dName = dataArray.getName();
    dName = StringUtilities::replace(dName, " ", "_");

    const std::string vtkTypeString = TypeForPrimitive<T>(dataArray[0], messageHandler);
    bool useIntCast = false;
    if(vtkTypeString == "unsigned_char" || vtkTypeString == "char")
    {
      useIntCast = true;
    }

    fprintf(outputFile, "SCALARS %s %s %d\n", dName.c_str(), vtkTypeString.c_str(), numComps);
    fprintf(outputFile, "LOOKUP_TABLE default\n");
    if(binary)
    {
      if constexpr(endian::little == endian::native)
      {
        dataArray.byteSwapElements();
      }
      fwrite(dataArray.template getIDataStoreAs<DataStore<T>>()->data(), sizeof(T), totalElements, outputFile);
      fprintf(outputFile, "\n");
      if constexpr(endian::little == endian::native)
      {
        dataArray.byteSwapElements();
      }
    }
    else
    {
      std::string buffer;
      buffer.reserve(k_BufferDumpVal);
      for(size_t i = 0; i < totalElements; i++)
      {
        if(i % 20 == 0 && i > 0)
        {
          buffer.append("\n");
        }
        if(useIntCast)
        {
          buffer.append(fmt::format(" {:d}", static_cast<int>(dataArray[i])));
        }
        else if constexpr(std::is_same_v<T, float32>)
        {
          buffer.append(fmt::format(" {:f}", dataArray[i]));
        }
        else if constexpr(std::is_same_v<T, float64>)
        {
          buffer.append(fmt::format(" {:f}", dataArray[i]));
        }
        else
        {
          buffer.append(fmt::format(" {}", dataArray[i]));
        }
        // If the buffer is within 32 bytes of the reserved size, then dump
        // the contents to the file.
        if(buffer.size() > (k_BufferDumpVal - 32))
        {
          fprintf(outputFile, "%s", buffer.c_str());
          buffer.clear();
          buffer.reserve(k_BufferDumpVal);
        }
      }
      buffer.append("\n");
      fprintf(outputFile, "%s", buffer.c_str());
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
WriteVtkRectilinearGrid::WriteVtkRectilinearGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 WriteVtkRectilinearGridInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteVtkRectilinearGrid::~WriteVtkRectilinearGrid() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteVtkRectilinearGrid::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteVtkRectilinearGrid::operator()()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 res = imageGeom.getSpacing();
  FloatVec3 origin = imageGeom.getOrigin();

  int err = 0;
  FILE* outputFile = nullptr;
  outputFile = fopen(m_InputValues->OutputFile.string().c_str(), "wb");
  if(nullptr == outputFile)
  {
    return MakeErrorResult(-2073, fmt::format("Error opening output vtk file '{}'", m_InputValues->OutputFile.string()));
  }

  // write the header
  writeVtkHeader(outputFile);

  // Write the Coordinate Points
  Result<> writeCoordsResults = writeCoords<float32>(outputFile, "X_COORDINATES", "float", dims[0] + 1, origin[0] - res[0] * 0.5f, (float)(dims[0] + 1 * res[0]), res[0]);
  if(writeCoordsResults.invalid())
  {
    return MergeResults(writeCoordsResults, MakeErrorResult(-2075, fmt::format("Error writing X Coordinates in vtk file {}'\n ", m_InputValues->OutputFile.string())));
  }
  writeCoordsResults = writeCoords<float32>(outputFile, "Y_COORDINATES", "float", dims[1] + 1, origin[1] - res[1] * 0.5f, (float)(dims[1] + 1 * res[1]), res[1]);
  if(writeCoordsResults.invalid())
  {
    return MergeResults(writeCoordsResults, MakeErrorResult(-2076, fmt::format("Error writing Y Coordinates in vtk file %s'\n ", m_InputValues->OutputFile.string())));
  }
  writeCoordsResults = writeCoords<float32>(outputFile, "Z_COORDINATES", "float", dims[2] + 1, origin[2] - res[2] * 0.5f, (float)(dims[2] + 1 * res[2]), res[2]);
  if(writeCoordsResults.invalid())
  {
    return MergeResults(writeCoordsResults, MakeErrorResult(-2077, fmt::format("Error writing Z Coordinates in vtk file %s'\n ", m_InputValues->OutputFile.string())));
  }

  // Write the data arrays
  const auto totalCells = imageGeom.getNumXCells() * imageGeom.getNumYCells() * imageGeom.getNumZCells();
  fprintf(outputFile, "CELL_DATA %d\n", static_cast<int>(totalCells));

  for(const DataPath& arrayPath : m_InputValues->SelectedDataArrayPaths)
  {
    ExecuteDataFunction(WriteVtkDataArrayFunctor{}, m_DataStructure.getDataAs<IDataArray>(arrayPath)->getDataType(), outputFile, m_InputValues->WriteBinaryFile, m_DataStructure, arrayPath,
                        m_MessageHandler);
  }

  fclose(outputFile);

  return {};
}

// -----------------------------------------------------------------------------
void WriteVtkRectilinearGrid::writeVtkHeader(FILE* outputFile) const
{
  const auto& geom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize xPoints = geom.getNumXCells() + 1;
  const usize yPoints = geom.getNumYCells() + 1;
  const usize zPoints = geom.getNumZCells() + 1;

  fprintf(outputFile, "# vtk DataFile Version 2.0\n");
  fprintf(outputFile, "Data set from DREAM.3D ComplexCore version 7.0.0\n");
  if(m_InputValues->WriteBinaryFile)
  {
    fprintf(outputFile, "BINARY\n");
  }
  else
  {
    fprintf(outputFile, "ASCII\n");
  }
  fprintf(outputFile, "\n");
  fprintf(outputFile, "DATASET RECTILINEAR_GRID\n");
  fprintf(outputFile, "DIMENSIONS %ld %ld %ld\n", static_cast<long int>(xPoints), static_cast<long int>(yPoints), static_cast<long int>(zPoints));
}

// -----------------------------------------------------------------------------
template <typename T>
Result<> WriteVtkRectilinearGrid::writeCoords(FILE* outputFile, const std::string& axis, const std::string& type, int64 nPoints, T min, T max, T step)
{
  fprintf(outputFile, "%s %lld %s\n", axis.c_str(), static_cast<long long unsigned int>(nPoints), type.c_str());
  if(m_InputValues->WriteBinaryFile)
  {
    std::vector<T> data(nPoints);
    T d;
    for(int idx = 0; idx < nPoints; ++idx)
    {
      d = idx * step + min;
      if constexpr(endian::little == endian::native)
      {
        d = complex::byteswap(d);
      }
      data[idx] = d;
    }
    const usize totalWritten = fwrite(static_cast<void*>(data.data()), sizeof(T), static_cast<usize>(nPoints), outputFile);
    fprintf(outputFile, "\n"); // Write a newline character at the end of the coordinates
    if(totalWritten != static_cast<usize>(nPoints))
    {
      fclose(outputFile);
      return MakeErrorResult(-2074, fmt::format("Error Writing Binary VTK Data into file"));
    }
  }
  else
  {
    T d;
    for(int idx = 0; idx < nPoints; ++idx)
    {
      d = idx * step + min;
      fprintf(outputFile, "%f ", d);
      if(idx % 20 == 0 && idx != 0)
      {
        fprintf(outputFile, "\n");
      }
    }
    fprintf(outputFile, "\n");
  }
  return {};
}
