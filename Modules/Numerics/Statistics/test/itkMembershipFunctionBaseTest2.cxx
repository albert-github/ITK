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

#include <iostream>
#include "itkMembershipFunctionBase.h"

namespace itk::Statistics::MembershipFunctionBaseTest
{

template <typename TMeasurementVector>
class MyMembershipFunctionBase : public MembershipFunctionBase<TMeasurementVector>
{
public:
  /** Standard class type alias. */
  using Self = MyMembershipFunctionBase;

  using Superclass = MembershipFunctionBase<TMeasurementVector>;

  using Pointer = SmartPointer<Self>;

  using ConstPointer = SmartPointer<const Self>;

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(MyMembershipFunctionBase);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Evaluate membership score */
  double
  Evaluate(const TMeasurementVector &) const override
  {
    constexpr double score = 1;
    return score;
  }
};

} // namespace itk::Statistics::MembershipFunctionBaseTest


/* Test MembershipFunctionBase using a resizable vector type */
int
itkMembershipFunctionBaseTest2(int, char *[])
{

  constexpr unsigned int MeasurementVectorSize = 3;

  using MeasurementVectorType = itk::Array<float>;

  using MembershipFunctionBaseType =
    itk::Statistics::MembershipFunctionBaseTest::MyMembershipFunctionBase<MeasurementVectorType>;

  auto function = MembershipFunctionBaseType::New();

  std::cout << function->GetNameOfClass() << std::endl;
  std::cout << function->MembershipFunctionBaseType::Superclass::GetNameOfClass() << std::endl;

  function->Print(std::cout);

  function->SetMeasurementVectorSize(MeasurementVectorSize);
  if (function->GetMeasurementVectorSize() != MeasurementVectorSize)
  {
    std::cerr << "Set/GetMeasurementVectorSize() Failed !" << std::endl;
    return EXIT_FAILURE;
  }

  // Test if it is possible to reset the vector size
  function->SetMeasurementVectorSize(MeasurementVectorSize);
  if (function->GetMeasurementVectorSize() != MeasurementVectorSize)
  {
    std::cerr << "Set/GetMeasurementVectorSize() Failed !" << std::endl;
    return EXIT_FAILURE;
  }

  // test if it is possible to change the measurement vector size
  constexpr MembershipFunctionBaseType::MeasurementVectorSizeType newSize = 5;
  function->SetMeasurementVectorSize(newSize); // for code coverage

  if (function->GetMeasurementVectorSize() != newSize)
  {
    std::cerr << "Set/GetMeasurementVectorSize() Failed !" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
