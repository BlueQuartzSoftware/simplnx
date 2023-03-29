#include "AppendImageGeometryZSlice.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"

using namespace complex;

namespace
{
template <typename T>
class AppendImageGeomDataArray
{
public:
  AppendImageGeomDataArray(const IDataArray* inputCellArray, IDataArray* destCellArray, usize tupleOffset)
  : m_InputCellArray(dynamic_cast<const DataArray<T>*>(inputCellArray))
  , m_DestCellArray(dynamic_cast<DataArray<T>*>(destCellArray))
  , m_TupleOffset(tupleOffset)
  {
  }

  ~AppendImageGeomDataArray() = default;

  AppendImageGeomDataArray(const AppendImageGeomDataArray&) = default;
  AppendImageGeomDataArray(AppendImageGeomDataArray&&) noexcept = default;
  AppendImageGeomDataArray& operator=(const AppendImageGeomDataArray&) = delete;
  AppendImageGeomDataArray& operator=(AppendImageGeomDataArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    auto srcBegin = m_InputCellArray->cbegin();
    auto srcEnd = m_InputCellArray->cend();
    auto dstBegin = m_DestCellArray->begin() + (m_TupleOffset * m_DestCellArray->getNumberOfComponents());
    std::copy(srcBegin, srcEnd, dstBegin);
  }

private:
  const DataArray<T>* m_InputCellArray = nullptr;
  DataArray<T>* m_DestCellArray = nullptr;
  usize m_TupleOffset;
};

template <typename T>
class CombineImageGeomDataArray
{
public:
  CombineImageGeomDataArray(const IDataArray* inputCellArray1, const IDataArray* inputCellArray2, IDataArray* destCellArray)
  : m_InputCellArray1(dynamic_cast<const DataArray<T>*>(inputCellArray1))
  , m_InputCellArray2(dynamic_cast<const DataArray<T>*>(inputCellArray2))
  , m_DestCellArray(dynamic_cast<DataArray<T>*>(destCellArray))
  {
  }

  ~CombineImageGeomDataArray() = default;

  CombineImageGeomDataArray(const CombineImageGeomDataArray&) = default;
  CombineImageGeomDataArray(CombineImageGeomDataArray&&) noexcept = default;
  CombineImageGeomDataArray& operator=(const CombineImageGeomDataArray&) = delete;
  CombineImageGeomDataArray& operator=(CombineImageGeomDataArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    auto src1Begin = m_InputCellArray1->cbegin();
    auto src1End = m_InputCellArray1->cend();
    auto dstBegin = m_DestCellArray->begin();
    std::copy(src1Begin, src1End, dstBegin);

    const usize offset = m_InputCellArray1->getSize();
    auto src2Begin = m_InputCellArray2->cbegin();
    auto src2End = m_InputCellArray2->cend();
    auto dstOffsetBegin = m_DestCellArray->begin() + (offset * m_DestCellArray->getNumberOfComponents());
    std::copy(src2Begin, src2End, dstOffsetBegin);
  }

private:
  const DataArray<T>* m_InputCellArray1 = nullptr;
  const DataArray<T>* m_InputCellArray2 = nullptr;
  DataArray<T>* m_DestCellArray = nullptr;
};
} // namespace

