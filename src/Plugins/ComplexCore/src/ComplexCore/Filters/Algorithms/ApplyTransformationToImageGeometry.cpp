
#include "ApplyTransformationToImageGeometry.hpp"

#include "complex/Common/ComplexRange3D.hpp"

#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelData3DAlgorithm.hpp"



#include <Eigen/Dense>

#include <fmt/format.h>

#include <cstdint>
#include <map>

using namespace complex;

using Matrix3fR = Eigen::Matrix<float32, 3, 3, Eigen::RowMajor>;
using Matrix4fR = Eigen::Matrix<float32, 4, 4, Eigen::RowMajor>;

namespace
{
template <class T>
void linearEquivalent(T& linEquivalent, const DataArray<T>& lin, int64_t linIntIndexes, double xt, double yt, double zt, RotateArgs params)
{
  int index0 = linIntIndexes;
  int index1 = linIntIndexes + 1;
  int index2 = linIntIndexes + params.xp;
  int index3 = linIntIndexes + 1 + params.xp;
  int index4 = linIntIndexes + params.xp * params.yp;
  int index5 = linIntIndexes + 1 + params.xp * params.yp;
  int index6 = linIntIndexes + params.xp + params.xp * params.yp;
  int index7 = linIntIndexes + 1 + params.xp + params.xp * params.yp;
  if(index0 >= 0 && index0 <= params.xp * params.yp * params.zp)
  {
    linEquivalent += lin[index0];
  }
  if(index1 >= 0 && index1 <= params.xp * params.yp * params.zp)
  {
    linEquivalent += ((lin[index1] - lin[index0]) * xt);
  }
  if(index2 >= 0 && index2 <= params.xp * params.yp * params.zp)
  {
    linEquivalent += ((lin[index2] - lin[index0]) * yt);
  }
  if(index3 >= 0 && index3 <= params.xp * params.yp * params.zp)
  {
    linEquivalent += ((lin[index3] - lin[index2] - lin[index1] + lin[index0]) * xt * yt);
  }
  if(index4 >= 0 && index4 <= params.xp * params.yp * params.zp)
  {
    linEquivalent += ((lin[index4] - lin[index0]) * zt);
  }
  if(index5 >= 0 && index5 <= params.xp * params.yp * params.zp)
  {
    linEquivalent += ((lin[index5] - lin[index4] - lin[index1] + lin[index0]) * xt * zt);
  }
  if(index6 >= 0 && index6 <= params.xp * params.yp * params.zp)
  {
    linEquivalent += ((lin[index6] - lin[index4] - lin[index2] + lin[index0]) * yt * zt);
  }
  if(index7 >= 0 && index7 <= params.xp * params.yp * params.zp)
  {
    linEquivalent += ((lin[index7] - lin[index6] - lin[index5] - lin[index3] + lin[index1] + lin[index4] +
                       lin[index2] - lin[index0]) *
                      xt * yt * zt);
  }
}

template <class T>
void linearEquivalentRGB(T linEquivalent[3], const DataArray<T>& lin, int64_t linIntIndexes, double xt, double yt, double zt, RotateArgs params)
{
  int index0 = linIntIndexes;
  int index1 = linIntIndexes + 1;
  int index2 = linIntIndexes + params.xp;
  int index3 = linIntIndexes + 1 + params.xp;
  int index4 = linIntIndexes + params.xp * params.yp;
  int index5 = linIntIndexes + 1 + params.xp * params.yp;
  int index6 = linIntIndexes + params.xp + params.xp * params.yp;
  int index7 = linIntIndexes + 1 + params.xp + params.xp * params.yp;
  if(index0 >= 0 && index0 <= params.xp * params.yp * params.zp)
  {
    for(int i = 0; i < 3; i++)
    {
      linEquivalent[i] += lin[index0 * 3 + i];
    }
  }
  if(index1 >= 0 && index1 <= params.xp * params.yp * params.zp)
  {
    for(int i = 0; i < 3; i++)
    {
      linEquivalent[i] += ((lin[index1 * 3 + i] - lin[index0 * 3 + i]) * xt);
    }
  }
  if(index2 >= 0 && index2 <= params.xp * params.yp * params.zp)
  {
    for(int i = 0; i < 3; i++)
    {
      linEquivalent[i] += ((lin[index2 * 3 + i] - lin[index0 * 3 + i]) * yt);
    }
  }
  if(index3 >= 0 && index3 <= params.xp * params.yp * params.zp)
  {
    for(int i = 0; i < 3; i++)
    {
      linEquivalent[i] += ((lin[index3 * 3 + i] - lin[index2 * 3 + i] - lin[index1 * 3 + i] + lin[index0 * 3 + i]) * xt * yt);
    }
  }
  if(index4 >= 0 && index4 <= params.xp * params.yp * params.zp)
  {
    for(int i = 0; i < 3; i++)
    {
      linEquivalent[i] += ((lin[index4 * 3 + i] - lin[index0 * 3 + i]) * zt);
    }
  }
  if(index5 >= 0 && index5 <= params.xp * params.yp * params.zp)
  {
    for(int i = 0; i < 3; i++)
    {
      linEquivalent[i] += ((lin[index5 * 3 + i] - lin[index4 * 3 + i] - lin[index1 * 3 + i] + lin[index0 * 3 + i]) * xt * zt);
    }
  }
  if(index6 >= 0 && index6 <= params.xp * params.yp * params.zp)
  {
    for(int i = 0; i < 3; i++)
    {
      linEquivalent[i] += ((lin[index6 * 3 + i] - lin[index4 * 3 + i] - lin[index2 * 3 + i] + lin[index0 * 3 + i]) * yt * zt);
    }
  }
  if(index7 >= 0 && index7 <= params.xp * params.yp * params.zp)
  {
    for(int i = 0; i < 3; i++)
    {
      linEquivalent[i] += ((lin[index7 * 3 + i] - lin[index6 * 3 + i] - lin[index5 * 3 + i] - lin[index3 * 3 + i] + lin[index1 * 3 + i] +
                            lin[index4 * 3 + i] + lin[index2 * 3 + i] - lin[index0 * 3 + i]) *
                           xt * yt * zt);
    }
  }
}

template <class T>
bool linearIndexes(double* LinearInterpolationData, int64_t tupleIndex, T& linEquivalent, const DataArray<T>& lin, RotateArgs params)
{
  bool write = false;
  double xt = LinearInterpolationData[tupleIndex];
  double yt = LinearInterpolationData[tupleIndex + 1];
  double zt = LinearInterpolationData[tupleIndex + 2];
  double colOld = LinearInterpolationData[tupleIndex + 3];
  double rowOld = LinearInterpolationData[tupleIndex + 4];
  double planeOld = LinearInterpolationData[tupleIndex + 5];

  if(colOld >= 0 && colOld < params.xp && colOld >= 0 && colOld < params.xp && rowOld >= 0 && rowOld < params.yp && planeOld >= 0 && planeOld < params.zp)
  {
    int planeFloor = std::floor(planeOld);
    int rowFloor = std::floor(rowOld);
    int colFloor = std::floor(colOld);

    int64_t linIntIndexes = std::nearbyint((params.xp * params.yp * planeFloor) + (params.xp * rowFloor) + colFloor);
    linearEquivalent<T>(linEquivalent, lin, linIntIndexes, xt, yt, zt, params);
    write = true;
  }
  return write;
}

template <class T>
bool linearIndexesRGB(double* LinearInterpolationData, int64_t tupleIndex, T linEquivalent[3], const DataArray<T>& lin, RotateArgs params)
{
  bool write = false;

  double xt = LinearInterpolationData[tupleIndex];
  double yt = LinearInterpolationData[tupleIndex + 1];
  double zt = LinearInterpolationData[tupleIndex + 2];
  double colOld = LinearInterpolationData[tupleIndex + 3];
  double rowOld = LinearInterpolationData[tupleIndex + 4];
  double planeOld = LinearInterpolationData[tupleIndex + 5];

  if(colOld >= 0 && colOld < params.xp && colOld >= 0 && colOld < params.xp && rowOld >= 0 && rowOld < params.yp && planeOld >= 0 && planeOld < params.zp)
  {
    int planeFloor = std::floor(planeOld);
    int rowFloor = std::floor(rowOld);
    int colFloor = std::floor(colOld);

    int64_t linIntIndexes = std::nearbyint((params.xp * params.yp * planeFloor) + (params.xp * rowFloor) + colFloor);
    linearEquivalentRGB<T>(linEquivalent, lin, linIntIndexes, xt, yt, zt, params);
    write = true;
  }
  return write;
}

template <typename T>
void wrapLinearIndexes(double* LinearInterpolationData, int64_t tupleIndex, const DataArray<T>& lin, AbstractDataStore<T>& linData, RotateArgs params)
{
  bool wrote = false;
  int index = tupleIndex / 6;

  T linEquivalent = 0;
  //const IDataArray& linIData = dynamic_cast<IDataArray>(lin);
  wrote = linearIndexes<T>(LinearInterpolationData, tupleIndex, linEquivalent, lin, params);
  if(wrote)
  {
    linData.fillTuple(index, linEquivalent);
  }
  else
  {
    int var = 0;
    linData.fillTuple(index, var);
  }
}

template <typename T>
void wrapLinearIndexesRGB(double* LinearInterpolationData, int64_t tupleIndex, const DataArray<T>& lin, AbstractDataStore<T>& linData, RotateArgs params)
{
  bool wrote = false;
  int index = tupleIndex / 6;
  T linEquivalent[3] = {0, 0, 0};
  //const IDataArray& linIData = dynamic_cast<IDataArray>(lin);
  wrote = linearIndexesRGB<T>(LinearInterpolationData, tupleIndex, linEquivalent, lin, params);
  if(wrote)
  {
    linData[index * 3] = linEquivalent[0];
    linData[index * 3 + 1] = linEquivalent[1];
    linData[index * 3 + 2] = linEquivalent[2];

  }
  else
  {
    int var = 0;
    linData.fillTuple(index, var);
  }
}

template <typename T>
void applyLinearInterpolation(const DataArray<T>& lin, int64_t index, int64_t tupleIndex, nonstd::span<double> LinearInterpolationData, AbstractDataStore<T>& linData, bool RGB, RotateArgs params)
{
  double* linearInterpolationData = &LinearInterpolationData[0];
  if(RGB)
  {
    wrapLinearIndexesRGB<T>(linearInterpolationData, tupleIndex, lin, linData, params);
  }
  else
  {
    wrapLinearIndexes<T>(linearInterpolationData, tupleIndex, lin, linData, params);
  }
}
}
// -----------------------------------------------------------------------------
class SampleRefFrameRotator
{
public:
  SampleRefFrameRotator(nonstd::span<int64> newIndices, nonstd::span<double> newLinearIndices, const RotateArgs& args, const Matrix3fR& rotationMatrix, int interpolationType)
  : m_NewIndices(newIndices)
  , m_NewLinearIndices(newLinearIndices)
  , m_Params(args)
  , m_InterpolationType(interpolationType)
  {
    // We have to inline the 3x3 Matrix transpose here because of the "const" nature of the 'convert' function
    Matrix3fR transpose = rotationMatrix.transpose();
    // Need to use row based Eigen matrix so that the values get mapped to the right place in the raw array
    // Raw array is faster than Eigen
    Eigen::Map<Matrix3fR>(&m_RotMatrixInv[0][0], transpose.rows(), transpose.cols()) = transpose;
  }

