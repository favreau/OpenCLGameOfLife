include_directories(../gol)

ADD_EXECUTABLE(
  golViewer
  main.cpp
)

TARGET_LINK_LIBRARIES(
    golViewer
    gol
	${OpenCL_LIBRARIES}
	${FREEGLUT_LIBRARIES}
)

# ------------------------------------------------------------
INSTALL(TARGETS golViewer DESTINATION bin)
# ------------------------------------------------------------
