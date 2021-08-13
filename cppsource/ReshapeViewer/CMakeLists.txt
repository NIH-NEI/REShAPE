
cmake_minimum_required(VERSION 3.9.5)
cmake_policy(SET CMP0020 NEW)

set(PROJ_NAME "ReshapeViewer")
project(${PROJ_NAME})
 
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(UNIX)
find_package(Qt5 REQUIRED COMPONENTS Core Gui X11Extras)
endif()
if(WIN32)
find_package(Qt5 REQUIRED COMPONENTS Core Gui)
endif()

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})
if (ITKVtkGlue_LOADED)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
else()
  find_package(ItkVtkGlue REQUIRED)
  include(${ItkVtkGlue_USE_FILE})
  set(Glue ItkVtkGlue)
endif()

file(GLOB H_FILES src/*.h)
file(GLOB CXX_FILES src/*.cxx)
file(GLOB CPP_FILES src/*.cpp)

set(VTK_VER "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
message("VTK_VER: ${VTK_VER}")
 
add_executable(${PROJ_NAME} WIN32 ${CXX_FILES} ${CPP_FILES} Icons/qresources.qrc)
if(UNIX)
#  target_sources(${PROJ_NAME} PUBLIC ${H_FILES})
endif()
target_link_libraries(${PROJECT_NAME}
  ${Glue}
  ${VTK_LIBRARIES}
  ${ITK_LIBRARIES}
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
)
if(UNIX)
target_link_libraries(${PROJECT_NAME}
  Qt5::X11Extras
)
endif()

if(WIN32)
	set(VTK_DLL ${VTK_DIR}/bin)
	set(QT5_DLL ${Qt5_DIR}/../../../bin)
	
	macro(cp_qt5 APP DLL)
	    add_custom_command(TARGET ${APP} POST_BUILD
	       COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${DLL}d.dll> $<$<NOT:$<CONFIG:Debug>>:${DLL}.dll> $<TARGET_FILE_DIR:${APP}>)
	endmacro()
	
	# copy all xxd.dll from QT5 depending on msvc debug or release build
	list(APPEND QT5_DLLS
	    "${QT5_DLL}/Qt5Core"
	    "${QT5_DLL}/Qt5Gui"
	    "${QT5_DLL}/Qt5Widgets"
	    )
	foreach( dll ${QT5_DLLS} )
	    cp_qt5(${PROJECT_NAME} ${dll})
	    install(FILES ${dll}.dll DESTINATION . CONFIGURATIONS Release)
	endforeach(dll)
	
	macro(cp_vtk APP DLL)
	    add_custom_command(TARGET ${APP} POST_BUILD
	       COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VTK_DLL}/$<CONFIG>/${DLL} $<TARGET_FILE_DIR:${APP}>)
	endmacro()
	
# copy all vtk dll depending on msvc debug or release build
if(${VTK_VER} VERSION_LESS "8.2")
list(APPEND VTK_DLLS
	"vtkalglib-${VTK_VER}.dll"
)
endif()
	list(APPEND VTK_DLLS
	    "vtkCommonColor-${VTK_VER}.dll"
	    "vtkCommonComputationalGeometry-${VTK_VER}.dll"
	    "vtkCommonCore-${VTK_VER}.dll"
	    "vtkCommonDataModel-${VTK_VER}.dll"
	    "vtkCommonExecutionModel-${VTK_VER}.dll"
	    "vtkCommonMath-${VTK_VER}.dll"
	    "vtkCommonMisc-${VTK_VER}.dll"
	    "vtkCommonSystem-${VTK_VER}.dll"
	    "vtkCommonTransforms-${VTK_VER}.dll"
	    "vtkDICOMParser-${VTK_VER}.dll"
	    "vtkFiltersCore-${VTK_VER}.dll"
	    "vtkFiltersExtraction-${VTK_VER}.dll"
	    "vtkFiltersGeneral-${VTK_VER}.dll"
	    "vtkFiltersGeometry-${VTK_VER}.dll"
	    "vtkFiltersSources-${VTK_VER}.dll"
	    "vtkFiltersStatistics-${VTK_VER}.dll"
	    "vtkfreetype-${VTK_VER}.dll"
	    "vtkglew-${VTK_VER}.dll"
	    "vtkImagingCore-${VTK_VER}.dll"
	    "vtkImagingFourier-${VTK_VER}.dll"
	    "vtkInteractionStyle-${VTK_VER}.dll"
	    "vtkIOImage-${VTK_VER}.dll"
	    "vtkjpeg-${VTK_VER}.dll"
	    "vtkmetaio-${VTK_VER}.dll"
	    "vtkpng-${VTK_VER}.dll"
	    "vtkRenderingCore-${VTK_VER}.dll"
	    "vtkRenderingFreeType-${VTK_VER}.dll"
	    "vtkRenderingOpenGL2-${VTK_VER}.dll"
	    "vtksys-${VTK_VER}.dll"
	    "vtktiff-${VTK_VER}.dll"
	    "vtkzlib-${VTK_VER}.dll"
	    "vtkGUISupportQt-${VTK_VER}.dll"
	    "vtkRenderingGL2PSOpenGL2-${VTK_VER}.dll"
	    "vtkgl2ps-${VTK_VER}.dll"
	)
	foreach( dll ${VTK_DLLS} )
	    cp_vtk(${PROJ_NAME} ${dll})
	    install(FILES ${VTK_DLL}/Release/${dll} DESTINATION . CONFIGURATIONS Release)
	endforeach(dll)
	
	install(FILES "${Qt5_DIR}/../../../plugins/platforms/qwindows.dll" DESTINATION "./platforms" CONFIGURATIONS Release)
	install(FILES "${Qt5_DIR}/../../../plugins/imageformats/qtiff.dll" DESTINATION "./imageformats" CONFIGURATIONS Release)
	install(FILES "${Qt5_DIR}/../../../translations/qt_en.qm" DESTINATION "./translations" CONFIGURATIONS Release)
	
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
	include (InstallRequiredSystemLibraries)

	install(TARGETS ${PROJ_NAME} DESTINATION . CONFIGURATIONS Release)
endif()

if(UNIX)
	install(PROGRAMS "${${PROJ_NAME}_BINARY_DIR}/${PROJ_NAME}" DESTINATION .)
endif()
