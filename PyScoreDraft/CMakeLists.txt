cmake_minimum_required (VERSION 3.0)

find_package(PythonLibs 3 REQUIRED)

set(SOURCES
PyScoreDraft.cpp
WinWavWriter.cpp
MIDIWriter.cpp
)

set(HEADERS 
PyScoreDraft.h
WinWavWriter.h
MIDIWriter.h
)

set (INCLUDE_DIR
${PYTHON_INCLUDE_DIRS}
.
../ScoreDraftCore
)

set (LINK_LIBS 
${PYTHON_LIBRARIES}
ScoreDraftCore)

if (WIN32) 
set (DEFINES  ${DEFINES}
-D"_CRT_SECURE_NO_DEPRECATE"  
-D"_SCL_SECURE_NO_DEPRECATE" 
)
else()
add_definitions(-std=c++0x)
add_compile_options(-fPIC)
endif()

include_directories(${INCLUDE_DIR})
add_definitions(${DEFINES})
add_library (PyScoreDraft SHARED ${SOURCES} ${HEADERS})
if (WIN32) 
set_target_properties(PyScoreDraft PROPERTIES SUFFIX ".pyd")
else()
SET_TARGET_PROPERTIES(PyScoreDraft PROPERTIES PREFIX "")
endif()
target_link_libraries(PyScoreDraft ${LINK_LIBS})





