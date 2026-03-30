# generate the automatically generated files

set(IMGUI_WS_DIR ${SOURCE_DIR}/imgui/imgui-ws)

file(READ "${IMGUI_WS_DIR}/imgui-ws.js" src_imgui-ws_js)
string(REPLACE "\n" "\n)\" R\"(" src_imgui-ws_js "${src_imgui-ws_js}")
file(READ "${IMGUI_WS_DIR}/index.html" src_index_html)
string(REPLACE "\n" "\n)\" R\"(" src_index_html "${src_index_html}")
configure_file(${IMGUI_WS_DIR}/common.h.in ${IMGUI_WS_DIR}/common.h @ONLY)

file(READ "${IMGUI_WS_DIR}/incppect/incppect.js" src_incppect_js)
string(REPLACE "\n" "\n)\" R\"(" src_incppect_js "${src_incppect_js}")
configure_file(${IMGUI_WS_DIR}/incppect/common.h.in ${IMGUI_WS_DIR}/incppect/common.h @ONLY)
