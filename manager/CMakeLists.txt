function(build_manager)
  project(MANAGER_BINARY)

  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -s")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb")

  add_executable(MANAGER_BINARY "${PROJECT_SOURCE_DIR}/manager/main.c")

  target_link_libraries(MANAGER_BINARY pthread)

  install(TARGETS MANAGER_BINARY RUNTIME DESTINATION /var/www/cgi_data/vertesine)
endfunction()
