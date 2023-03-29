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
template <class K>
void AppendData(K* inputArray, K* destArray, usize offset)
{
  for(usize i = 0; i < inputArray->getNumberOfTuples(); ++i)
  {
    auto value = (*inputArray)[i]; // make sure we are getting a copy not a ref
    (*destArray)[offset + i] = value;
  }
}

template <typename T>
class AppendImageGeomDataArray
{
public:
  AppendImageGeomDataArray(IArray* inputCellArray, IArray* destCellArray, usize tupleOffset)
  : m_InputCellArray(dynamic_cast<DataArray<T>*>(inputCellArray))
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
    const usize offset = m_TupleOffset * m_DestCellArray->getNumberOfComponents();
    AppendData<DataArray<T>>(m_InputCellArray, m_DestCellArray, offset);
  }

private:
  DataArray<T>* m_InputCellArray = nullptr;
  DataArray<T>* m_DestCellArray = nullptr;
  usize m_TupleOffset;
};

template <typename T>
class AppendImageGeomNeighborListArray
{
public:
  AppendImageGeomNeighborListArray(IArray* inputCellArray, IArray* destCellArray, usize tupleOffset)
  : m_InputCellArray(dynamic_cast<NeighborList<T>*>(inputCellArray))
  , m_DestCellArray(dynamic_cast<NeighborList<T>*>(destCellArray))
  , m_TupleOffset(tupleOffset)
  {
  }

  ~AppendImageGeomNeighborListArray() = default;

  AppendImageGeomNeighborListArray(const AppendImageGeomNeighborListArray&) = default;
  AppendImageGeomNeighborListArray(AppendImageGeomNeighborListArray&&) noexcept = default;
  AppendImageGeomNeighborListArray& operator=(const AppendImageGeomNeighborListArray&) = delete;
  AppendImageGeomNeighborListArray& operator=(AppendImageGeomNeighborListArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    const usize offset = m_TupleOffset * m_DestCellArray->getNumberOfComponents();
    AppendData<NeighborList<T>>(m_InputCellArray, m_DestCellArray, offset);
  }

private:
  NeighborList<T>* m_InputCellArray = nullptr;
  NeighborList<T>* m_DestCellArray = nullptr;
  usize m_TupleOffset;
};

class AppendImageGeomStringArray
{
public:
  AppendImageGeomStringArray(IArray* inputCellArray, IArray* destCellArray, usize tupleOffset)
  : m_InputCellArray(dynamic_cast<StringArray*>(inputCellArray))
  , m_DestCellArray(dynamic_cast<StringArray*>(destCellArray))
  , m_TupleOffset(tupleOffset)
  {
  }

  ~AppendImageGeomStringArray() = default;

  AppendImageGeomStringArray(const AppendImageGeomStringArray&) = default;
  AppendImageGeomStringArray(AppendImageGeomStringArray&&) noexcept = default;
  AppendImageGeomStringArray& operator=(const AppendImageGeomStringArray&) = delete;
  AppendImageGeomStringArray& operator=(AppendImageGeomStringArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    const usize offset = m_TupleOffset * m_DestCellArray->getNumberOfComponents();
    AppendData<StringArray>(m_InputCellArray, m_DestCellArray, offset);
  }

private:
  StringArray* m_InputCellArray = nullptr;
  StringArray* m_DestCellArray = nullptr;
  usize m_TupleOffset;
};

template <typename T>
class CombineImageGeomDataArray
{
public:
  CombineImageGeomDataArray(IArray* inputCellArray1, IArray* inputCellArray2, IArray* destCellArray)
  : m_InputCellArray1(dynamic_cast<DataArray<T>*>(inputCellArray1))
  , m_InputCellArray2(dynamic_cast<DataArray<T>*>(inputCellArray2))
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
    AppendData<DataArray<T>>(m_InputCellArray1, m_DestCellArray, 0);
    AppendData<DataArray<T>>(m_InputCellArray2, m_DestCellArray, m_InputCellArray1->getSize());
  }

private:
  DataArray<T>* m_InputCellArray1 = nullptr;
  DataArray<T>* m_InputCellArray2 = nullptr;
  DataArray<T>* m_DestCellArray = nullptr;
};

template <typename T>
class CombineImageGeomNeighborListArray
{
public:
  CombineImageGeomNeighborListArray(IArray* inputCellArray1, IArray* inputCellArray2, IArray* destCellArray)
  : m_InputCellArray1(dynamic_cast<NeighborList<T>*>(inputCellArray1))
  , m_InputCellArray2(dynamic_cast<NeighborList<T>*>(inputCellArray2))
  , m_DestCellArray(dynamic_cast<NeighborList<T>*>(destCellArray))
  {
  }

  ~CombineImageGeomNeighborListArray() = default;

  CombineImageGeomNeighborListArray(const CombineImageGeomNeighborListArray&) = default;
  CombineImageGeomNeighborListArray(CombineImageGeomNeighborListArray&&) noexcept = default;
  CombineImageGeomNeighborListArray& operator=(const CombineImageGeomNeighborListArray&) = delete;
  CombineImageGeomNeighborListArray& operator=(CombineImageGeomNeighborListArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    AppendData<NeighborList<T>>(m_InputCellArray1, m_DestCellArray, 0);
    AppendData<NeighborList<T>>(m_InputCellArray2, m_DestCellArray, m_InputCellArray1->getSize());
  }

private:
  NeighborList<T>* m_InputCellArray1 = nullptr;
  NeighborList<T>* m_InputCellArray2 = nullptr;
  NeighborList<T>* m_DestCellArray = nullptr;
};