  ~SampleRefFrameRotator() = default;

  void convert(int64 zStart, int64 zEnd, int64 yStart, int64 yEnd, int64 xStart, int64 xEnd) const
  {
    for(int64 k = zStart; k < zEnd; k++)
    {
      int64 ktot = (m_Params.xpNew * m_Params.ypNew) * k;
      for(int64 j = yStart; j < yEnd; j++)
      {
        int64 jtot = (m_Params.xpNew) * j;
        for(int64 i = xStart; i < xEnd; i++)
        {
          int64 index = ktot + jtot + i;
          m_NewIndices[index] = -1;
          m_NewLinearIndices[index * 6] = -1;
          m_NewLinearIndices[index * 6 + 1] = -1;
          m_NewLinearIndices[index * 6 + 2] = -1;
          m_NewLinearIndices[index * 6 + 3] = -1;
          m_NewLinearIndices[index * 6 + 4] = -1;
          m_NewLinearIndices[index * 6 + 5] = -1;

          std::array<float32, 3> coords = {0.0f, 0.0f, 0.0f};
          std::array<float32, 3> coordsNew = {0.0f, 0.0f, 0.0f};

          coords[0] = (static_cast<float32>(i) * m_Params.xResNew) + m_Params.xMinNew;
          coords[1] = (static_cast<float32>(j) * m_Params.yResNew) + m_Params.yMinNew;
          coords[2] = (static_cast<float32>(k) * m_Params.zResNew) + m_Params.zMinNew;

          MatrixMath::Multiply3x3with3x1(m_RotMatrixInv, coords.data(), coordsNew.data());

          double x0 = static_cast<float>(std::floor(coordsNew[0] / m_Params.xRes));
          double x1 = static_cast<float>(std::ceil(coordsNew[0] / m_Params.xRes));
          double y0 = static_cast<float>(std::floor(coordsNew[1] / m_Params.yRes));
          double y1 = static_cast<float>(std::ceil(coordsNew[1] / m_Params.yRes));
          double z0 = static_cast<float>(std::floor(coordsNew[2] / m_Params.zRes));
          double z1 = static_cast<float>(std::ceil(coordsNew[2] / m_Params.zRes));

		  if (m_InterpolationType == 0)
		  {
			int64 colOld = static_cast<int64>(std::nearbyint(coordsNew[0] / m_Params.xRes));
			int64 rowOld = static_cast<int64>(std::nearbyint(coordsNew[1] / m_Params.yRes));
			int64 planeOld = static_cast<int64>(std::nearbyint(coordsNew[2] / m_Params.zRes));

			if(colOld >= 0 && colOld < m_Params.xp && rowOld >= 0 && rowOld < m_Params.yp && planeOld >= 0 && planeOld < m_Params.zp)
			{
			  m_NewIndices[index] = (m_Params.xp * m_Params.yp * planeOld) + (m_Params.xp * rowOld) + colOld;
			}
		  }
		  else if(m_InterpolationType == 1)
		  {
	        double colOld = static_cast<double>(coordsNew[0] / m_Params.xRes);
		    double rowOld = static_cast<double>(coordsNew[1] / m_Params.yRes);
			double planeOld = static_cast<double>(coordsNew[2] / m_Params.zRes);

			double xt = 0.5;
			double yt = 0.5;
			double zt = 0.5;

			if(x0 == x1)
			{
			  xt = 0;
			}
			else
			{
			  xt = (colOld - x0) / (x1 - x0);
			}
			
			if(y0 == y1)
			{
			  yt = 0;
			}
			else
			{
			  yt = (rowOld - y0) / (y1 - y0);
			}
			
			if(z0 == z1)
			{
			  zt = 0;
			}
			else
			{
			  zt = (planeOld - z0) / (z1 - z0);
			}

			if(colOld >= 0 && colOld < m_Params.xp && rowOld >= 0 && rowOld < m_Params.yp && planeOld >= 0 && planeOld < m_Params.zp)
			{
			  m_NewLinearIndices[index * 6] = xt;
			  m_NewLinearIndices[index * 6 + 1] = yt;
			  m_NewLinearIndices[index * 6 + 2] = zt;
			  m_NewLinearIndices[index * 6 + 3] = colOld;
			  m_NewLinearIndices[index * 6 + 4] = rowOld;
			  m_NewLinearIndices[index * 6 + 5] = planeOld;
			}
		  }
        }
      }
    }
  }

