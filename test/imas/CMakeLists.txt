set( TESTS
  test_imas_jet_magnetics
  test_imas_jet_tf
  test_imas_jet_pf_active
  test_imas_jet_wall
  test_imas_jet_iron_core
  test_imas_jet_mse
  test_imas_jet_thomson_scattering
  test_imas_jet_all
  test_imas_tcv_magnetics
  test_imas_tcv_pf_active
  test_imas_aug_magnetics
  test_imas_aug_pf_active
)

add_definitions( -D__USE_XOPEN2K8 )

foreach( TEST ${TESTS} )
  BUILD_TEST( ${TEST} ${TEST}.cpp )
endforeach()