class CombineImageGeomStringArray
{
public:
  CombineImageGeomStringArray(IArray* inputCellArray1, IArray* inputCellArray2, IArray* destCellArray)
  : m_InputCellArray1(dynamic_cast<StringArray*>(inputCellArray1))
  , m_InputCellArray2(dynamic_cast<StringArray*>(inputCellArray2))
  , m_DestCellArray(dynamic_cast<StringArray*>(destCellArray))
  {
  }

  ~CombineImageGeomStringArray() = default;

  CombineImageGeomStringArray(const CombineImageGeomStringArray&) = default;
  CombineImageGeomStringArray(CombineImageGeomStringArray&&) noexcept = default;
  CombineImageGeomStringArray& operator=(const CombineImageGeomStringArray&) = delete;
  CombineImageGeomStringArray& operator=(CombineImageGeomStringArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    AppendData<StringArray>(m_InputCellArray1, m_DestCellArray, 0);
    AppendData<StringArray>(m_InputCellArray2, m_DestCellArray, m_InputCellArray1->getSize());
  }

private:
  StringArray* m_InputCellArray1 = nullptr;
  StringArray* m_InputCellArray2 = nullptr;
  StringArray* m_DestCellArray = nullptr;
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
  DataPath newCellDataPath = destCellDataPath;
  AttributeMatrix* newCellData = destCellData;
  SizeVec3 inputGeomDims = inputGeometry.getDimensions();
  SizeVec3 destGeomDims = destGeometry.getDimensions();

  if(m_InputValues->SaveAsNewGeometry)
  {
    newCellData = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->NewGeometryPath).getCellData();
    newCellDataPath = m_InputValues->NewGeometryPath.createChildPath(newCellData->getName());
  }
  else
  {
    // We are only appending in the Z direction
    destGeomDims[2] = destGeomDims[2] + inputGeomDims[2];
    destGeometry.setDimensions(destGeomDims);
    const std::vector<size_t> newDims = {destGeomDims[2], destGeomDims[1], destGeomDims[0]};
    ResizeAttributeMatrix(*destCellData, newDims);
  }
  const usize tupleOffset = destGeometry.getNumberOfCells();

  ParallelTaskAlgorithm taskRunner;
  for(const auto& [dataId, dataObject] : *newCellData)
  {
    if(getCancel())
    {
      return {};
    }

    const std::string name = dataObject->getName();
    if(m_DataStructure.getData(inputCellDataPath.createChildPath(name)) == nullptr)
    {
      results = MergeResults(
          results,
          MakeWarningVoidResult(
              -8213,
              fmt::format("Data object {} does not exist in the input geometry cell data attribute matrix. Cannot append data so the resulting data object will likely contain invalid data!", name)));
      continue;
    }

    auto* inputDataArray = m_DataStructure.getDataAs<IArray>(inputCellDataPath.createChildPath(name));
    auto* destDataArray = m_DataStructure.getDataAs<IArray>(destCellDataPath.createChildPath(name));
    auto* newDataArray = m_DataStructure.getDataAs<IArray>(newCellDataPath.createChildPath(name));
    if(destDataArray == nullptr || newDataArray == nullptr || inputDataArray == nullptr)
    {
      continue;
    }
    const IArray::ArrayType arrayType = destDataArray->getArrayType();

    if(m_InputValues->SaveAsNewGeometry)
    {
      m_MessageHandler(fmt::format("Combining data into array {}", newCellDataPath.createChildPath(name).toString()));
      if(arrayType == IArray::ArrayType::NeighborListArray)
      {
        ExecuteParallelFunction<CombineImageGeomDataArray, NoBooleanType>(dynamic_cast<INeighborList*>(destDataArray)->getDataType(), taskRunner, destDataArray, inputDataArray, newDataArray);
      }
      if(arrayType == IArray::ArrayType::DataArray)
      {
        ExecuteParallelFunction<CombineImageGeomDataArray>(dynamic_cast<IDataArray*>(destDataArray)->getDataType(), taskRunner, destDataArray, inputDataArray, newDataArray);
      }
      if(arrayType == IArray::ArrayType::StringArray)
      {
        taskRunner.execute(CombineImageGeomStringArray(destDataArray, inputDataArray, newDataArray));
      }
    }
    else
    {
      m_MessageHandler(fmt::format("Appending Data Array {}", inputCellDataPath.createChildPath(name).toString()));

      if(arrayType == IArray::ArrayType::NeighborListArray)
      {
        ExecuteParallelFunction<AppendImageGeomNeighborListArray, NoBooleanType>(dynamic_cast<INeighborList*>(destDataArray)->getDataType(), taskRunner, inputDataArray, destDataArray, tupleOffset);
      }
      if(arrayType == IArray::ArrayType::DataArray)
      {
        ExecuteParallelFunction<AppendImageGeomDataArray>(dynamic_cast<IDataArray*>(destDataArray)->getDataType(), taskRunner, inputDataArray, destDataArray, tupleOffset);
      }
      if(arrayType == IArray::ArrayType::StringArray)
      {
        taskRunner.execute(AppendImageGeomStringArray(inputDataArray, destDataArray, tupleOffset));
      }
    }
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return results;
}
