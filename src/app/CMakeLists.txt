add_executable(madridista main.cpp)

target_link_libraries(madridista PRIVATE Qt6::Widgets)
set_target_properties(madridista PROPERTIES WIN32_EXECUTABLE ON)

madridista_set_warnings(madridista)