set(SCC_NAME shortCircuit)

file(GLOB SCC_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB SCC_INCS    ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
set(SCC_PLIST         ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist)

# SDK headers grouped only for nicer IDE browsing
file(GLOB SCC_INC_TD     ${NATID_SDK_INC}/td/*.h)
file(GLOB SCC_INC_GUI    ${NATID_SDK_INC}/gui/*.h)
file(GLOB SCC_INC_FO     ${NATID_SDK_INC}/fo/*.h)
file(GLOB SCC_INC_SPARSE ${NATID_SDK_INC}/sparse/*.h)
file(GLOB SCC_INC_DENSE  ${NATID_SDK_INC}/dense/*.h)

add_executable(${SCC_NAME} ${SCC_INCS} ${SCC_SOURCES}
	${SCC_INC_TD} ${SCC_INC_GUI} ${SCC_INC_FO} ${SCC_INC_SPARSE} ${SCC_INC_DENSE})

source_group("src"          FILES ${SCC_SOURCES})
source_group("inc"          FILES ${SCC_INCS})
source_group("inc\\td"      FILES ${SCC_INC_TD})
source_group("inc\\gui"     FILES ${SCC_INC_GUI})
source_group("inc\\fo"      FILES ${SCC_INC_FO})
source_group("inc\\sparse"  FILES ${SCC_INC_SPARSE})
source_group("inc\\dense"   FILES ${SCC_INC_DENSE})

target_link_libraries(${SCC_NAME}
	debug ${MU_LIB_DEBUG}      optimized ${MU_LIB_RELEASE}
	debug ${NATGUI_LIB_DEBUG}  optimized ${NATGUI_LIB_RELEASE}
	debug ${MATRIX_LIB_DEBUG}  optimized ${MATRIX_LIB_RELEASE}
	debug ${DP_LIB_DEBUG}      optimized ${DP_LIB_RELEASE})

setTargetPropertiesForGUIApp(${SCC_NAME} ${SCC_PLIST})
setIDEPropertiesForGUIExecutable(${SCC_NAME} ${CMAKE_CURRENT_LIST_DIR})
setPlatformDLLPath(${SCC_NAME})
