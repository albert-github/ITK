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

#include "itkLevelSetContainer.h"
#include "itkLevelSetEquationChanAndVeseInternalTerm.h"
#include "itkLevelSetEquationLaplacianTerm.h"
#include "itkLevelSetEquationTermContainer.h"
#include "itkSinRegularizedHeavisideStepFunction.h"
#include "itkBinaryImageToLevelSetImageAdaptor.h"
#include "itkTestingMacros.h"

int
itkLevelSetEquationTermContainerTest(int argc, char * argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing Arguments" << std::endl;
    std::cerr << "Program " << itkNameOfTestExecutableMacro(argv) << std::endl;
    return EXIT_FAILURE;
  }

  constexpr unsigned int Dimension = 2;

  using InputPixelType = unsigned short;
  using InputImageType = itk::Image<InputPixelType, Dimension>;
  using IdentifierType = itk::IdentifierType;

  using InputPixelType = unsigned short;
  using InputImageType = itk::Image<InputPixelType, Dimension>;

  using PixelType = float;
  using SparseLevelSetType = itk::WhitakerSparseLevelSetImage<PixelType, Dimension>;
  using BinaryToSparseAdaptorType = itk::BinaryImageToLevelSetImageAdaptor<InputImageType, SparseLevelSetType>;

  using LevelSetContainerType = itk::LevelSetContainer<IdentifierType, SparseLevelSetType>;

  using ChanAndVeseInternalTermType =
    itk::LevelSetEquationChanAndVeseInternalTerm<InputImageType, LevelSetContainerType>;

  using IdListType = std::list<IdentifierType>;
  using IdListImageType = itk::Image<IdListType, Dimension>;
  using CacheImageType = itk::Image<short, Dimension>;
  using DomainMapImageFilterType = itk::LevelSetDomainMapImageFilter<IdListImageType, CacheImageType>;

  using LaplacianTermType = itk::LevelSetEquationLaplacianTerm<InputImageType, LevelSetContainerType>;
  using ChanAndVeseInternalTermType =
    itk::LevelSetEquationChanAndVeseInternalTerm<InputImageType, LevelSetContainerType>;
  using TermContainerType = itk::LevelSetEquationTermContainer<InputImageType, LevelSetContainerType>;

  using LevelSetOutputRealType = SparseLevelSetType::OutputRealType;
  using HeavisideFunctionBaseType =
    itk::SinRegularizedHeavisideStepFunction<LevelSetOutputRealType, LevelSetOutputRealType>;
  using InputIteratorType = itk::ImageRegionIteratorWithIndex<InputImageType>;

  // load binary mask
  auto size = InputImageType::SizeType::Filled(50);

  InputImageType::PointType origin;
  origin[0] = 0.0;
  origin[1] = 0.0;

  InputImageType::SpacingType spacing;
  spacing[0] = 1.0;
  spacing[1] = 1.0;

  InputImageType::IndexType index{};

  InputImageType::RegionType region{ index, size };

  // Binary initialization
  auto binary = InputImageType::New();
  binary->SetRegions(region);
  binary->SetSpacing(spacing);
  binary->SetOrigin(origin);
  binary->Allocate();
  binary->FillBuffer(InputPixelType{});

  index.Fill(10);
  size.Fill(30);

  region.SetIndex(index);
  region.SetSize(size);

  InputIteratorType iIt(binary, region);
  iIt.GoToBegin();
  while (!iIt.IsAtEnd())
  {
    iIt.Set(itk::NumericTraits<InputPixelType>::OneValue());
    ++iIt;
  }

  // Convert binary mask to sparse level set
  auto adaptor = BinaryToSparseAdaptorType::New();
  adaptor->SetInputImage(binary);
  adaptor->Initialize();
  std::cout << "Finished converting to sparse format" << std::endl;

  const SparseLevelSetType::Pointer level_set = adaptor->GetModifiableLevelSet();

  IdListType list_ids;
  list_ids.push_back(1);

  auto id_image = IdListImageType::New();
  id_image->SetRegions(binary->GetLargestPossibleRegion());
  id_image->Allocate();
  id_image->FillBuffer(list_ids);

  auto domainMapFilter = DomainMapImageFilterType::New();
  domainMapFilter->SetInput(id_image);
  domainMapFilter->Update();
  std::cout << "Domain map computed" << std::endl;

  // Define the Heaviside function
  auto heaviside = HeavisideFunctionBaseType::New();
  heaviside->SetEpsilon(1.0);

  // Insert the levelsets in a levelset container
  auto lscontainer = LevelSetContainerType::New();
  lscontainer->SetHeaviside(heaviside);
  lscontainer->SetDomainMapFilter(domainMapFilter);

  const bool LevelSetNotYetAdded = lscontainer->AddLevelSet(0, level_set, false);
  if (!LevelSetNotYetAdded)
  {
    return EXIT_FAILURE;
  }

  // Create ChanAndVese External term for phi_{1}
  auto term0 = LaplacianTermType::New();
  term0->SetInput(binary);
  term0->SetCoefficient(1.0);
  std::cout << "Laplacian term created" << std::endl;

  auto term1 = ChanAndVeseInternalTermType::New();
  term1->SetInput(binary);
  term1->SetCoefficient(1.0);
  std::cout << "CV internal term created" << std::endl;

  auto termContainer0 = TermContainerType::New();

  ITK_EXERCISE_BASIC_OBJECT_METHODS(termContainer0, LevelSetEquationTermContainer, Object);


  termContainer0->SetInput(binary);
  ITK_TEST_SET_GET_VALUE(binary, termContainer0->GetInput());

  constexpr typename TermContainerType::LevelSetIdentifierType currentLevelSetId = 0;
  termContainer0->SetCurrentLevelSetId(currentLevelSetId);
  ITK_TEST_SET_GET_VALUE(currentLevelSetId, termContainer0->GetCurrentLevelSetId());

  termContainer0->SetLevelSetContainer(lscontainer);
  ITK_TEST_SET_GET_VALUE(lscontainer, termContainer0->GetLevelSetContainer());

  termContainer0->AddTerm(0, term0);
  termContainer0->AddTerm(1, term1);

  std::cout << "Term container 0 created" << std::endl;

  return EXIT_SUCCESS;
}
