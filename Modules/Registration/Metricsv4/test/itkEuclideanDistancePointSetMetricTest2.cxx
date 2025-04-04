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

#include "itkEuclideanDistancePointSetToPointSetMetricv4.h"
#include "itkTranslationTransform.h"
#include "itkGaussianSmoothingOnUpdateDisplacementFieldTransform.h"
#include "itkTestingMacros.h"

#include <fstream>
#include "itkMath.h"

/*
 * Test with a displacement field transform
 */

template <unsigned int Dimension>
int
itkEuclideanDistancePointSetMetricTest2Run()
{
  using PointSetType = itk::PointSet<unsigned char, Dimension>;

  using PointType = typename PointSetType::PointType;

  auto fixedPoints = PointSetType::New();

  auto movingPoints = PointSetType::New();

  // Create a few points and apply a small offset to make the moving points
  auto      pointMax = static_cast<float>(100.0);
  PointType fixedPoint{};
  fixedPoint[0] = 0.0;
  fixedPoint[1] = 0.0;
  fixedPoints->SetPoint(0, fixedPoint);
  fixedPoint[0] = pointMax;
  fixedPoint[1] = 0.0;
  fixedPoints->SetPoint(1, fixedPoint);
  fixedPoint[0] = pointMax;
  fixedPoint[1] = pointMax;
  fixedPoints->SetPoint(2, fixedPoint);
  fixedPoint[0] = 0.0;
  fixedPoint[1] = pointMax;
  fixedPoints->SetPoint(3, fixedPoint);
  fixedPoint[0] = pointMax / 2.0;
  fixedPoint[1] = pointMax / 2.0;
  fixedPoints->SetPoint(4, fixedPoint);
  if constexpr (Dimension == 3)
  {
    fixedPoint[0] = pointMax / 2.0;
    fixedPoint[1] = pointMax / 2.0;
    fixedPoint[2] = pointMax / 2.0;
    fixedPoints->SetPoint(5, fixedPoint);
    fixedPoint[0] = 0.0;
    fixedPoint[1] = 0.0;
    fixedPoint[2] = pointMax / 2.0;
    fixedPoints->SetPoint(6, fixedPoint);
  }
  const unsigned int numberOfPoints = fixedPoints->GetNumberOfPoints();

  PointType movingPoint;
  for (unsigned int n = 0; n < numberOfPoints; ++n)
  {
    fixedPoint = fixedPoints->GetPoint(n);
    movingPoint[0] = fixedPoint[0] + (n + 1) * 0.5;
    movingPoint[1] = fixedPoint[1] - (n + 2) * 0.5;
    if constexpr (Dimension == 3)
    {
      movingPoint[2] = fixedPoint[2] + (n + 3) * 0.5;
    }
    movingPoints->SetPoint(n, movingPoint);
    // std::cout << fixedPoint << " -> " << movingPoint << std::endl;
  }

  // Test with displacement field transform
  std::cout << "Testing with displacement field transform." << std::endl;
  // using DisplacementFieldTransformType = itk::GaussianSmoothingOnUpdateDisplacementFieldTransform<double, Dimension>;
  using DisplacementFieldTransformType = itk::DisplacementFieldTransform<double, Dimension>;
  auto displacementTransform = DisplacementFieldTransformType::New();

  // Setup the physical space to match the point set virtual domain,
  // which is defined by the fixed point set since the fixed transform
  // is identity.
  using FieldType = typename DisplacementFieldTransformType::DisplacementFieldType;
  using RegionType = typename FieldType::RegionType;

  auto regionSize = RegionType::SizeType::Filled(static_cast<itk::SizeValueType>(pointMax + 1.0));

  const typename RegionType::IndexType regionIndex{};

  const RegionType region{ regionIndex, regionSize };

  auto                              displacementField = FieldType::New();
  typename FieldType::DirectionType direction{};
  for (unsigned int d = 0; d < Dimension; ++d)
  {
    direction[d][d] = 1.0;
  }
  displacementField->SetDirection(direction);
  displacementField->SetRegions(region);
  displacementField->Allocate();
  const typename DisplacementFieldTransformType::OutputVectorType zeroVector{};
  displacementField->FillBuffer(zeroVector);
  displacementTransform->SetDisplacementField(displacementField);

  // Instantiate the metric
  using PointSetMetricType = itk::EuclideanDistancePointSetToPointSetMetricv4<PointSetType>;
  auto metric = PointSetMetricType::New();
  metric->SetFixedPointSet(fixedPoints);
  metric->SetMovingPointSet(movingPoints);
  metric->SetMovingTransform(displacementTransform);
  // If we don't set this explicitly, it will still work because it will be taken from the
  // displacement field during initialization.
  auto                                spacing = itk::MakeFilled<typename FieldType::SpacingType>(1.0);
  const typename FieldType::PointType origin{};
  metric->SetVirtualDomain(spacing, origin, direction, region);

  metric->Initialize();

  // test
  const typename PointSetMetricType::MeasureType value = metric->GetValue();
  typename PointSetMetricType::DerivativeType    derivative;
  metric->GetDerivative(derivative);

  typename PointSetMetricType::MeasureType    value2;
  typename PointSetMetricType::DerivativeType derivative2;
  metric->GetValueAndDerivative(value2, derivative2);

  std::cout << "value: " << value << std::endl;

  // Check for the same results from different methods
  if (itk::Math::NotExactlyEquals(value, value2))
  {
    std::cerr << "value does not match between calls to different methods: "
              << "value: " << value << " value2: " << value2 << std::endl;
  }
  if (derivative != derivative2)
  {
    std::cerr << "derivative does not match between calls to different methods: "
              << "derivative: " << derivative << " derivative2: " << derivative2 << std::endl;
  }

  displacementTransform->UpdateTransformParameters(derivative2);

  // check the results
  bool passed = true;
  for (itk::SizeValueType n = 0; n < fixedPoints->GetNumberOfPoints(); ++n)
  {
    PointType transformedPoint;
    fixedPoint = fixedPoints->GetPoint(n);
    movingPoint = movingPoints->GetPoint(n);
    transformedPoint = displacementTransform->TransformPoint(fixedPoint);
    for (unsigned int d = 0; d < Dimension; ++d)
    {
      if (itk::Math::NotExactlyEquals(transformedPoint[d], movingPoint[d]))
      {
        passed = false;
      }
    }
    std::cout << " fixed, moving, txf'ed, moving-txf: " << fixedPoint << ' ' << movingPoint << ' ' << transformedPoint
              << ' ' << movingPoint - transformedPoint << std::endl;
  }

  if (!passed)
  {
    std::cerr << "Not all points match after transformation with result." << std::endl;
    return EXIT_FAILURE;
  }

  // Test the valid points are counted correctly.
  // We should get a warning printed.
  fixedPoint[0] = 0.0;
  fixedPoint[1] = 2 * pointMax;
  const unsigned int numberExpected = fixedPoints->GetNumberOfPoints() - 1;
  fixedPoints->SetPoint(fixedPoints->GetNumberOfPoints() - 1, fixedPoint);
  metric->GetValueAndDerivative(value2, derivative2);
  if (metric->GetNumberOfValidPoints() != numberExpected)
  {
    std::cerr << "Expected " << numberExpected << " valid points, but got " << metric->GetNumberOfValidPoints()
              << std::endl;
    return EXIT_FAILURE;
  }

  // Test with no valid points.
  auto fixedPoints2 = PointSetType::New();
  fixedPoint[0] = -pointMax;
  fixedPoint[1] = 0.0;
  fixedPoints2->SetPoint(0, fixedPoint);
  metric->SetFixedPointSet(fixedPoints2);
  metric->Initialize();
  metric->GetValueAndDerivative(value2, derivative2);
  bool derivative2IsZero = true;
  for (itk::SizeValueType n = 0; n < metric->GetNumberOfParameters(); ++n)
  {
    if (itk::Math::NotExactlyEquals(derivative2[n], typename PointSetMetricType::DerivativeValueType{}))
    {
      derivative2IsZero = false;
    }
  }
  if (itk::Math::NotExactlyEquals(value2, itk::NumericTraits<typename PointSetMetricType::MeasureType>::max()) ||
      !derivative2IsZero)
  {
    std::cerr << "Failed testing with no valid points. Number of valid points: " << metric->GetNumberOfValidPoints()
              << " value2: " << value2 << " derivative2IsZero: " << derivative2IsZero << std::endl;
    return EXIT_FAILURE;
  }

  // Test with invalid virtual domain, i.e.
  // one that doesn't match the displacement field.
  auto             badSize = RegionType::SizeType::Filled(static_cast<itk::SizeValueType>(pointMax / 2.0));
  const RegionType badRegion{ regionIndex, badSize };
  metric->SetVirtualDomain(spacing, origin, direction, badRegion);
  ITK_TRY_EXPECT_EXCEPTION(metric->Initialize());

  auto badSpacing = itk::MakeFilled<typename FieldType::SpacingType>(0.5);
  metric->SetVirtualDomain(badSpacing, origin, direction, region);
  ITK_TRY_EXPECT_EXCEPTION(metric->Initialize());

  return EXIT_SUCCESS;
}

int
itkEuclideanDistancePointSetMetricTest2(int, char *[])
{
  int result = EXIT_SUCCESS;

  if (itkEuclideanDistancePointSetMetricTest2Run<2>() == EXIT_FAILURE)
  {
    std::cerr << "Failed for Dimension 2." << std::endl;
    result = EXIT_FAILURE;
  }

  if (itkEuclideanDistancePointSetMetricTest2Run<3>() == EXIT_FAILURE)
  {
    std::cerr << "Failed for Dimension 3." << std::endl;
    result = EXIT_FAILURE;
  }

  return result;
}
