
cmake_minimum_required(VERSION 3.9.5)
cmake_policy(SET CMP0020 NEW)

set(PROJ_NAME "RsCircles")
project(${PROJ_NAME})
 
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

file(GLOB H_FILES src/*.h)
file(GLOB CXX_FILES src/*.cxx)
file(GLOB CPP_FILES src/*.cpp)

add_executable(${PROJ_NAME} WIN32 ${CXX_FILES} ${CPP_FILES} Icons/qresources.qrc)
if(UNIX)
#	target_sources(${PROJ_NAME} PUBLIC ${H_FILES})
endif()
target_link_libraries(${PROJECT_NAME}
  ${Glue}
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
)

if(WIN32)
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
	
	install(FILES "${Qt5_DIR}/../../../plugins/platforms/qwindows.dll" DESTINATION "./platforms" CONFIGURATIONS Release)
	install(FILES "${Qt5_DIR}/../../../plugins/imageformats/qtiff.dll" DESTINATION "./imageformats" CONFIGURATIONS Release)
	install(FILES "${Qt5_DIR}/../../../translations/qt_en.qm" DESTINATION "./translations" CONFIGURATIONS Release)
	
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Qt5_DIR}/../../../plugins/platforms/qwindows.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/qwindows.dll")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Qt5_DIR}/../../../plugins/imageformats/qtiff.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/imageformats/qtiff.dll")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Qt5_DIR}/../../../translations/qt_en.qm" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations/qt_en.qm")
	
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
	include (InstallRequiredSystemLibraries)

	install(TARGETS ${PROJ_NAME} DESTINATION . CONFIGURATIONS Release)
endif()

if(UNIX)
	install(PROGRAMS "${${PROJ_NAME}_BINARY_DIR}/${PROJ_NAME}" DESTINATION .)
endif()
