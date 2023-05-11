#include "AvizoUniformCoordinateWriter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <chrono>
#include <ctime>

using namespace complex;

// -----------------------------------------------------------------------------
AvizoUniformCoordinateWriter::AvizoUniformCoordinateWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           AvizoUniformCoordinateWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AvizoUniformCoordinateWriter::~AvizoUniformCoordinateWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AvizoUniformCoordinateWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AvizoUniformCoordinateWriter::operator()()
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

  return {};
}

// -----------------------------------------------------------------------------
Result<> AvizoUniformCoordinateWriter::generateHeader(FILE* outputFile)
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
  SizeVec3 dims = geom.getDimensions();

  fprintf(outputFile, "define Lattice %llu %llu %llu\n", static_cast<unsigned long long>(dims[0]), static_cast<unsigned long long>(dims[1]), static_cast<unsigned long long>(dims[2]));

  fprintf(outputFile, "Parameters {\n");
  fprintf(outputFile, "     DREAM3DParams {\n");
  fprintf(outputFile, "         Author \"DREAM.3D ComplexCore Version 7.0.0\",\n");

  const std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  fprintf(outputFile, "         DateTime \"%s\"\n", std::ctime(&currentTime));
  fprintf(outputFile, "         FeatureIds Path \"%s\"\n", m_InputValues->FeatureIdsArrayPath.toString().c_str());
  fprintf(outputFile, "     }\n");

  fprintf(outputFile, "     Units {\n");
  fprintf(outputFile, "         Coordinates \"%s\"\n", m_InputValues->Units.c_str());
  fprintf(outputFile, "     }\n");

  fprintf(outputFile, "     Content \"%llux%llux%llu int, uniform coordinates\",\n", static_cast<unsigned long long int>(dims[0]), static_cast<unsigned long long int>(dims[1]),
          static_cast<unsigned long long int>(dims[2]));

  FloatVec3 origin = geom.getOrigin();
  FloatVec3 res = geom.getSpacing();
  fprintf(outputFile, "     # Bounding Box is xmin xmax ymin ymax zmin zmax\n");
  fprintf(outputFile, "     BoundingBox %f %f %f %f %f %f\n", origin[0], origin[0] + (res[0] * dims[0]), origin[1], origin[1] + (res[1] * dims[1]), origin[2], origin[2] + (res[2] * dims[2]));

  fprintf(outputFile, "     CoordType \"uniform\"\n");
  fprintf(outputFile, "}\n\n");

  fprintf(outputFile, "Lattice { int FeatureIds } = @1\n");

  fprintf(outputFile, "# Data section follows\n");

  return {};
}

// -----------------------------------------------------------------------------
Result<> AvizoUniformCoordinateWriter::writeData(FILE* outputFile)
{
  fprintf(outputFile, "@1\n");

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const usize totalPoints = featureIds.getNumberOfTuples();

  if(m_InputValues->WriteBinaryFile)
  {
    fwrite(featureIds.getIDataStoreAs<Int32DataStore>()->data(), sizeof(int32), totalPoints, outputFile);
  }
  else
  {
    // The "20 Items" is purely arbitrary and is put in to try and save some space in the ASCII file
    int count = 0;
    for(size_t i = 0; i < totalPoints; ++i)
    {
      auto debug = fprintf(outputFile, "%d", featureIds[i]);
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
  return {};
}