target_include_directories(app PRIVATE .)
target_sources(app PRIVATE
               ${CMAKE_CURRENT_SOURCE_DIR}/HTTPServer.cpp
)

set(gen_dir ${CMAKE_CURRENT_SOURCE_DIR}/generated/)
set(source_file_index resources/index.html)
generate_inc_file_for_target(app ${source_file_index} ${gen_dir}/index.html.gz.inc --gzip)