#include "WriteAvizoUniformCoordinate.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

#include <chrono>
#include <ctime>

using namespace complex;

// -----------------------------------------------------------------------------
WriteAvizoUniformCoordinate::WriteAvizoUniformCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         AvizoWriterInputValues* inputValues)
: AvizoWriter(dataStructure, mesgHandler, shouldCancel, inputValues)
{
}

// -----------------------------------------------------------------------------
WriteAvizoUniformCoordinate::~WriteAvizoUniformCoordinate() noexcept = default;

// -----------------------------------------------------------------------------
Result<> WriteAvizoUniformCoordinate::operator()()
{
  return AvizoWriter::execute();
}

// -----------------------------------------------------------------------------
Result<> WriteAvizoUniformCoordinate::generateHeader(FILE* outputFile) const
{
  const auto& geom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);

  if(m_InputValues->WriteBinaryFile)
  {
    if constexpr(endian::big == endian::native)
    {
      fprintf(outputFile, "# AmiraMesh BINARY 2.1\n");
    }
    else
    {
      fprintf(outputFile, "# AmiraMesh BINARY-LITTLE-ENDIAN 2.1\n");
    }
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
  const std::string timeString = std::ctime(&currentTime);
  fprintf(outputFile, "         DateTime \"%s\"\n", timeString.substr(0, timeString.length() - 1).c_str()); // remove the \n character from the time string
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
Result<> WriteAvizoUniformCoordinate::writeData(FILE* outputFile) const
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
  return {};
}