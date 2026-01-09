target_include_directories(app PRIVATE .)
target_sources(app PRIVATE
               ${CMAKE_CURRENT_SOURCE_DIR}/HTTPServer.cpp
)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
set(source_file_index resources/index.html)
set(source_file_sucess resources/sucess.html)
set(source_file_error resources/error.html)
generate_inc_file_for_target(app ${source_file_index} ${gen_dir}/index.html.gz.inc --gzip)
generate_inc_file_for_target(app ${source_file_sucess} ${gen_dir}/sucess.html.gz.inc --gzip)
generate_inc_file_for_target(app ${source_file_error} ${gen_dir}/error.html.gz.inc --gzip)