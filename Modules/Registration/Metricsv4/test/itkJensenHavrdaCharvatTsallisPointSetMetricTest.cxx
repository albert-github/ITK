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

#include "itkJensenHavrdaCharvatTsallisPointSetToPointSetMetricv4.h"
#include "itkTranslationTransform.h"
#include "itkTestingMacros.h"
#include "itkMakeFilled.h"
#include <fstream>

template <unsigned int Dimension>
int
itkJensenHavrdaCharvatTsallisPointSetMetricTestRun()
{
  using PointSetType = itk::PointSet<unsigned char, Dimension>;

  using PointType = typename PointSetType::PointType;
  using VectorType = typename PointType::VectorType;

  auto fixedPoints = PointSetType::New();

  auto movingPoints = PointSetType::New();

  // Produce two simple point sets of 1) a circle and 2) the same circle with an offset
  auto offset = itk::MakeFilled<PointType>(2);
  auto normalizedOffset = itk::MakeFilled<VectorType>(2);

  float normOffset = 0;
  for (unsigned int d = 0; d < Dimension; ++d)
  {
    normOffset += itk::Math::sqr(offset[d]);
  }
  normOffset = std::sqrt(normOffset);
  normalizedOffset /= normOffset;

  unsigned long count = 0;
  for (float theta = 0; theta < 2.0 * itk::Math::pi; theta += 0.1)
  {
    constexpr float radius = 100.0;
    PointType       fixedPoint;
    fixedPoint[0] = radius * std::cos(theta);
    fixedPoint[1] = radius * std::sin(theta);
    // simplistic point set test:
    //    fixedPoint[0] = 1;
    //    fixedPoint[1] = 1;
    if constexpr (Dimension > 2)
    {
      fixedPoint[2] = radius * std::sin(theta);
      //      fixedPoint[2] = 1;
    }
    fixedPoints->SetPoint(count, fixedPoint);

    PointType movingPoint;
    movingPoint[0] = fixedPoint[0] + offset[0];
    movingPoint[1] = fixedPoint[1] + offset[1];
    if constexpr (Dimension > 2)
    {
      movingPoint[2] = fixedPoint[2] + offset[2];
    }
    movingPoints->SetPoint(count, movingPoint);

    count++;
  }

  // Simple translation transform for moving point set
  using TranslationTransformType = itk::TranslationTransform<double, Dimension>;
  auto translationTransform = TranslationTransformType::New();
  translationTransform->SetIdentity();

  // check various alpha values between accepted values of [1.0, 2.0]

  constexpr unsigned int numberOfAlphaValues = 6;
  constexpr float        alphaValues[] = { 1.0f, 1.2f, 1.4f, 1.6f, 1.8f, 2.0f };

  constexpr unsigned int evaluationKNeighborhood = 50;
  auto                   useAnisotropicCovariances = false;
  constexpr unsigned int covarianceKNeighborhood = 5;

  for (unsigned int i = 0; i < numberOfAlphaValues; ++i)
  {

    std::cout << "Alpha = " << alphaValues[i] << std::endl;

    // Instantiate the metric ( alpha = 1.0 )
    using PointSetMetricType = itk::JensenHavrdaCharvatTsallisPointSetToPointSetMetricv4<PointSetType>;
    auto metric = PointSetMetricType::New();

    ITK_EXERCISE_BASIC_OBJECT_METHODS(
      metric, JensenHavrdaCharvatTsallisPointSetToPointSetMetricv4, PointSetToPointSetMetricv4);


    metric->SetAlpha(alphaValues[i]);
    ITK_TEST_SET_GET_VALUE(alphaValues[i], metric->GetAlpha());

    const typename PointSetMetricType::RealType pointSetSigma = 1.0;
    metric->SetPointSetSigma(pointSetSigma);
    ITK_TEST_SET_GET_VALUE(pointSetSigma, metric->GetPointSetSigma());

    metric->SetEvaluationKNeighborhood(evaluationKNeighborhood);
    ITK_TEST_SET_GET_VALUE(evaluationKNeighborhood, metric->GetEvaluationKNeighborhood());

    ITK_TEST_SET_GET_BOOLEAN(metric, UseAnisotropicCovariances, useAnisotropicCovariances);

    metric->SetCovarianceKNeighborhood(covarianceKNeighborhood);
    ITK_TEST_SET_GET_VALUE(covarianceKNeighborhood, metric->GetCovarianceKNeighborhood());

    const typename PointSetMetricType::RealType kernelSigma = 10.0;
    metric->SetKernelSigma(kernelSigma);
    ITK_TEST_SET_GET_VALUE(kernelSigma, metric->GetKernelSigma());

    metric->SetFixedPointSet(fixedPoints);
    metric->SetMovingPointSet(movingPoints);
    metric->SetMovingTransform(translationTransform);

    metric->Initialize();

    const typename PointSetMetricType::MeasureType value = metric->GetValue();
    typename PointSetMetricType::DerivativeType    derivative;
    metric->GetDerivative(derivative);
    typename PointSetMetricType::MeasureType    value2;
    typename PointSetMetricType::DerivativeType derivative2;
    metric->GetValueAndDerivative(value2, derivative2);

    derivative /= derivative.magnitude();
    derivative2 /= derivative2.magnitude();

    std::cout << "value: " << value << std::endl;
    std::cout << "normalized derivative: " << derivative << std::endl;

    for (unsigned int d = 0; d < metric->GetNumberOfParameters(); ++d)
    {
      if (itk::Math::abs(derivative[d] - normalizedOffset[d]) / normalizedOffset[d] > 0.01)
      {
        std::cerr << "derivative does not match expected normalized offset of " << offset << std::endl;
        return EXIT_FAILURE;
      }
    }

    if constexpr (Dimension == 2)
    {
      static constexpr float metricValues2D[] = { 0.143842f,     -0.0129571f,   -0.00105768f,
                                                  -0.000115118f, -1.40956e-05f, -1.84099e-06f };

      if (itk::Math::abs(value - metricValues2D[i]) > 0.01)
      {
        std::cerr << "calculated value is different than expected." << std::endl;
      }
    }
    else if constexpr (Dimension == 3)
    {
      static constexpr float metricValues3D[] = { 0.175588f,     -0.0086854f,   -0.000475248f,
                                                  -3.46729e-05f, -2.84585e-06f, -2.49151e-07f };

      if (itk::Math::abs(value - metricValues3D[i]) > 0.01)
      {
        std::cerr << "calculated value is different than expected." << std::endl;
      }
    }

    // Check for the same results from different methods
    if (itk::Math::abs(value - value2) > 0.01)
    {
      std::cerr << "value does not match between calls to different methods: "
                << "value: " << value << " value2: " << value2 << std::endl;
    }
    if (derivative != derivative2)
    {
      std::cerr << "derivative does not match between calls to different methods: "
                << "derivative: " << derivative << " derivative2: " << derivative2 << std::endl;
    }

    std::ofstream moving_str1("sourceMoving.txt");
    std::ofstream moving_str2("targetMoving.txt");

    count = 0;

    moving_str1 << "0 0 0 0" << std::endl;
    moving_str2 << "0 0 0 0" << std::endl;

    typename PointType::VectorType vector;
    for (unsigned int d = 0; d < metric->GetNumberOfParameters(); ++d)
    {
      vector[d] = derivative[count++];
    }

    typename PointSetType::PointsContainer::ConstIterator ItM = movingPoints->GetPoints()->Begin();
    while (ItM != movingPoints->GetPoints()->End())
    {
      PointType sourcePoint = ItM.Value();
      PointType targetPoint = sourcePoint + vector;

      for (unsigned int d = 0; d < metric->GetNumberOfParameters(); ++d)
      {
        moving_str1 << sourcePoint[d] << ' ';
        moving_str2 << targetPoint[d] << ' ';
      }
      if constexpr (Dimension < 3)
      {
        moving_str1 << "0 ";
        moving_str2 << "0 ";
      }
      moving_str1 << ItM.Index() << std::endl;
      moving_str2 << ItM.Index() << std::endl;

      ++ItM;
    }

    moving_str1 << "0 0 0 0" << std::endl;
    moving_str2 << "0 0 0 0" << std::endl;
  }

  return EXIT_SUCCESS;
}

int
itkJensenHavrdaCharvatTsallisPointSetMetricTest(int, char *[])
{
  int result = EXIT_SUCCESS;

  if (itkJensenHavrdaCharvatTsallisPointSetMetricTestRun<2>() == EXIT_FAILURE)
  {
    std::cerr << "Failed for Dimension 2." << std::endl;
    result = EXIT_FAILURE;
  }

  if (itkJensenHavrdaCharvatTsallisPointSetMetricTestRun<3>() == EXIT_FAILURE)
  {
    std::cerr << "Failed for Dimension 3." << std::endl;
    result = EXIT_FAILURE;
  }

  return result;
}
