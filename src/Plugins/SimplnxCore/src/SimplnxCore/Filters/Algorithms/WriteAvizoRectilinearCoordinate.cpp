#include "WriteAvizoRectilinearCoordinate.hpp"

#include "simplnx/Common/Bit.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <limits>
#include <utility>

using namespace nx::core;

WriteAvizoRectilinearCoordinate::WriteAvizoRectilinearCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 AvizoWriterInputValues* inputValues)
: AvizoWriter(dataStructure, mesgHandler, shouldCancel, inputValues)
{
}

WriteAvizoRectilinearCoordinate::~WriteAvizoRectilinearCoordinate() noexcept = default;

Result<> WriteAvizoRectilinearCoordinate::operator()()
{
  return AvizoWriter::execute();
}

Result<> WriteAvizoRectilinearCoordinate::generateHeader(FILE* outputFile) const
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
  SizeVec3 geoDim = geom.getDimensions();

  if(auto outputResult =
         printOutput(outputFile, "define Lattice %llu %llu %llu\n", static_cast<unsigned long long>(geoDim[0]), static_cast<unsigned long long>(geoDim[1]), static_cast<unsigned long long>(geoDim[2]));
     outputResult.invalid())
  {
    return outputResult;
  }
  if(auto outputResult = printOutput(outputFile, "define Coordinates %llu\n\n", static_cast<unsigned long long>(geoDim[0] + geoDim[1] + geoDim[2])); outputResult.invalid())
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

  if(auto outputResult = printOutput(outputFile, "     CoordType \"rectilinear\"\n"); outputResult.invalid())
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
  if(auto outputResult = printOutput(outputFile, "Coordinates { float xyz } = @2\n\n"); outputResult.invalid())
  {
    return outputResult;
  }

  if(auto outputResult = printOutput(outputFile, "# Data section follows\n"); outputResult.invalid())
  {
    return outputResult;
  }

  return {};
}

Result<> WriteAvizoRectilinearCoordinate::writeData(FILE* outputFile) const
{
  Result<> result;
  const auto& geom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);
  SizeVec3 dims = geom.getDimensions();
  FloatVec3 origin = geom.getOrigin();
  FloatVec3 res = geom.getSpacing();

  if(auto outputResult = printOutput(outputFile, "@1 # FeatureIds in z, y, x with X moving fastest, then Y, then Z\n"); outputResult.invalid())
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

  if(auto outputResult = printOutput(outputFile, "@2 # x coordinates, then y, then z\n"); outputResult.invalid())
  {
    return MergeResults(std::move(result), std::move(outputResult));
  }

  if(m_InputValues->WriteBinaryFile)
  {
    constexpr char k_AxisNames[] = "XYZ";
    for(int d = 0; d < 3; ++d)
    {
      if(dims[d] > std::numeric_limits<usize>::max() / sizeof(float32))
      {
        auto overflowError = MakeErrorResult(
            -5833, fmt::format("Cannot write {} coordinates to Avizo output '{}': {} float values exceed the byte-count limit.", k_AxisNames[d], m_InputValues->OutputFile.string(), dims[d]));
        return MergeResults(std::move(result), std::move(overflowError));
      }
      const usize axisBytes = dims[d] * sizeof(float32);
      std::vector<float32> coords(dims[d]);
      for(usize i = 0; i < dims[d]; ++i)
      {
        coords[i] = origin[d] + (res[d] * i);
      }
      if(auto outputResult = writeOutputChecked(outputFile, reinterpret_cast<const char*>(coords.data()), sizeof(char), axisBytes); outputResult.invalid())
      {
        return MergeResults(std::move(result), std::move(outputResult));
      }
      if(auto outputResult = printOutput(outputFile, "\n"); outputResult.invalid())
      {
        return MergeResults(std::move(result), std::move(outputResult));
      }
    }
  }
  else
  {
    for(int d = 0; d < 3; ++d)
    {
      for(usize i = 0; i < dims[d]; ++i)
      {
        if(auto outputResult = printOutput(outputFile, "%f ", origin[d] + (res[d] * i)); outputResult.invalid())
        {
          return MergeResults(std::move(result), std::move(outputResult));
        }
      }
      if(auto outputResult = printOutput(outputFile, "\n"); outputResult.invalid())
      {
        return MergeResults(std::move(result), std::move(outputResult));
      }
    }
  }

  return result;
}
