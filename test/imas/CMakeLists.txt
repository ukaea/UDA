set( TESTS
  test_imas_hl2a_magnetics
  test_imas_ts_magnetics
)

add_definitions( -D__USE_XOPEN2K8 )

foreach( TEST ${TESTS} )
  BUILD_TEST( ${TEST} ${TEST}.cpp )
endforeach()
