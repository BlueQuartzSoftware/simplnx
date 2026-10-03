#include "WriteAvizoUniformCoordinate.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <ctime>
#include <utility>

using namespace nx::core;

WriteAvizoUniformCoordinate::WriteAvizoUniformCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         AvizoWriterInputValues* inputValues)
: AvizoWriter(dataStructure, mesgHandler, shouldCancel, inputValues)
{
}

WriteAvizoUniformCoordinate::~WriteAvizoUniformCoordinate() noexcept = default;

Result<> WriteAvizoUniformCoordinate::operator()()
{
  return AvizoWriter::execute();
}

Result<> WriteAvizoUniformCoordinate::generateHeader(FILE* outputFile) const
{
  const auto& geom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);

  if(m_InputValues->WriteBinaryFile)
  {
    if constexpr(endian::big == endian::native)
    {
      if(auto outputResult = printOutput(outputFile, "# AmiraMesh BINARY 2.1\n"); outputResult.invalid())
      {
        return outputResult;
      }
    }
    else
    {
      if(auto outputResult = printOutput(outputFile, "# AmiraMesh BINARY-LITTLE-ENDIAN 2.1\n"); outputResult.invalid())
      {
        return outputResult;
      }
    }
  }
  else
  {
    if(auto outputResult = printOutput(outputFile, "# AmiraMesh 3D ASCII 2.0\n"); outputResult.invalid())
    {
      return outputResult;
    }
  }
  if(auto outputResult = printOutput(outputFile, "\n"); outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "# Dimensions in x-, y-, and z-direction\n"); outputResult.invalid())
  {
    return outputResult;
  }
  SizeVec3 dims = geom.getDimensions();

  if(auto outputResult =
         printOutput(outputFile, "define Lattice %llu %llu %llu\n", static_cast<unsigned long long>(dims[0]), static_cast<unsigned long long>(dims[1]), static_cast<unsigned long long>(dims[2]));
     outputResult.invalid())
  {
    return outputResult;
  }

  if(auto outputResult = printOutput(outputFile, "Parameters {\n"); outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "     DREAM3DParams {\n"); outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "         Author \"DREAM3D-NX SimplnxCore Version 7.0.0\",\n"); outputResult.invalid())
  {
    return outputResult;
  }

  const std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  const std::string timeString = std::ctime(&currentTime);
  // ctime() includes a final newline that is not part of the quoted value.
  if(auto outputResult = printOutput(outputFile, "         DateTime \"%s\"\n", timeString.substr(0, timeString.length() - 1).c_str()); outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "         FeatureIds Path \"%s\"\n", m_InputValues->FeatureIdsArrayPath.toString().c_str()); outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "     }\n"); outputResult.invalid())
  {
    return outputResult;
  }

  if(auto outputResult = printOutput(outputFile, "     Units {\n"); outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "         Coordinates \"%s\"\n", m_InputValues->Units.c_str()); outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "     }\n"); outputResult.invalid())
  {
    return outputResult;
  }

  if(auto outputResult = printOutput(outputFile, "     Content \"%llux%llux%llu int, uniform coordinates\",\n", static_cast<unsigned long long int>(dims[0]),
                                     static_cast<unsigned long long int>(dims[1]), static_cast<unsigned long long int>(dims[2]));
     outputResult.invalid())
  {
    return outputResult;
  }

  FloatVec3 origin = geom.getOrigin();
  FloatVec3 res = geom.getSpacing();
  if(auto outputResult = printOutput(outputFile, "     # Bounding Box is xmin xmax ymin ymax zmin zmax\n"); outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "     BoundingBox %f %f %f %f %f %f\n", origin[0], origin[0] + (res[0] * dims[0]), origin[1], origin[1] + (res[1] * dims[1]), origin[2],
                                     origin[2] + (res[2] * dims[2]));
     outputResult.invalid())
  {
    return outputResult;
  }

  if(auto outputResult = printOutput(outputFile, "     CoordType \"uniform\"\n"); outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "}\n\n"); outputResult.invalid())
  {
    return outputResult;
  }

  if(auto outputResult = printOutput(outputFile, "Lattice { int FeatureIds } = @1\n"); outputResult.invalid())
  {
    return outputResult;
  }

  if(auto outputResult = printOutput(outputFile, "# Data section follows\n"); outputResult.invalid())
  {
    return outputResult;
  }

  return {};
}

Result<> WriteAvizoUniformCoordinate::writeData(FILE* outputFile) const
{
  Result<> result;
  if(auto outputResult = printOutput(outputFile, "@1\n"); outputResult.invalid())
  {
    return MergeResults(std::move(result), std::move(outputResult));
  }

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const usize totalPoints = featureIds.getNumberOfTuples();

  constexpr usize k_ChunkSize = 65536;
  std::vector<int32> buffer(k_ChunkSize);
  const auto& featureIdsStore = featureIds.getDataStoreRef();
  if(m_InputValues->WriteBinaryFile)
  {
    for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
    {
      if(m_ShouldCancel)
      {
        return result;
      }
      const usize count = std::min(k_ChunkSize, totalPoints - offset);
      Result<> readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(buffer.data(), count));
      if(readResult.invalid())
      {
        return MergeResults(std::move(result), std::move(readResult));
      }
      for(auto& warning : readResult.warnings())
      {
        result.warnings().push_back(std::move(warning));
      }
      if(auto outputResult = writeOutputChecked(outputFile, buffer.data(), sizeof(int32), count); outputResult.invalid())
      {
        return MergeResults(std::move(result), std::move(outputResult));
      }
    }
  }
  else
  {
    // Current counter placement inserts a newline after 21 ASCII values.
    int itemCount = 0;
    for(usize offset = 0; offset < totalPoints; offset += k_ChunkSize)
    {
      if(m_ShouldCancel)
      {
        return result;
      }
      const usize count = std::min(k_ChunkSize, totalPoints - offset);
      Result<> readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(buffer.data(), count));
      if(readResult.invalid())
      {
        return MergeResults(std::move(result), std::move(readResult));
      }
      for(auto& warning : readResult.warnings())
      {
        result.warnings().push_back(std::move(warning));
      }
      for(usize i = 0; i < count; ++i)
      {
        if(auto outputResult = printOutput(outputFile, "%d", buffer[i]); outputResult.invalid())
        {
          return MergeResults(std::move(result), std::move(outputResult));
        }
        if(itemCount < 20)
        {
          if(auto outputResult = printOutput(outputFile, " "); outputResult.invalid())
          {
            return MergeResults(std::move(result), std::move(outputResult));
          }
          itemCount++;
        }
        else
        {
          if(auto outputResult = printOutput(outputFile, "\n"); outputResult.invalid())
          {
            return MergeResults(std::move(result), std::move(outputResult));
          }
          itemCount = 0;
        }
      }
    }
  }
  if(auto outputResult = printOutput(outputFile, "\n"); outputResult.invalid())
  {
    return MergeResults(std::move(result), std::move(outputResult));
  }
  return result;
}
