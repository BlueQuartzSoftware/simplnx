#include "WriteOnScaleTableFile.hpp"

#include "SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.hpp"
#include "SimplnxCore/Filters/RotateSampleRefFrameFilter.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <Eigen/Dense>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <numbers>
#include <numeric>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_ScratchGeometryPath({"Image Geometry"});
const DataPath k_ScratchCellDataPath = k_ScratchGeometryPath.createChildPath("Cell Data");

constexpr int32 k_NegativeFeatureIdsError = -12037;
constexpr int32 k_InvalidImageGeometryError = -12039;
constexpr int32 k_InvalidRectGridGeometryError = -12040;
constexpr int32 k_UnexpectedReorderedDimensionsError = -12041;

enum class WriteStatus : uint8
{
  Success,
  Cancelled,
  OpenError,
  WriteError
};

enum class Axis : uint8
{
  X,
  Y,
  Z
};

Eigen::Matrix3f CreateRotationMatrix(float32 angle, Axis axis)
{
  const float32 cosAngle = std::cos(angle);
  const float32 sinAngle = std::sin(angle);
  Eigen::Matrix3f matrix = Eigen::Matrix3f::Identity();

  switch(axis)
  {
  case Axis::X:
    matrix << Eigen::Vector3f{1.0F, 0.0F, 0.0F}, Eigen::Vector3f{0.0F, cosAngle, sinAngle}, Eigen::Vector3f{0.0F, -sinAngle, cosAngle};
    break;
  case Axis::Y:
    matrix << Eigen::Vector3f{cosAngle, 0.0F, -sinAngle}, Eigen::Vector3f{0.0F, 1.0F, 0.0F}, Eigen::Vector3f{sinAngle, 0.0F, cosAngle};
    break;
  case Axis::Z:
    matrix << Eigen::Vector3f{cosAngle, sinAngle, 0.0F}, Eigen::Vector3f{-sinAngle, cosAngle, 0.0F}, Eigen::Vector3f{0.0F, 0.0F, 1.0F};
    break;
  }

  return matrix;
}

Eigen::Matrix3f DetermineRotationMatrix(const SizeVec3& dimensions)
{
  // DREAM.3D 6.6 built a 3x3 table that Rotate Sample Reference Frame rejected because it required a 4x4 table. Thus, the legacy reorder always failed with error -10115. This port implements
  // the intended reorder with a valid 4x4 table.
  // The fixed swap order and left multiplication reproduce the intended DREAM.3D 6.6 axis-reorder matrix.
  std::vector<Eigen::Matrix3f> rotationMatrices;
  SizeVec3 reorderedDimensions = dimensions;
  SizeVec3 sortedDimensions = dimensions;
  std::sort(sortedDimensions.begin(), sortedDimensions.end(), std::greater<usize>());

  if(reorderedDimensions[0] != sortedDimensions[0])
  {
    if(reorderedDimensions[1] == sortedDimensions[0])
    {
      std::swap(reorderedDimensions[0], reorderedDimensions[1]);
      rotationMatrices.push_back(CreateRotationMatrix(std::numbers::pi_v<float32> / 2.0F, Axis::Z));
    }
    else
    {
      std::swap(reorderedDimensions[0], reorderedDimensions[2]);
      rotationMatrices.push_back(CreateRotationMatrix(std::numbers::pi_v<float32> / 2.0F, Axis::Y));
    }
  }

  if(reorderedDimensions[1] != sortedDimensions[1])
  {
    std::swap(reorderedDimensions[1], reorderedDimensions[2]);
    rotationMatrices.push_back(CreateRotationMatrix(std::numbers::pi_v<float32> / 2.0F, Axis::X));
  }

  const Eigen::Matrix3f identity = Eigen::Matrix3f::Identity();
  return std::accumulate(rotationMatrices.cbegin(), rotationMatrices.cend(), identity,
                         [](const Eigen::Matrix3f& accumulated, const Eigen::Matrix3f& next) { return Eigen::Matrix3f(next * accumulated); });
}

DynamicTableParameter::ValueType ConvertRotationMatrixToTable(const Eigen::Matrix3f& matrix)
{
  DynamicTableParameter::ValueType table(4, std::vector<float64>(4, 0.0));
  for(usize row = 0; row < 3; row++)
  {
    for(usize col = 0; col < 3; col++)
    {
      table[row][col] = matrix(row, col);
    }
  }
  table[3][3] = 1.0;
  return table;
}

