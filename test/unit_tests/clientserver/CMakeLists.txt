add_executable( test_name_value_list name_value_list.test.cpp )
target_include_directories( test_name_value_list PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_name_value_list PRIVATE Catch2::Catch2WithMain c_api )
add_test( NAME TestNameValueList COMMAND test_name_value_list )

# install(
#   DIRECTORY
#   test_files
#   DESTINATION 
#   ${CMAKE_BINARY_DIR}/test/unit_tests/clientserver
# )

