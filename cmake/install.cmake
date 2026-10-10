include(CMakePackageConfigHelpers)

set(teaql_packages teaql_memory teaql_async teaql_core teaql_data_service
    teaql_sql teaql_runtime teaql_provider_sqlite)
foreach(package IN LISTS teaql_packages)
    string(REGEX REPLACE "^teaql_" "" component ${package})
    set_target_properties(${package} PROPERTIES EXPORT_NAME ${component})
    install(TARGETS ${package} EXPORT teaql_cpp_targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/${package}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    if(TEAQL_ENABLE_MODULES)
        set_target_properties(${package}_module PROPERTIES EXPORT_NAME ${component}_module)
        install(TARGETS ${package}_module EXPORT teaql_cpp_targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_DATADIR}/teaql_cpp/modules/${package}
            CXX_MODULES_BMI DESTINATION "")
    endif()
endforeach()

set(teaql_config_dir ${CMAKE_INSTALL_LIBDIR}/cmake/teaql_cpp)
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/teaql_cpp_config.cmake.in
    ${PROJECT_BINARY_DIR}/teaql_cppConfig.cmake
    INSTALL_DESTINATION ${teaql_config_dir}
    PATH_VARS CMAKE_INSTALL_DATADIR)
write_basic_package_version_file(${PROJECT_BINARY_DIR}/teaql_cppConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${PROJECT_BINARY_DIR}/teaql_cppConfig.cmake
    ${PROJECT_BINARY_DIR}/teaql_cppConfigVersion.cmake DESTINATION ${teaql_config_dir})
install(EXPORT teaql_cpp_targets FILE teaql_cpp_targets.cmake NAMESPACE teaql::
    DESTINATION ${teaql_config_dir})