WriteStatus FlushBuffer(std::ostream& output, fmt::memory_buffer& buffer)
{
  output.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
  buffer.clear();
  return output ? WriteStatus::Success : WriteStatus::WriteError;
}

WriteStatus OpenOutput(const fs::path& filePath, std::ofstream& output)
{
  output.open(filePath, std::ios::binary | std::ios::trunc);
  return output.is_open() ? WriteStatus::Success : WriteStatus::OpenError;
}

void SendProgress(ThrottledMessenger& messenger, std::string_view label, usize current, usize total)
{
  messenger.sendThrottledMessage([=]() { return fmt::format("{}: {:.0f}%", label, CalculatePercentComplete(current, total)); });
}

template <class Generator>
void WriteEntries(fmt::memory_buffer& buffer, usize count, usize maxEntriesPerLine, Generator&& generator)
{
  // The legacy format writes each separator before its entry and leaves matrix data without a trailing newline.
  for(usize index = 0; index < count; index++)
  {
    if(index != 0)
    {
      fmt::format_to(std::back_inserter(buffer), "{}", index % maxEntriesPerLine == 0 ? "\n" : " ");
    }
    fmt::format_to(std::back_inserter(buffer), "{}", generator(index));
  }
}

WriteStatus WriteImageCoordinates(std::ofstream& output, std::string_view label, usize nodeCount, float32 origin, float32 spacing)
{
  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "{} {}\n", label, nodeCount);
  WriteEntries(buffer, nodeCount, 6, [origin, spacing](usize index) { return fmt::format("{:.8E}", origin + (static_cast<float32>(index) * spacing)); });
  fmt::format_to(std::back_inserter(buffer), "\n");
  return FlushBuffer(output, buffer);
}

WriteStatus WriteRectGridCoordinates(std::ofstream& output, std::string_view label, const Float32Array& bounds)
{
  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "{} {}\n", label, bounds.getNumberOfTuples());
  WriteEntries(buffer, bounds.getNumberOfTuples(), 6, [&bounds](usize index) { return fmt::format("{:.8E}", bounds[index]); });
  fmt::format_to(std::back_inserter(buffer), "\n");
  return FlushBuffer(output, buffer);
}

struct FeatureIdStats
{
  usize MaxGrainId = 0;
  usize NegativeCount = 0;
  bool Copied = true;
};

template <class T>
void UpdateFeatureIdStats(T value, FeatureIdStats& stats)
{
  if constexpr(std::is_signed_v<T>)
  {
    if(value < 0)
    {
      stats.NegativeCount++;
      return;
    }
  }
  stats.MaxGrainId = std::max(stats.MaxGrainId, static_cast<usize>(value));
}

struct FindFeatureIdStats
{
  template <class T>
  FeatureIdStats operator()(const IDataArray& featureIds) const
  {
    const auto& typedFeatureIds = dynamic_cast<const DataArray<T>&>(featureIds);
    FeatureIdStats stats;
    for(const T value : typedFeatureIds)
    {
      UpdateFeatureIdStats(value, stats);
    }
    return stats;
  }
};

struct CopyFeatureIds
{
  template <class T>
  FeatureIdStats operator()(const IDataArray& sourceArray, DataStructure& destinationDataStructure, const DataObject::IdType& destinationParentId, const ShapeType& destinationTupleShape) const
  {
    const auto& typedSource = dynamic_cast<const DataArray<T>&>(sourceArray);
    auto destinationStore =
        DataStoreUtilities::CreateDataStore<T>(destinationDataStructure, k_ScratchCellDataPath.createChildPath(sourceArray.getName()), destinationTupleShape, typedSource.getComponentShape());
    auto* destinationArray = DataArray<T>::Create(destinationDataStructure, sourceArray.getName(), destinationStore, destinationParentId);
    if(destinationArray == nullptr)
    {
      return {.Copied = false};
    }

    FeatureIdStats stats;
    for(usize index = 0; index < typedSource.getNumberOfTuples(); index++)
    {
      const T value = typedSource[index];
      (*destinationArray)[index] = value;
      UpdateFeatureIdStats(value, stats);
    }
    return stats;
  }
};