// -----------------------------------------------------------------------------
AppendImageGeometryZSlice::AppendImageGeometryZSlice(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     AppendImageGeometryZSliceInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AppendImageGeometryZSlice::~AppendImageGeometryZSlice() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AppendImageGeometryZSlice::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AppendImageGeometryZSlice::operator()()
{
  Result<> results = {};

  const auto& inputGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputGeometryPath);
  auto& destGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DestinationGeometryPath);
  AttributeMatrix* destCellData = destGeometry.getCellData();

  const DataPath destCellDataPath = m_InputValues->DestinationGeometryPath.createChildPath(destCellData->getName());
  const DataPath inputCellDataPath = m_InputValues->InputGeometryPath.createChildPath(inputGeometry.getCellData()->getName());

  if(m_InputValues->SaveAsNewGeometry)
  {
    // copy over the user defined "destination" arrays to the new geometry first and then the "input" arrays
    auto* newCellData = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->NewGeometryPath).getCellData();
    const DataPath newCellDataPath = m_InputValues->NewGeometryPath.createChildPath(newCellData->getName());
    ParallelTaskAlgorithm taskRunner;
    for(const auto& [dataId, dataObject] : *newCellData)
    {
      if(getCancel())
      {
        return {};
      }
      const std::string name = dataObject->getName();
      const auto* inputDataArray = m_DataStructure.getDataAs<IDataArray>(inputCellDataPath.createChildPath(name));
      const auto* destDataArray = m_DataStructure.getDataAs<IDataArray>(destCellDataPath.createChildPath(name));
      auto* newDataArray = m_DataStructure.getDataAs<IDataArray>(newCellDataPath.createChildPath(name));
      if(destDataArray == nullptr || newDataArray == nullptr || inputDataArray == nullptr)
      {
        continue;
      }
      m_MessageHandler(fmt::format("Combining data into array {}", newCellDataPath.createChildPath(name).toString()));
      ExecuteParallelFunction<CombineImageGeomDataArray>(newDataArray->getDataType(), taskRunner, destDataArray, inputDataArray, newDataArray);
    }
    taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  }
  else
  {
    // We are only appending in the Z direction
    SizeVec3 inputGeomDims = inputGeometry.getDimensions();
    SizeVec3 destGeomDims = destGeometry.getDimensions();
    destGeomDims[2] = destGeomDims[2] + inputGeomDims[2];
    destGeometry.setDimensions(destGeomDims);
    const std::vector<size_t> newDims = {destGeomDims[2], destGeomDims[1], destGeomDims[0]};
    ResizeAttributeMatrix(*destCellData, newDims);
    usize tupleOffset = destGeometry.getNumberOfCells();
    if(tupleOffset > destCellData->getNumTuples())
    {
      return MakeErrorResult(-8206,
                             fmt::format("Calculated tuple offset ({}) for appending the input data is larger than the total number of tuples ({}).", tupleOffset, destCellData->getNumTuples()));
    }

    ParallelTaskAlgorithm taskRunner;
    for(const auto& [dataId, dataObject] : *destCellData)
    {
      if(getCancel())
      {
        return {};
      }

      // Types/bounds checking
      const std::string name = dataObject->getName();
      auto* destDataArray = m_DataStructure.getDataAs<IDataArray>(destCellDataPath.createChildPath(name));
      if(destDataArray == nullptr)
      {
        results =
            MergeResults(results, MakeWarningVoidResult(-8207, fmt::format("Cannot append data to destination data object {} in cell data attribute matrix at path '{}' because it is not of type "
                                                                           "IDataArray. The resulting data object will likely contain invalid data!",
                                                                           name, destCellDataPath.toString())));
        continue;
      }
      if(m_DataStructure.getData(inputCellDataPath.createChildPath(name)) == nullptr)
      {
        results = MergeResults(
            results,
            MakeWarningVoidResult(
                -8208, fmt::format("Data object {} does not exist in the input geometry cell data attribute matrix. Cannot append data so the resulting data object will likely contain invalid data!",
                                   name)));
        continue;
      }
      const auto* inputDataArray = m_DataStructure.getDataAs<IDataArray>(inputCellDataPath.createChildPath(name));
      if(inputDataArray == nullptr)
      {
        results = MergeResults(
            results, MakeWarningVoidResult(
                         -8209, fmt::format("Cannot append data from input data object {} because it is not of type IDataArray. The resulting data object will likely contain invalid data!", name)));
        continue;
      }
      const DataType inputDataType = inputDataArray->getDataType();
      const DataType destDataType = destDataArray->getDataType();
      if(inputDataType != destDataType)
      {
        results = MergeResults(results, MakeWarningVoidResult(-8210, fmt::format("Cannot append data from input data array with type {} to destination data array with type {} because the data array "
                                                                                 "types do not match. The resulting data object will likely contain invalid data!",
                                                                                 DataTypeToString(inputDataType).str(), DataTypeToString(destDataType).str())));
        continue;
      }
      const usize srcNumComps = inputDataArray->getNumberOfComponents();
      const usize numComps = destDataArray->getNumberOfComponents();
      if(srcNumComps != numComps)
      {
        results = MergeResults(
            results,
            MakeWarningVoidResult(
                -8211,
                fmt::format("Cannot append data from input data array with {} components to destination data array with {} components. The resulting data object will likely contain invalid data!",
                            srcNumComps, numComps)));
        continue;
      }
      const usize srcNumElements = inputDataArray->getSize();
      const usize numElements = destDataArray->getSize();
      if(srcNumElements + tupleOffset * numComps > numElements)
      {
        results = MergeResults(
            results, MakeWarningVoidResult(-8212, fmt::format("Cannot append data from input data array with {} total elements to destination data array with {} total elements starting at tuple {} "
                                                              "because there are not enough elements in the destination array. The resulting data object will likely contain invalid data!",
                                                              srcNumElements, numElements, tupleOffset)));
        continue;
      }

      m_MessageHandler(fmt::format("Appending Data Array {}", inputCellDataPath.createChildPath(name).toString()));
      ExecuteParallelFunction<AppendImageGeomDataArray>(destDataType, taskRunner, inputDataArray, destDataArray, tupleOffset);
    }
    taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  }

  return results;
}
