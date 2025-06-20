/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
/*=========================================================================
 *
 *  Portions of this file are subject to the VTK Toolkit Version 3 copyright.
 *
 *  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 *
 *  For complete copyright, license and disclaimer of warranty information
 *  please refer to the NOTICE file at the top of the ITK source tree.
 *
 *=========================================================================*/
#ifndef itkBSplineInterpolateImageFunction_hxx
#define itkBSplineInterpolateImageFunction_hxx

#include "itkImageLinearIteratorWithIndex.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkImageRegionIterator.h"

#include "itkVector.h"

#include "itkMatrix.h"
#include "itkPrintHelper.h"

namespace itk
{

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::BSplineInterpolateImageFunction()
  : m_Coefficients(CoefficientImageType::New())
  , m_CoefficientFilter(CoefficientFilter::New())
  , m_NumberOfWorkUnits(1)
{
  constexpr unsigned int SplineOrder = 3;
  this->SetSplineOrder(SplineOrder);
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::PrintSelf(std::ostream & os,
                                                                                      Indent         indent) const
{
  using namespace print_helper;

  Superclass::PrintSelf(os, indent);

  os << indent << "Scratch: " << m_Scratch << std::endl;
  os << indent
     << "DataLength: " << static_cast<typename NumericTraits<typename TImageType::SizeType>::PrintType>(m_DataLength)
     << std::endl;
  os << indent << "SplineOrder: " << m_SplineOrder << std::endl;

  itkPrintSelfObjectMacro(Coefficients);

  os << indent << "MaxNumberInterpolationPoints: " << m_MaxNumberInterpolationPoints << std::endl;
  os << indent << "PointsToIndex: " << m_PointsToIndex << std::endl;

  itkPrintSelfObjectMacro(CoefficientFilter);

  itkPrintSelfBooleanMacro(UseImageDirection);

  os << indent
     << "NumberOfWorkUnits: " << static_cast<typename NumericTraits<ThreadIdType>::PrintType>(m_NumberOfWorkUnits)
     << std::endl;

  os << indent << "ThreadedEvaluateIndex: ";
  if (m_ThreadedEvaluateIndex != nullptr)
  {
    os << m_ThreadedEvaluateIndex.get() << std::endl;
  }
  else
  {
    os << "(null)" << std::endl;
  }

  os << indent << "ThreadedWeights: ";
  if (m_ThreadedWeights != nullptr)
  {
    os << m_ThreadedWeights.get() << std::endl;
  }
  else
  {
    os << "(null)" << std::endl;
  }

  os << indent << "ThreadedWeightsDerivative: ";
  if (m_ThreadedWeightsDerivative != nullptr)
  {
    os << m_ThreadedWeightsDerivative.get() << std::endl;
  }
  else
  {
    os << "(null)" << std::endl;
  }
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::SetInputImage(const TImageType * inputData)
{
  if (inputData)
  {
    m_CoefficientFilter->SetInput(inputData);

    m_CoefficientFilter->Update();
    m_Coefficients = m_CoefficientFilter->GetOutput();

    // Call the Superclass implementation after, in case the filter
    // pulls in  more of the input image
    Superclass::SetInputImage(inputData);

    m_DataLength = inputData->GetBufferedRegion().GetSize();
  }
  else
  {
    m_Coefficients = nullptr;
  }
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::SetSplineOrder(unsigned int SplineOrder)
{
  if (SplineOrder == m_SplineOrder)
  {
    return;
  }
  m_SplineOrder = SplineOrder;
  m_CoefficientFilter->SetSplineOrder(SplineOrder);

  // this->SetPoles();
  m_MaxNumberInterpolationPoints = 1;
  for (unsigned int n = 0; n < ImageDimension; ++n)
  {
    m_MaxNumberInterpolationPoints *= (m_SplineOrder + 1);
  }
  this->GeneratePointsToIndex();
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::SetNumberOfWorkUnits(
  ThreadIdType numWorkUnits)
{
  m_NumberOfWorkUnits = numWorkUnits;
  this->GeneratePointsToIndex();
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::SetInterpolationWeights(
  const ContinuousIndexType & x,
  const vnl_matrix<long> &    EvaluateIndex,
  vnl_matrix<double> &        weights,
  unsigned int                splineOrder) const
{
  // For speed improvements we could make each case a separate function and use
  // function pointers to reference the correct weight order.
  // Left as is for now for readability.
  switch (splineOrder)
  {
    case 3:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        const double w = x[n] - static_cast<double>(EvaluateIndex[n][1]);
        weights[n][3] = (1.0 / 6.0) * w * w * w;
        weights[n][0] = (1.0 / 6.0) + 0.5 * w * (w - 1.0) - weights[n][3];
        weights[n][2] = w + weights[n][0] - 2.0 * weights[n][3];
        weights[n][1] = 1.0 - weights[n][0] - weights[n][2] - weights[n][3];
      }
      break;
    }
    case 0:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        weights[n][0] = 1; // implements nearest neighbor
      }
      break;
    }
    case 1:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        const double w = x[n] - static_cast<double>(EvaluateIndex[n][0]);
        weights[n][1] = w;
        weights[n][0] = 1.0 - w;
      }
      break;
    }
    case 2:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        /* x */
        const double w = x[n] - static_cast<double>(EvaluateIndex[n][1]);
        weights[n][1] = 0.75 - w * w;
        weights[n][2] = 0.5 * (w - weights[n][1] + 1.0);
        weights[n][0] = 1.0 - weights[n][1] - weights[n][2];
      }
      break;
    }
    case 4:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        /* x */
        const double w = x[n] - static_cast<double>(EvaluateIndex[n][2]);
        const double w2 = w * w;
        const double t = (1.0 / 6.0) * w2;
        weights[n][0] = 0.5 - w;
        weights[n][0] *= weights[n][0];
        weights[n][0] *= (1.0 / 24.0) * weights[n][0];
        const double t0 = w * (t - 11.0 / 24.0);
        const double t1 = 19.0 / 96.0 + w2 * (0.25 - t);
        weights[n][1] = t1 + t0;
        weights[n][3] = t1 - t0;
        weights[n][4] = weights[n][0] + t0 + 0.5 * w;
        weights[n][2] = 1.0 - weights[n][0] - weights[n][1] - weights[n][3] - weights[n][4];
      }
      break;
    }
    case 5:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        /* x */
        double w = x[n] - static_cast<double>(EvaluateIndex[n][2]);
        double w2 = w * w;
        weights[n][5] = (1.0 / 120.0) * w * w2 * w2;
        w2 -= w;
        const double w4 = w2 * w2;
        w -= 0.5;
        const double t = w2 * (w2 - 3.0);
        weights[n][0] = (1.0 / 24.0) * (1.0 / 5.0 + w2 + w4) - weights[n][5];
        double t0 = (1.0 / 24.0) * (w2 * (w2 - 5.0) + 46.0 / 5.0);
        double t1 = (-1.0 / 12.0) * w * (t + 4.0);
        weights[n][2] = t0 + t1;
        weights[n][3] = t0 - t1;
        t0 = (1.0 / 16.0) * (9.0 / 5.0 - t);
        t1 = (1.0 / 24.0) * w * (w4 - w2 - 5.0);
        weights[n][1] = t0 + t1;
        weights[n][4] = t0 - t1;
      }
      break;
    }
    default:
    {
      // SplineOrder not implemented yet.
      ExceptionObject err(__FILE__, __LINE__);
      err.SetLocation(ITK_LOCATION);
      err.SetDescription("SplineOrder must be between 0 and 5. Requested spline order has not been implemented yet.");
      throw err;
    }
  }
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::SetDerivativeWeights(
  const ContinuousIndexType & x,
  const vnl_matrix<long> &    EvaluateIndex,
  vnl_matrix<double> &        weights,
  unsigned int                splineOrder) const
{
  // For speed improvements we could make each case a separate function and use
  // function pointers to reference the correct weight order.
  // Another possibility would be to loop inside the case statement (reducing
  // the number
  // of switch statement executions to one per routine call.
  // Left as is for now for readability.
  const int derivativeSplineOrder = static_cast<int>(splineOrder) - 1;

  switch (derivativeSplineOrder)
  {
    // Calculates B(splineOrder) ( (x + 1/2) - xi) -
    //            B(splineOrder -1)( (x - 1/2) - xi)
    case -1:
    {
      // Why would we want to do this?
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        weights[n][0] = 0.0;
      }
      break;
    }
    case 0:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        weights[n][0] = -1.0;
        weights[n][1] = 1.0;
      }
      break;
    }
    case 1:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        const double w = x[n] + 0.5 - static_cast<double>(EvaluateIndex[n][1]);
        // w2 = w;
        const double w1 = 1.0 - w;

        weights[n][0] = 0.0 - w1;
        weights[n][1] = w1 - w;
        weights[n][2] = w;
      }
      break;
    }
    case 2:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        const double w = x[n] + .5 - static_cast<double>(EvaluateIndex[n][2]);
        const double w2 = 0.75 - w * w;
        const double w3 = 0.5 * (w - w2 + 1.0);
        const double w1 = 1.0 - w2 - w3;

        weights[n][0] = 0.0 - w1;
        weights[n][1] = w1 - w2;
        weights[n][2] = w2 - w3;
        weights[n][3] = w3;
      }
      break;
    }
    case 3:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        const double w = x[n] + 0.5 - static_cast<double>(EvaluateIndex[n][2]);
        const double w4 = (1.0 / 6.0) * w * w * w;
        const double w1 = (1.0 / 6.0) + 0.5 * w * (w - 1.0) - w4;
        const double w3 = w + w1 - 2.0 * w4;
        const double w2 = 1.0 - w1 - w3 - w4;

        weights[n][0] = 0.0 - w1;
        weights[n][1] = w1 - w2;
        weights[n][2] = w2 - w3;
        weights[n][3] = w3 - w4;
        weights[n][4] = w4;
      }
      break;
    }
    case 4:
    {
      for (unsigned int n = 0; n < ImageDimension; ++n)
      {
        const double w = x[n] + .5 - static_cast<double>(EvaluateIndex[n][3]);
        const double t2 = w * w;
        const double t = (1.0 / 6.0) * t2;
        double       w1 = 0.5 - w;
        w1 *= w1;
        w1 *= (1.0 / 24.0) * w1;
        const double t0 = w * (t - 11.0 / 24.0);
        const double t1 = 19.0 / 96.0 + t2 * (0.25 - t);
        const double w2 = t1 + t0;
        const double w4 = t1 - t0;
        const double w5 = w1 + t0 + 0.5 * w;
        const double w3 = 1.0 - w1 - w2 - w4 - w5;

        weights[n][0] = 0.0 - w1;
        weights[n][1] = w1 - w2;
        weights[n][2] = w2 - w3;
        weights[n][3] = w3 - w4;
        weights[n][4] = w4 - w5;
        weights[n][5] = w5;
      }
      break;
    }
    default:
    {
      // SplineOrder not implemented yet.
      ExceptionObject err(__FILE__, __LINE__);
      err.SetLocation(ITK_LOCATION);
      err.SetDescription(
        "SplineOrder (for derivatives) must be between 1 and 5. Requested spline order has not been implemented yet.");
      throw err;
    }
  }
}

