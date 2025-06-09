# generate the automatically generated files

add_custom_target(
	ImGuiGeneratedFiles
	ALL
	SOURCES
	imgui-ws.js
	index.html
	incppect/incppect.js
)

file(READ "imgui-ws.js" src_imgui-ws_js)
string(REPLACE "\n" "\n)\" R\"(" src_imgui-ws_js "${src_imgui-ws_js}")
file(READ "index.html" src_index_html)
string(REPLACE "\n" "\n)\" R\"(" src_index_html "${src_index_html}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/common.h.in ${CMAKE_CURRENT_SOURCE_DIR}/common.h @ONLY)

file(READ "incppect/incppect.js" src_incppect_js)
string(REPLACE "\n" "\n)\" R\"(" src_incppect_js "${src_incppect_js}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/incppect/common.h.in ${CMAKE_CURRENT_SOURCE_DIR}/incppect/common.h @ONLY)
