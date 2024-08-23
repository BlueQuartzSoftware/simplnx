#include "WriteVtkRectilinearGrid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/utils/VtkUtilities.hpp"

using namespace nx::core;

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
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 res = imageGeom.getSpacing();
  FloatVec3 origin = imageGeom.getOrigin();

  FILE* outputFile = nullptr;
  outputFile = fopen(m_InputValues->OutputFile.string().c_str(), "wb");
  if(nullptr == outputFile)
  {
    return MakeErrorResult(-2073, fmt::format("Error opening output vtk file '{}'", m_InputValues->OutputFile.string()));
  }

  // write the header
  writeVtkHeader(outputFile);

  // Write the Coordinate Points
  Result<> writeCoordsResults = writeCoords<float32>(outputFile, "X_COORDINATES", "float", dims[0] + 1, origin[0] - res[0] * 0.5f, res[0]);
  if(writeCoordsResults.invalid())
  {
    return MergeResults(writeCoordsResults, MakeErrorResult(-2075, fmt::format("Error writing X Coordinates in vtk file {}'\n ", m_InputValues->OutputFile.string())));
  }
  writeCoordsResults = writeCoords<float32>(outputFile, "Y_COORDINATES", "float", dims[1] + 1, origin[1] - res[1] * 0.5f, res[1]);
  if(writeCoordsResults.invalid())
  {
    return MergeResults(writeCoordsResults, MakeErrorResult(-2076, fmt::format("Error writing Y Coordinates in vtk file %s'\n ", m_InputValues->OutputFile.string())));
  }
  writeCoordsResults = writeCoords<float32>(outputFile, "Z_COORDINATES", "float", dims[2] + 1, origin[2] - res[2] * 0.5f, res[2]);
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
  fprintf(outputFile, "Data set from DREAM.3D SimplnxCore version 7.0.0\n");
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
Result<> WriteVtkRectilinearGrid::writeCoords(FILE* outputFile, const std::string& axis, const std::string& type, int64 nPoints, T min, T step)
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
        d = nx::core::byteswap(d);
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
