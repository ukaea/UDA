cmake_minimum_required( VERSION 2.8 )

file( GLOB INSTALL_FILES udarest/*.py )

install( FILES ${INSTALL_FILES}
  DESTINATION include/udarest )