// Generates m_PointsToIndex;
template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::GeneratePointsToIndex()
{
  // m_PointsToIndex is used to convert a sequential location to an N-dimension
  // index vector.  This is precomputed to save time during the interpolation
  // routine.
  m_ThreadedEvaluateIndex = std::make_unique<vnl_matrix<long>[]>(m_NumberOfWorkUnits);
  m_ThreadedWeights = std::make_unique<vnl_matrix<double>[]>(m_NumberOfWorkUnits);
  m_ThreadedWeightsDerivative = std::make_unique<vnl_matrix<double>[]>(m_NumberOfWorkUnits);
  for (unsigned int i = 0; i < m_NumberOfWorkUnits; ++i)
  {
    m_ThreadedEvaluateIndex[i].set_size(ImageDimension, m_SplineOrder + 1);
    m_ThreadedWeights[i].set_size(ImageDimension, m_SplineOrder + 1);
    m_ThreadedWeightsDerivative[i].set_size(ImageDimension, m_SplineOrder + 1);
  }

  m_PointsToIndex.resize(m_MaxNumberInterpolationPoints);
  for (unsigned int p = 0; p < m_MaxNumberInterpolationPoints; ++p)
  {
    int           pp = p;
    unsigned long indexFactor[ImageDimension];
    indexFactor[0] = 1;
    for (int j = 1; j < int{ ImageDimension }; ++j)
    {
      indexFactor[j] = indexFactor[j - 1] * (m_SplineOrder + 1);
    }
    for (int j = (int{ ImageDimension } - 1); j >= 0; j--)
    {
      m_PointsToIndex[p][j] = pp / indexFactor[j];
      pp = pp % indexFactor[j];
    }
  }
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::DetermineRegionOfSupport(
  vnl_matrix<long> &          evaluateIndex,
  const ContinuousIndexType & x,
  unsigned int                splineOrder) const
{
  const float halfOffset = splineOrder & 1 ? 0.0 : 0.5;
  for (unsigned int n = 0; n < ImageDimension; ++n)
  {
    long indx = static_cast<long>(std::floor(static_cast<float>(x[n]) + halfOffset)) - splineOrder / 2;
    for (unsigned int k = 0; k <= splineOrder; ++k)
    {
      evaluateIndex[n][k] = indx++;
    }
  }
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::ApplyMirrorBoundaryConditions(
  vnl_matrix<long> & evaluateIndex,
  unsigned int       splineOrder) const
{
  const IndexType startIndex = this->GetStartIndex();
  const IndexType endIndex = this->GetEndIndex();

  for (unsigned int n = 0; n < ImageDimension; ++n)
  {
    // apply the mirror boundary conditions
    // TODO:  We could implement other boundary options beside mirror
    if (m_DataLength[n] == 1)
    {
      for (unsigned int k = 0; k <= splineOrder; ++k)
      {
        evaluateIndex[n][k] = 0;
      }
    }
    else
    {
      for (unsigned int k = 0; k <= splineOrder; ++k)
      {
        if (evaluateIndex[n][k] < startIndex[n])
        {
          evaluateIndex[n][k] = startIndex[n] + (startIndex[n] - evaluateIndex[n][k]);
        }
        if (evaluateIndex[n][k] >= endIndex[n])
        {
          evaluateIndex[n][k] = endIndex[n] - (evaluateIndex[n][k] - endIndex[n]);
        }
      }
    }
  }
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
auto
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::EvaluateAtContinuousIndexInternal(
  const ContinuousIndexType & x,
  vnl_matrix<long> &          evaluateIndex,
  vnl_matrix<double> &        weights) const -> OutputType
{
  // compute the interpolation indexes
  this->DetermineRegionOfSupport((evaluateIndex), x, m_SplineOrder);

  // Determine weights
  SetInterpolationWeights(x, (evaluateIndex), (weights), m_SplineOrder);

  // Modify evaluateIndex at the boundaries using mirror boundary conditions
  this->ApplyMirrorBoundaryConditions((evaluateIndex), m_SplineOrder);

  // perform interpolation
  double    interpolated = 0.0;
  IndexType coefficientIndex;
  // Step through each point in the n-dimensional interpolation cube.
  for (unsigned int p = 0; p < m_MaxNumberInterpolationPoints; ++p)
  {
    double w = 1.0;
    for (unsigned int n = 0; n < ImageDimension; ++n)
    {
      const unsigned int indx = m_PointsToIndex[p][n];
      w *= (weights)[n][indx];
      coefficientIndex[n] = (evaluateIndex)[n][indx];
    }
    interpolated += w * m_Coefficients->GetPixel(coefficientIndex);
  }

  return (interpolated);
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
void
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::
  EvaluateValueAndDerivativeAtContinuousIndexInternal(const ContinuousIndexType & x,
                                                      OutputType &                value,
                                                      CovariantVectorType &       derivativeValue,
                                                      vnl_matrix<long> &          evaluateIndex,
                                                      vnl_matrix<double> &        weights,
                                                      vnl_matrix<double> &        weightsDerivative) const
{
  this->DetermineRegionOfSupport((evaluateIndex), x, m_SplineOrder);

  SetInterpolationWeights(x, (evaluateIndex), (weights), m_SplineOrder);

  SetDerivativeWeights(x, (evaluateIndex), (weightsDerivative), m_SplineOrder);

  // Modify EvaluateIndex at the boundaries using mirror boundary conditions
  this->ApplyMirrorBoundaryConditions((evaluateIndex), m_SplineOrder);

  value = 0.0;
  derivativeValue[0] = 0.0;
  IndexType coefficientIndex;
  for (unsigned int p = 0; p < m_MaxNumberInterpolationPoints; ++p)
  {
    unsigned int indx = m_PointsToIndex[p][0];
    coefficientIndex[0] = (evaluateIndex)[0][indx];
    double w = (weights)[0][indx];
    double w1 = (weightsDerivative)[0][indx];
    for (unsigned int n = 1; n < ImageDimension; ++n)
    {
      indx = m_PointsToIndex[p][n];
      coefficientIndex[n] = (evaluateIndex)[n][indx];
      const double tmpW = (weights)[n][indx];
      w *= tmpW;
      w1 *= tmpW;
    }
    const double tmpV = m_Coefficients->GetPixel(coefficientIndex);
    value += w * tmpV;
    derivativeValue[0] += w1 * tmpV;
  }
  derivativeValue[0] /= this->GetInputImage()->GetSpacing()[0];
  for (unsigned int n = 1; n < ImageDimension; ++n)
  {
    derivativeValue[n] = 0.0;
    for (unsigned int p = 0; p < m_MaxNumberInterpolationPoints; ++p)
    {
      double w1 = 1.0;
      for (unsigned int n1 = 0; n1 < ImageDimension; ++n1)
      {
        const unsigned int indx = m_PointsToIndex[p][n1];
        coefficientIndex[n1] = (evaluateIndex)[n1][indx];

        if (n1 == n)
        {
          w1 *= (weightsDerivative)[n1][indx];
        }
        else
        {
          w1 *= (weights)[n1][indx];
        }
      }
      derivativeValue[n] += m_Coefficients->GetPixel(coefficientIndex) * w1;
    }
    // take spacing into account
    derivativeValue[n] /= this->GetInputImage()->GetSpacing()[n];
  }

  if (this->m_UseImageDirection)
  {
    derivativeValue = this->GetInputImage()->TransformLocalVectorToPhysicalVector(derivativeValue);
  }
}

template <typename TImageType, typename TCoordinate, typename TCoefficientType>
auto
BSplineInterpolateImageFunction<TImageType, TCoordinate, TCoefficientType>::EvaluateDerivativeAtContinuousIndexInternal(
  const ContinuousIndexType & x,
  vnl_matrix<long> &          evaluateIndex,
  vnl_matrix<double> &        weights,
  vnl_matrix<double> &        weightsDerivative) const -> CovariantVectorType
{
  this->DetermineRegionOfSupport((evaluateIndex), x, m_SplineOrder);

  SetInterpolationWeights(x, (evaluateIndex), (weights), m_SplineOrder);

  SetDerivativeWeights(x, (evaluateIndex), (weightsDerivative), m_SplineOrder);

  // Modify EvaluateIndex at the boundaries using mirror boundary conditions
  this->ApplyMirrorBoundaryConditions((evaluateIndex), m_SplineOrder);

  const InputImageType *                       inputImage = this->GetInputImage();
  const typename InputImageType::SpacingType & spacing = inputImage->GetSpacing();

  // Calculate derivative
  CovariantVectorType derivativeValue;

  IndexType coefficientIndex;
  for (unsigned int n = 0; n < ImageDimension; ++n)
  {
    derivativeValue[n] = 0.0;
    for (unsigned int p = 0; p < m_MaxNumberInterpolationPoints; ++p)
    {
      double tempValue = 1.0;
      for (unsigned int n1 = 0; n1 < ImageDimension; ++n1)
      {
        const unsigned int indx = m_PointsToIndex[p][n1];
        coefficientIndex[n1] = (evaluateIndex)[n1][indx];

        if (n1 == n)
        {
          tempValue *= (weightsDerivative)[n1][indx];
        }
        else
        {
          tempValue *= (weights)[n1][indx];
        }
      }
      derivativeValue[n] += m_Coefficients->GetPixel(coefficientIndex) * tempValue;
    }
    derivativeValue[n] /= spacing[n];
  }

  if (this->m_UseImageDirection)
  {
    return inputImage->TransformLocalVectorToPhysicalVector(derivativeValue);
  }

  return (derivativeValue);
}
} // namespace itk

#endif
