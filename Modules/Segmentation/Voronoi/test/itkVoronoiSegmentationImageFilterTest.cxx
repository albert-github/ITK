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

#include "itkVoronoiSegmentationImageFilter.h"
#include "itkVoronoiSegmentationImageFilterBase.h"
#include "itkTestingMacros.h"

int
itkVoronoiSegmentationImageFilterTest(int argc, char * argv[])
{
  if (argc != 9)
  {
    std::cerr << "Missing Parameters." << std::endl;
    std::cerr << "Usage: " << itkNameOfTestExecutableMacro(argv)
              << " mean std meanTolerance stdTolerance numberOfSeeds steps meanPercentError stdPercentError"
              << std::endl;
    return EXIT_FAILURE;
  }

  constexpr int width = 256;
  constexpr int height = 256;

  using UShortImage = itk::Image<unsigned short, 2>;
  using PriorImage = itk::Image<unsigned char, 2>;
  using VoronoiSegmentationImageFilterType = itk::VoronoiSegmentationImageFilter<UShortImage, UShortImage, PriorImage>;

  auto voronoiSegmenter = VoronoiSegmentationImageFilterType::New();

  ITK_EXERCISE_BASIC_OBJECT_METHODS(
    voronoiSegmenter, VoronoiSegmentationImageFilter, VoronoiSegmentationImageFilterBase);

  auto                            inputImage = UShortImage::New();
  constexpr UShortImage::SizeType size = { { width, height } };
  UShortImage::IndexType          index{};

  UShortImage::RegionType region;

  region.SetSize(size);
  region.SetIndex(index);

  std::cout << "Allocating image" << std::endl;
  inputImage->SetRegions(region);
  inputImage->Allocate();


  itk::ImageRegionIteratorWithIndex<UShortImage> it(inputImage, region);

  // Background: random field with mean: 500, std: 50
  std::cout << "Setting background random pattern image" << std::endl;
  while (!it.IsAtEnd())
  {
    it.Set(static_cast<unsigned short>(vnl_sample_uniform(450, 550)));
    ++it;
  }

  // Object (2): random field with mean: 520, std: 20
  std::cout << "Defining object #2" << std::endl;
  for (unsigned int i = 30; i < 94; ++i)
  {
    index[0] = i;
    for (unsigned int j = 30; j < 94; ++j)
    {
      index[1] = j;
      inputImage->SetPixel(index, static_cast<unsigned short>(vnl_sample_uniform(500, 540)));
    }
  }

  for (unsigned int i = 150; i < 214; ++i)
  {
    index[0] = i;
    for (unsigned int j = 150; j < 214; ++j)
    {
      index[1] = j;
      inputImage->SetPixel(index, static_cast<unsigned short>(vnl_sample_uniform(500, 540)));
    }
  }

  unsigned short TestImg[65536];

  voronoiSegmenter->SetInput(inputImage);

  auto mean = std::stod(argv[1]);
  voronoiSegmenter->SetMean(mean);
  ITK_TEST_SET_GET_VALUE(mean, voronoiSegmenter->GetMean());

  auto std = std::stod(argv[2]);
  voronoiSegmenter->SetSTD(std);
  ITK_TEST_SET_GET_VALUE(std, voronoiSegmenter->GetSTD());

  auto meanTolerance = std::stod(argv[3]);
  voronoiSegmenter->SetMeanTolerance(meanTolerance);
  ITK_TEST_SET_GET_VALUE(meanTolerance, voronoiSegmenter->GetMeanTolerance());

  auto stdTolerance = std::stod(argv[4]);
  voronoiSegmenter->SetSTDTolerance(stdTolerance);
  ITK_TEST_SET_GET_VALUE(stdTolerance, voronoiSegmenter->GetSTDTolerance());

  auto numberOfSeeds = std::stoi(argv[5]);
  voronoiSegmenter->SetNumberOfSeeds(numberOfSeeds);
  ITK_TEST_SET_GET_VALUE(numberOfSeeds, voronoiSegmenter->GetNumberOfSeeds());

  auto steps = std::stoi(argv[6]);
  voronoiSegmenter->SetSteps(steps);
  ITK_TEST_SET_GET_VALUE(steps, voronoiSegmenter->GetSteps());

  auto meanPercentError = std::stod(argv[7]);
  voronoiSegmenter->SetMeanPercentError(meanPercentError);
  ITK_TEST_SET_GET_VALUE(meanPercentError, voronoiSegmenter->GetMeanPercentError());

  auto stdPercentError = std::stod(argv[8]);
  voronoiSegmenter->SetSTDPercentError(stdPercentError);
  ITK_TEST_SET_GET_VALUE(stdPercentError, voronoiSegmenter->GetSTDPercentError());

  std::cout << "Running algorithm" << std::endl;
  voronoiSegmenter->Update();

  std::cout << "Walking output" << std::endl;
  itk::ImageRegionIteratorWithIndex<UShortImage> ot(voronoiSegmenter->GetOutput(), region);

  int k = 0;
  while (!ot.IsAtEnd())
  {
    TestImg[k] = ot.Get();
    (void)(TestImg[k]); // prevents "set but not used" warning
    k++;
    ++ot;
  }

  /* Test OK on local machine.
  FILE *imgfile = fopen("output.raw","wb");
  fwrite(TestImg,2,65536,imgfile);
  fclose(imgfile);

  imgfile = fopen("input.raw","wb");
  it.GoToBegin();
  k=0;
  while( !it.IsAtEnd()){
    TestImg[k]=it.Get();
    k++;
    ++it;
  }
  fwrite(TestImg,2,65536,imgfile);
  fclose(imgfile);*/

  std::cout << "Test Succeeded!" << std::endl;
  return EXIT_SUCCESS;
}
