#include "AvizoRectilinearCoordinateWriter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AvizoRectilinearCoordinateWriter::AvizoRectilinearCoordinateWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   AvizoRectilinearCoordinateWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AvizoRectilinearCoordinateWriter::~AvizoRectilinearCoordinateWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AvizoRectilinearCoordinateWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AvizoRectilinearCoordinateWriter::operator()()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  FILE* outputFile = fopen(m_InputValues->OutputFile.string().c_str(), "wb");
  if(outputFile == nullptr)
  {
    return MakeErrorResult(-5830, fmt::format("Error creating output file {}", m_InputValues->OutputFile.string()));
  }

  Result<> headerResult = generateHeader(outputFile);
  if(headerResult.invalid())
  {
    return headerResult;
  }

  Result<> result = writeData(outputFile);
  if(result.invalid())
  {
    return result;
  }

  fclose(outputFile);

  return {};
}

// -----------------------------------------------------------------------------
Result<> AvizoRectilinearCoordinateWriter::generateHeader(FILE* outputFile) const
{
  const auto& geom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);

  if(m_InputValues->WriteBinaryFile)
  {
#ifdef CMP_WORDS_BIGENDIAN
    fprintf(outputFile, "# AmiraMesh BINARY 2.1\n");
#else
    fprintf(outputFile, "# AmiraMesh BINARY-LITTLE-ENDIAN 2.1\n");
#endif
  }
  else
  {
    fprintf(outputFile, "# AmiraMesh 3D ASCII 2.0\n");
  }
  fprintf(outputFile, "\n");
  fprintf(outputFile, "# Dimensions in x-, y-, and z-direction\n");
  SizeVec3 geoDim = geom.getDimensions();

  fprintf(outputFile, "define Lattice %llu %llu %llu\n", static_cast<unsigned long long>(geoDim[0]), static_cast<unsigned long long>(geoDim[1]), static_cast<unsigned long long>(geoDim[2]));
  fprintf(outputFile, "define Coordinates %llu\n\n", static_cast<unsigned long long>(geoDim[0] + geoDim[1] + geoDim[2]));

  fprintf(outputFile, "Parameters {\n");
  fprintf(outputFile, "     DREAM3DParams {\n");
  fprintf(outputFile, "         Author \"DREAM.3D ComplexCore Version 7.0.0\",\n");
  const std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  const std::string timeString = std::ctime(&currentTime);
  fprintf(outputFile, "         DateTime \"%s\"\n", timeString.substr(0, timeString.length() - 1).c_str()); // remove the \n character from the time string
  fprintf(outputFile, "         FeatureIds Path \"%s\"\n", m_InputValues->FeatureIdsArrayPath.toString().c_str());
  fprintf(outputFile, "     }\n");

  fprintf(outputFile, "     Units {\n");
  fprintf(outputFile, "         Coordinates \"%s\"\n", m_InputValues->Units.c_str());
  fprintf(outputFile, "     }\n");

  fprintf(outputFile, "     CoordType \"rectilinear\"\n");
  fprintf(outputFile, "}\n\n");

  fprintf(outputFile, "Lattice { int FeatureIds } = @1\n");
  fprintf(outputFile, "Coordinates { float xyz } = @2\n\n");

  fprintf(outputFile, "# Data section follows\n");

  return {};
}

// -----------------------------------------------------------------------------
Result<> AvizoRectilinearCoordinateWriter::writeData(FILE* outputFile) const
{
  const auto& geom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);
  SizeVec3 dims = geom.getDimensions();
  FloatVec3 origin = geom.getOrigin();
  FloatVec3 res = geom.getSpacing();

  fprintf(outputFile, "@1 # FeatureIds in z, y, x with X moving fastest, then Y, then Z\n");

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const usize totalPoints = featureIds.getNumberOfTuples();

  if(m_InputValues->WriteBinaryFile)
  {
    fwrite(featureIds.getIDataStoreAs<Int32DataStore>()->data(), sizeof(int32_t), totalPoints, outputFile);
  }
  else
  {
    // The "20 Items" is purely arbitrary and is put in to try and save some space in the ASCII file
    int count = 0;
    for(size_t i = 0; i < totalPoints; ++i)
    {
      fprintf(outputFile, "%d", featureIds[i]);
      if(count < 20)
      {
        fprintf(outputFile, " ");
        count++;
      }
      else
      {
        fprintf(outputFile, "\n");
        count = 0;
      }
    }
  }
  fprintf(outputFile, "\n");

  fprintf(outputFile, "@2 # x coordinates, then y, then z\n");

  if(m_InputValues->WriteBinaryFile)
  {
    for(int d = 0; d < 3; ++d)
    {
      std::vector<float> coords(dims[d]);
      for(size_t i = 0; i < dims[d]; ++i)
      {
        coords[i] = origin[d] + (res[d] * i);
      }
      fwrite(reinterpret_cast<char*>(coords.data()), sizeof(char), sizeof(char) * sizeof(float) * dims[d], outputFile);
      fprintf(outputFile, "\n");
    }
  }
  else
  {
    for(int d = 0; d < 3; ++d)
    {
      for(size_t i = 0; i < dims[d]; ++i)
      {
        fprintf(outputFile, "%f ", origin[d] + (res[d] * i));
      }
      fprintf(outputFile, "\n");
    }
  }

  return {};
}
