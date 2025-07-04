project(ImageSegmentation)

add_executable(ConfidenceConnected ConfidenceConnected.cxx)
target_link_libraries(ConfidenceConnected ${ITK_LIBRARIES})

add_executable(ConfidenceConnected3D ConfidenceConnected3D.cxx)
target_link_libraries(ConfidenceConnected3D ${ITK_LIBRARIES})

add_executable(
  NeighborhoodConnectedImageFilter
  NeighborhoodConnectedImageFilter.cxx
)
target_link_libraries(NeighborhoodConnectedImageFilter ${ITK_LIBRARIES})

add_executable(IsolatedConnectedImageFilter IsolatedConnectedImageFilter.cxx)
target_link_libraries(IsolatedConnectedImageFilter ${ITK_LIBRARIES})

add_executable(ConnectedThresholdImageFilter ConnectedThresholdImageFilter.cxx)
target_link_libraries(ConnectedThresholdImageFilter ${ITK_LIBRARIES})

add_executable(FastMarchingImageFilter FastMarchingImageFilter.cxx)
target_link_libraries(FastMarchingImageFilter ${ITK_LIBRARIES})

add_executable(
  GeodesicActiveContourShapePriorLevelSetImageFilter
  GeodesicActiveContourShapePriorLevelSetImageFilter.cxx
)
target_link_libraries(
  GeodesicActiveContourShapePriorLevelSetImageFilter
  ${ITK_LIBRARIES}
)

add_executable(GibbsPriorImageFilter1 GibbsPriorImageFilter1.cxx)
target_link_libraries(GibbsPriorImageFilter1 ${ITK_LIBRARIES})

add_executable(
  HoughTransform2DCirclesImageFilter
  HoughTransform2DCirclesImageFilter.cxx
)
target_link_libraries(HoughTransform2DCirclesImageFilter ${ITK_LIBRARIES})

add_executable(
  HoughTransform2DLinesImageFilter
  HoughTransform2DLinesImageFilter.cxx
)
target_link_libraries(HoughTransform2DLinesImageFilter ${ITK_LIBRARIES})

add_executable(VectorConfidenceConnected VectorConfidenceConnected.cxx)
target_link_libraries(VectorConfidenceConnected ${ITK_LIBRARIES})

add_executable(RelabelComponentImageFilter RelabelComponentImageFilter.cxx)
target_link_libraries(RelabelComponentImageFilter ${ITK_LIBRARIES})

add_executable(
  CannySegmentationLevelSetImageFilter
  CannySegmentationLevelSetImageFilter.cxx
)
target_link_libraries(CannySegmentationLevelSetImageFilter ${ITK_LIBRARIES})

add_executable(
  GeodesicActiveContourImageFilter
  GeodesicActiveContourImageFilter.cxx
)
target_link_libraries(GeodesicActiveContourImageFilter ${ITK_LIBRARIES})

add_executable(ShapeDetectionLevelSetFilter ShapeDetectionLevelSetFilter.cxx)
target_link_libraries(ShapeDetectionLevelSetFilter ${ITK_LIBRARIES})

add_executable(CurvesLevelSetImageFilter CurvesLevelSetImageFilter.cxx)
target_link_libraries(CurvesLevelSetImageFilter ${ITK_LIBRARIES})

add_executable(WatershedSegmentation1 WatershedSegmentation1.cxx)
target_link_libraries(WatershedSegmentation1 ${ITK_LIBRARIES})

add_executable(WatershedSegmentation2 WatershedSegmentation2.cxx)
target_link_libraries(WatershedSegmentation2 ${ITK_LIBRARIES})

add_executable(
  LaplacianSegmentationLevelSetImageFilter
  LaplacianSegmentationLevelSetImageFilter.cxx
)
target_link_libraries(LaplacianSegmentationLevelSetImageFilter ${ITK_LIBRARIES})

add_executable(
  ThresholdSegmentationLevelSetImageFilter
  ThresholdSegmentationLevelSetImageFilter.cxx
)
target_link_libraries(ThresholdSegmentationLevelSetImageFilter ${ITK_LIBRARIES})

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
