SET(GOL_SOURCES OpenCLKernel.cpp)
SET(GOL_HEADERS_PUBLIC OpenCLKernel.h)

ADD_LIBRARY(
	gol 
	${GOL_SOURCES})
	
TARGET_LINK_LIBRARIES(
	gol
	${OPENCL_LIBRARIES})

# ------------------------------------------------------------
INSTALL(TARGETS gol DESTINATION lib)
# ------------------------------------------------------------