  void operator()(const ComplexRange3D& range) const
  {
    convert(range[4], range[5], range[2], range[3], range[0], range[1]);
  }

private:
  nonstd::span<int64> m_NewIndices;
  nonstd::span<double> m_NewLinearIndices;
  float32 m_RotMatrixInv[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  RotateArgs m_Params;
  int m_InterpolationType;
};

class ApplyTransformationToImageGeometryImpl
{

public:
  ApplyTransformationToImageGeometryImpl(ApplyTransformationToImageGeometry& filter, const std::vector<float>& transformationMatrix,
                                    const std::atomic_bool& shouldCancel, size_t progIncrement)
  : m_Filter(filter)
  , m_TransformationMatrix(transformationMatrix)
  , m_ShouldCancel(shouldCancel)
  , m_ProgIncrement(progIncrement)
  {
  }
  ~ApplyTransformationToImageGeometryImpl() = default;

  ApplyTransformationToImageGeometryImpl(const ApplyTransformationToImageGeometryImpl&) = default;           // Copy Constructor defaulted
  ApplyTransformationToImageGeometryImpl(ApplyTransformationToImageGeometryImpl&&) = delete;                 // Move Constructor Not Implemented
  ApplyTransformationToImageGeometryImpl& operator=(const ApplyTransformationToImageGeometryImpl&) = delete; // Copy Assignment Not Implemented
  ApplyTransformationToImageGeometryImpl& operator=(ApplyTransformationToImageGeometryImpl&&) = delete;      // Move Assignment Not Implemented

private:
  ApplyTransformationToImageGeometry& m_Filter;
  const std::vector<float>& m_TransformationMatrix;
  const std::atomic_bool& m_ShouldCancel;
  size_t m_ProgIncrement = 0;
};

// -----------------------------------------------------------------------------
struct CopyDataFunctor
{
  template <typename T>
  Result<> operator()(const IDataArray& oldCellArray, IDataArray& newCellArray, nonstd::span<const int64> newIndices, nonstd::span<double> newLinearIndices, int interpolationType, bool RGB, RotateArgs rotateArgs) const
  {
    const auto& oldDataStore = oldCellArray.getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& newDataStore = newCellArray.getIDataStoreRefAs<AbstractDataStore<T>>();
    const auto& typedOldDataArray = dynamic_cast<const DataArray<T>&>(oldCellArray);
    //auto& typedNewDataArray = dynamic_cast<DataArray<T>>(newCellArray);

	if(interpolationType == 0)
	{
      for(usize i = 0; i < newIndices.size(); i++)
      {
        int64 newIndicies_I = newIndices[i];
        if(newIndicies_I >= 0)
        {
          if(!newDataStore.copyFrom(i, oldDataStore, newIndicies_I, 1))
          {
            return MakeErrorResult(-45102, fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", oldCellArray.getName(),
                                                     newIndicies_I, newCellArray.getName(), i));
          }
        }
        else
        {
          newDataStore.fillTuple(i, 0);
        }
	  }
	}
	else if (interpolationType == 1)
	{
	  for (usize i = 0; i < newIndices.size(); i++)
	  {
        int64_t tupleIndex = i * 6;
        int64 newIndicies_I = newLinearIndices[tupleIndex];

		if(newIndicies_I >= 0)
        {
		  applyLinearInterpolation<T>(typedOldDataArray, i, tupleIndex, newLinearIndices, newDataStore, RGB, rotateArgs);
		}
		else
		{
          newDataStore.fillTuple(i, 0);
		}
	  }
	}
	return {};
  }
};

