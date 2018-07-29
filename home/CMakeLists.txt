function(build_home)
  project(HOME_BINARY)

  include(GNUInstallDirs)

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -s")

  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/find/")

  find_package(kcgi REQUIRED)

  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lkcgi -lz")

  add_executable(HOME_BINARY "${PROJECT_SOURCE_DIR}/home/home.c" "${PROJECT_SOURCE_DIR}/util.c" "${PROJECT_SOURCE_DIR}/home/user.c" "${PROJECT_SOURCE_DIR}/lib/snowflake/snowflake.c" "${PROJECT_SOURCE_DIR}/home/manage.c")

  target_link_libraries(HOME_BINARY kcgi z pthread unqlite :libsodium.a jansson curl b64 gd)

  install(TARGETS HOME_BINARY RUNTIME DESTINATION /var/www/cgi_data/vertesine)
endfunction()
