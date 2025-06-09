
# generate the automatically generated files

configure_file("imgui-ws.js" src_imgui-ws_js)
string(REPLACE "\n" "\n)\" R\"(" src_imgui-ws_js "${src_imgui-ws_js}")
configure_file("index.html" src_index_html)
string(REPLACE "\n" "\n)\" R\"(" src_index_html "${src_index_html}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/common.h.in ${CMAKE_CURRENT_SOURCE_DIR}/common.h @ONLY)

configure_file("incppect/incppect.js" src_incppect_js)
string(REPLACE "\n" "\n)\" R\"(" src_incppect_js "${src_incppect_js}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/incppect/common.h.in ${CMAKE_CURRENT_SOURCE_DIR}/incppect/common.h @ONLY)