struct WriteFeatureIds
{
  template <class T>
  WriteStatus operator()(std::ofstream& output, const IDataArray& featureIds, const SizeVec3& dimensions, const std::atomic_bool& shouldCancel, MessageHelper& messageHelper) const
  {
    const auto& typedFeatureIds = dynamic_cast<const DataArray<T>&>(featureIds);
    const usize cellsPerSlice = dimensions[0] * dimensions[1];
    fmt::memory_buffer buffer;
    ThrottledMessenger progressMessenger = messageHelper.createThrottledMessenger();
    for(usize z = 0; z < dimensions[2]; z++)
    {
      if(shouldCancel)
      {
        return WriteStatus::Cancelled;
      }

      const usize sliceStart = z * cellsPerSlice;
      const usize sliceEnd = sliceStart + cellsPerSlice;
      for(usize index = sliceStart; index < sliceEnd; index++)
      {
        if(index != 0)
        {
          fmt::format_to(std::back_inserter(buffer), "{}", index % 40 == 0 ? "\n" : " ");
        }
        fmt::format_to(std::back_inserter(buffer), "{}", typedFeatureIds[index]);
      }

      if(FlushBuffer(output, buffer) == WriteStatus::WriteError)
      {
        return WriteStatus::WriteError;
      }
      SendProgress(progressMessenger, "Writing matrix values", z + 1, dimensions[2]);
    }
    return WriteStatus::Success;
  }
};
} // namespace

