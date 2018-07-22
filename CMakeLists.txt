cmake_minimum_required(VERSION 3.0.2)


include(${PROJECT_SOURCE_DIR}/home/CMakeLists.txt)
include(${PROJECT_SOURCE_DIR}/comments/CMakeLists.txt)
include(${PROJECT_SOURCE_DIR}/manager/CMakeLists.txt)
build_home()
build_comments()
build_manager()

install(DIRECTORY res/vertesine DESTINATION /var/www/html)
install(DIRECTORY res/cgi_data DESTINATION /var/www)
