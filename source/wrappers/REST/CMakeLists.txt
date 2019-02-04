file( GLOB INSTALL_FILES udarest/*.py )

install( FILES ${INSTALL_FILES}
  DESTINATION include/udarest )