WriteOnScaleTableFile::WriteOnScaleTableFile(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                             WriteOnScaleTableFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

WriteOnScaleTableFile::~WriteOnScaleTableFile() noexcept = default;

Result<> WriteOnScaleTableFile::operator()()
{
  const auto& inputGeometry = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->InputGeometryPath);
  const auto& inputFeatureIds = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
  const auto& phaseNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->PhaseNamesArrayPath);

  if(inputFeatureIds.getNumberOfTuples() == 0)
  {
    return MakeErrorResult(-12027, fmt::format("The feature IDs array '{}' has 0 tuples. Select an array with at least one tuple.", m_InputValues->FeatureIdsArrayPath.toString()));
  }

  const IGridGeometry* exportGeometry = &inputGeometry;
  const IDataArray* exportFeatureIds = &inputFeatureIds;
  FeatureIdStats featureIdStats;
  DataStructure rotatedDataStructure;
  const SizeVec3 inputDimensions = inputGeometry.getDimensions();
  if(!(inputDimensions[0] >= inputDimensions[1] && inputDimensions[1] >= inputDimensions[2]))
  {
    // The sub-filter rotates a private copy so the export does not change the input data structure. Peak memory is three times the feature ID bytes while the sub-filter runs: the input, the scratch
    // copy, and the rotated output. Memory remains at two times the feature ID bytes after the sub-filter removes the scratch copy and until this algorithm returns.
    auto* scratchGeometry = ImageGeom::Create(rotatedDataStructure, k_ScratchGeometryPath.getTargetName());
    if(scratchGeometry == nullptr)
    {
      return MakeErrorResult(-12028, "Failed to create the scratch Image Geometry for the Rotate Sample Reference Frame sub-filter.");
    }
    const auto* inputImageGeometry = dynamic_cast<const ImageGeom*>(&inputGeometry);
    if(inputImageGeometry == nullptr)
    {
      return MakeErrorResult(k_InvalidImageGeometryError, fmt::format("The grid geometry '{}' requires reordering but is not an Image Geometry.", m_InputValues->InputGeometryPath.toString()));
    }
    scratchGeometry->setDimensions(inputImageGeometry->getDimensions());
    scratchGeometry->setOrigin(inputImageGeometry->getOrigin());
    scratchGeometry->setSpacing(inputImageGeometry->getSpacing());

    const ShapeType scratchCellShape = {inputDimensions[2], inputDimensions[1], inputDimensions[0]};
    auto* scratchCellData = AttributeMatrix::Create(rotatedDataStructure, k_ScratchCellDataPath.getTargetName(), scratchCellShape, scratchGeometry->getId());
    if(scratchCellData == nullptr)
    {
      return MakeErrorResult(-12029, "Failed to create the scratch Cell Data Attribute Matrix for the Rotate Sample Reference Frame sub-filter.");
    }
    scratchGeometry->setCellData(*scratchCellData);
    // The copy pass also validates IDs and finds the maximum ID. Thus, reordered data does not need an additional scan.
    featureIdStats = ExecuteDataFunctionIntType(CopyFeatureIds{}, inputFeatureIds.getDataType(), inputFeatureIds, rotatedDataStructure, scratchCellData->getId(), scratchCellShape);
    if(!featureIdStats.Copied)
    {
      return MakeErrorResult(-12030, fmt::format("Failed to copy the feature IDs array '{}' for the Rotate Sample Reference Frame sub-filter.", m_InputValues->FeatureIdsArrayPath.toString()));
    }
    if(featureIdStats.NegativeCount > 0)
    {
      return MakeErrorResult(k_NegativeFeatureIdsError, fmt::format("Found {} negative feature ids in '{}'. OnScale material indices must be 0 or greater.", featureIdStats.NegativeCount,
                                                                    m_InputValues->FeatureIdsArrayPath.toString()));
    }

    RotateSampleRefFrameFilter rotateFilter;
    Arguments rotateArgs = rotateFilter.getDefaultArguments();
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                              std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::RotationMatrix)));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key,
                              std::make_any<DynamicTableParameter::ValueType>(ConvertRotationMatrixToTable(DetermineRotationMatrix(inputDimensions))));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ScratchGeometryPath));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotateSliceBySlice_Key, std::make_any<bool>(false));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_KeepInputGeometryOrigin_Key, std::make_any<bool>(false));

    // Correctness depends on Rotate Sample Reference Frame treating this exact 90-degree matrix as a lossless grid rotation through its IsLosslessGridRotation tolerance. A direct index permutation
    // would be less expensive, but the sub-filter preserves parity with the intended legacy design.
    auto rotateExecute = rotateFilter.execute(rotatedDataStructure, rotateArgs, nullptr, m_MessageHandler, m_ShouldCancel);
    if(m_ShouldCancel)
    {
      return {};
    }
    if(rotateExecute.result.invalid())
    {
      return MakeErrorResult(-12032, fmt::format("The Rotate Sample Reference Frame sub-filter failed while reordering geometry '{}': {}", m_InputValues->InputGeometryPath.toString(),
                                                 rotateExecute.result.errors().front().message));
    }

    exportGeometry = &rotatedDataStructure.getDataRefAs<ImageGeom>(k_ScratchGeometryPath);
    const SizeVec3 reorderedDimensions = exportGeometry->getDimensions();
    SizeVec3 expectedDimensions = inputDimensions;
    std::sort(expectedDimensions.begin(), expectedDimensions.end(), std::greater<usize>());
    // ImageRotationUtilities::CreateRotationArgs selects output spacing from matrix columns, which show where each old axis lands.
    // New-axis spacing requires matrix rows, or equivalently the columns of the transposed matrix.
    if(reorderedDimensions != expectedDimensions)
    {
      return MakeErrorResult(
          k_UnexpectedReorderedDimensionsError,
          fmt::format("Rotate Sample Reference Frame produced dimensions ({}) for geometry '{}' but the OnScale reorder expected ({}). This happens for two-axis reorders of geometries with "
                      "anisotropic spacing because the rotation utility assigns output spacing from the wrong matrix axis. Resample to isotropic spacing or reorder the axes before this filter.",
                      StringUtilities::formatDimensions3D(reorderedDimensions), m_InputValues->InputGeometryPath.toString(), StringUtilities::formatDimensions3D(expectedDimensions)));
    }
    exportFeatureIds = &rotatedDataStructure.getDataRefAs<IDataArray>(k_ScratchCellDataPath.createChildPath(inputFeatureIds.getName()));
    m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Applied an OnScale axis reorder with Rotate Sample Reference Frame. New dimensions: {} x {} x {}.", reorderedDimensions[0],
                                                                reorderedDimensions[1], reorderedDimensions[2])});
  }
  else
  {
    featureIdStats = ExecuteDataFunctionIntType(FindFeatureIdStats{}, exportFeatureIds->getDataType(), *exportFeatureIds);
    if(featureIdStats.NegativeCount > 0)
    {
      return MakeErrorResult(k_NegativeFeatureIdsError, fmt::format("Found {} negative feature ids in '{}'. OnScale material indices must be 0 or greater.", featureIdStats.NegativeCount,
                                                                    m_InputValues->FeatureIdsArrayPath.toString()));
    }
  }

  const usize maxGrainId = featureIdStats.MaxGrainId;
  if(maxGrainId == 0)
  {
    m_MessageHandler(IFilter::Message::Type::Warning, fmt::format("No positive feature ids were found in '{}'; the name section is empty.", m_InputValues->FeatureIdsArrayPath.toString()));
  }

  const fs::path outputPath = m_InputValues->OutputPath / fmt::format("{}.flxtbl", m_InputValues->FilePrefix);
  auto atomicFileResult = AtomicFile::Create(outputPath);
  if(atomicFileResult.invalid())
  {
    return MakeErrorResult(-12033, fmt::format("Failed to create a temporary output file for '{}': {}", outputPath.string(), atomicFileResult.errors().front().message));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());
  const fs::path tempPath = atomicFile.tempFilePath();
  std::ofstream output;
  const WriteStatus openStatus = OpenOutput(tempPath, output);
  if(openStatus == WriteStatus::OpenError)
  {
    return MakeErrorResult(-12034, fmt::format("Could not open '{}' for writing (target '{}').", tempPath.string(), outputPath.string()));
  }

  const auto makeWriteError = [&tempPath, &outputPath]() {
    return MakeErrorResult(-12035, fmt::format("Writing to '{}' failed (target '{}'). Check available disk space.", tempPath.string(), outputPath.string()));
  };
  MessageHelper messageHelper(m_MessageHandler);

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale header..."});
  output << "hedr 0\ninfo 1\n";

  const SizeVec3 dimensions = exportGeometry->getDimensions();
  if(const auto* imageGeometry = dynamic_cast<const ImageGeom*>(exportGeometry); imageGeometry != nullptr)
  {
    const FloatVec3 origin = imageGeometry->getOrigin();
    const FloatVec3 spacing = imageGeometry->getSpacing();
    const std::array<std::string_view, 3> labels = {"xcrd", "ycrd", "zcrd"};
    for(usize axis = 0; axis < labels.size(); axis++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Writing {} coordinates...", labels[axis])});
      if(WriteImageCoordinates(output, labels[axis], dimensions[axis] + 1, origin[axis], spacing[axis]) == WriteStatus::WriteError)
      {
        return makeWriteError();
      }
    }
  }
  else
  {
    const auto* rectGridGeometry = dynamic_cast<const RectGridGeom*>(exportGeometry);
    if(rectGridGeometry == nullptr)
    {
      return MakeErrorResult(k_InvalidRectGridGeometryError,
                             fmt::format("The grid geometry '{}' is neither an Image Geometry nor a Rectilinear Grid Geometry.", m_InputValues->InputGeometryPath.toString()));
    }
    const std::array<std::pair<std::string_view, const Float32Array*>, 3> bounds = {
        {{"xcrd", rectGridGeometry->getXBounds()}, {"ycrd", rectGridGeometry->getYBounds()}, {"zcrd", rectGridGeometry->getZBounds()}}};
    for(const auto& [label, boundsArray] : bounds)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Writing {} coordinates...", label)});
      if(WriteRectGridCoordinates(output, label, *boundsArray) == WriteStatus::WriteError)
      {
        return makeWriteError();
      }
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale keypoints..."});
  output << "keypoints\n" << m_InputValues->NumKeypoints[0] << ' ' << m_InputValues->NumKeypoints[1] << ' ' << m_InputValues->NumKeypoints[2] << '\n';

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale divisions..."});
  output << "divisions\n" << dimensions[0] << ' ' << dimensions[1] << ' ' << dimensions[2] << '\n';

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale phase names..."});
  output << "name " << maxGrainId << '\n';
  const usize phaseNameCount = phaseNames.getNumberOfTuples();
  for(usize grainId = 1; grainId <= maxGrainId; grainId++)
  {
    output << (grainId < phaseNameCount ? phaseNames[grainId] : fmt::format("Phase_{}", grainId)) << ' ';
  }
  output << "\nmatr " << dimensions[0] * dimensions[1] * dimensions[2] << '\n';

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale matrix values..."});
  const WriteStatus matrixStatus = ExecuteDataFunctionIntType(WriteFeatureIds{}, exportFeatureIds->getDataType(), output, *exportFeatureIds, dimensions, m_ShouldCancel, messageHelper);
  if(matrixStatus == WriteStatus::Cancelled || m_ShouldCancel)
  {
    return {};
  }
  if(matrixStatus == WriteStatus::WriteError)
  {
    return makeWriteError();
  }

  output.close();
  if(output.fail())
  {
    return makeWriteError();
  }

  Result<> commitResult = atomicFile.commit();
  if(commitResult.invalid())
  {
    return MakeErrorResult(-12036, fmt::format("Failed to commit the OnScale table file '{}': {}", outputPath.string(), commitResult.errors().front().message));
  }
  return {};
}