// -----------------------------------------------------------------------------
ApplyTransformationToImageGeometry::ApplyTransformationToImageGeometry(DataStructure& dataStructure, ApplyTransformationToImageGeometryInputValues* inputValues, const std::atomic_bool& shouldCancel,
                                                             const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ApplyTransformationToImageGeometry::~ApplyTransformationToImageGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ApplyImageTransformation(DataStructure& dataStructure, const ApplyTransformationToImageGeometryInputValues* inputValues, bool shouldCancel)
{
  auto imagePath = inputValues->pGeometryToTransform;
  const auto& image = dataStructure.getDataRefAs<ImageGeom>(imagePath);

  int interpolationType = inputValues->pInterpolationType;

  std::vector<float> transformationMatrix = inputValues->pTransformationMatrix;
  Eigen::Map<Matrix4fR> transformation(transformationMatrix.data());

  Eigen::Transform<float, 3, Eigen::Affine> transform = Eigen::Transform<float, 3, Eigen::Affine>::Transform(transformation);
  Matrix3fR rotationMatrix = Matrix3fR::Zero();
  Matrix3fR scaleMatrix = Matrix3fR::Zero();

  transform.computeRotationScaling(&rotationMatrix, &scaleMatrix);

  RotateArgs rotateArgs = inputValues->pRotateArgs;

  int64 newNumCellTuples = rotateArgs.xpNew * rotateArgs.ypNew * rotateArgs.zpNew;
  int64 newNumLinearTuples = newNumCellTuples * 6;
  std::vector<int64> newIndices(newNumCellTuples, -1);
  std::vector<double> newLinearIndices(newNumLinearTuples, -1);

  ParallelData3DAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(ComplexRange3D(0, rotateArgs.xpNew, 0, rotateArgs.ypNew, 0, rotateArgs.zpNew));
  parallelAlgorithm.setParallelizationEnabled(true);
  parallelAlgorithm.execute(SampleRefFrameRotator(newIndices, newLinearIndices, rotateArgs, rotationMatrix, interpolationType));
  auto selectedCellArrays = dataStructure.getAllDataPaths();

  if(inputValues->pUseArraySelector)
  {
    selectedCellArrays = inputValues->pSelectedArrays;
  }

  DataPath createdImage = inputValues->pCreatedImageGeometry;

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    if(shouldCancel)
    {
      return {};
    }

    const auto& oldCellArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
    DataPath createdArrayPath = createdImage.createChildPath(oldCellArray.getName());
    auto& newCellArray = dataStructure.getDataRefAs<IDataArray>(createdArrayPath);
    
	bool RGB = false;
	if(newCellArray.getNumberOfComponents() == 3)
	{
      RGB = true;
	}

    DataType type = oldCellArray.getDataType();

    Result<> result = ExecuteDataFunction(CopyDataFunctor{}, type, oldCellArray, newCellArray, newIndices, newLinearIndices, interpolationType, RGB, rotateArgs);
    if(result.invalid())
    {
      return result;
    }
  }
  return {};
}

Result<> ApplyTransformationToImageGeometry::operator()()
{

  DataObject* dataObject = m_DataStructure.getData(m_InputValues->pGeometryToTransform);

  AbstractGeometry::SharedVertexList* vertexList = nullptr;

  if(dataObject->getDataObjectType() == DataObject::Type::ImageGeom)
  {
    return ApplyImageTransformation(m_DataStructure, m_InputValues, m_ShouldCancel);
  }
  else
  {
    return {MakeErrorResult(-7010, fmt::format("Geometry is not of the proper Type of Image. Type is: '{}", dataObject->getDataObjectType()))};
  }

  return {};
}