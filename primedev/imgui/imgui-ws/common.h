#pragma once

// the main js module
constexpr auto kImGuiWS_js = R"(var imgui_ws = {
)" R"(    canvas: null,
)" R"(    gl: null,
)" R"(
)" R"(    shader_program: null,
)" R"(    vertex_buffer: null,
)" R"(    index_buffer: null,
)" R"(
)" R"(    attribute_location_tex: null,
)" R"(    attribute_location_proj_mtx: null,
)" R"(    attribute_location_position: null,
)" R"(    attribute_location_uv: null,
)" R"(    attribute_location_color: null,
)" R"(
)" R"(    device_pixel_ratio: window.device_pixel_ratio || 1,
)" R"(
)" R"(    tex_map_id: {},
)" R"(    tex_map_rev: {},
)" R"(    tex_map_abuf: {},
)" R"(
)" R"(    n_draw_lists: null,
)" R"(    draw_lists_abuf: {},
)" R"(
)" R"(    io: {
)" R"(        mouse_x: 0.0,
)" R"(        mouse_y: 0.0,
)" R"(
)" R"(        want_capture_mouse: true,
)" R"(        want_capture_keyboard: true,
)" R"(    },
)" R"(
)" R"(    init: function(canvas_name) {
)" R"(        this.canvas = document.getElementById(canvas_name);
)" R"(
)" R"(        var onkeyup = this.canvas_on_keyup.bind(this);
)" R"(        var onkeydown = this.canvas_on_keydown.bind(this);
)" R"(        var onkeypress = this.canvas_on_keypress.bind(this);
)" R"(        var onpointermove = this.canvas_on_pointermove.bind(this);
)" R"(        var onpointerdown = this.canvas_on_pointerdown.bind(this);
)" R"(        var onpointerup = this.canvas_on_pointerup.bind(this);
)" R"(        var onwheel = this.canvas_on_wheel.bind(this);
)" R"(        var ontouch = this.canvas_on_touch.bind(this);
)" R"(
)" R"(        //this.canvas.style.touchAction = 'none'; // Disable browser handling of all panning and zooming gestures.
)" R"(        //this.canvas.addEventListener('blur', this.canvas_on_blur);
)" R"(
)" R"(        this.canvas.addEventListener('keyup', onkeyup, true);
)" R"(        this.canvas.addEventListener('keydown', onkeydown, true);
)" R"(        this.canvas.addEventListener('keypress', onkeypress, true);
)" R"(
)" R"(        this.canvas.addEventListener('pointermove', onpointermove);
)" R"(        this.canvas.addEventListener('mousemove', onpointermove);
)" R"(
)" R"(        this.canvas.addEventListener('pointerdown', onpointerdown);
)" R"(        this.canvas.addEventListener('mousedown', onpointerdown);
)" R"(
)" R"(        this.canvas.addEventListener('pointerup', onpointerup);
)" R"(        this.canvas.addEventListener('mouseup', onpointerup);
)" R"(
)" R"(        this.canvas.addEventListener('contextmenu', function(event) { event.preventDefault(); });
)" R"(        this.canvas.addEventListener('wheel', onwheel, false);
)" R"(
)" R"(        this.canvas.addEventListener('touchstart', ontouch);
)" R"(        this.canvas.addEventListener('touchmove', ontouch);
)" R"(        this.canvas.addEventListener('touchend', ontouch);
)" R"(        this.canvas.addEventListener('touchcancel', ontouch);
)" R"(
)" R"(        this.gl = this.canvas.getContext('webgl');
)" R"(
)" R"(        this.vertex_buffer = this.gl.createBuffer();
)" R"(        this.index_buffer = this.gl.createBuffer();
)" R"(
)" R"(        var vertex_shader_source = [
)" R"(            'precision mediump float;' +
)" R"(            'uniform mat4 ProjMtx;' +
)" R"(            'attribute vec2 Position;' +
)" R"(            'attribute vec2 UV;' +
)" R"(            'attribute vec4 Color;' +
)" R"(            'varying vec2 Frag_UV;' +
)" R"(            'varying vec4 Frag_Color;' +
)" R"(            'void main(void) {' +
)" R"(            '	Frag_UV = UV;' +
)" R"(            '	Frag_Color = Color;' +
)" R"(            '   gl_Position = ProjMtx * vec4(Position, 0, 1);' +
)" R"(            '}'
)" R"(        ];
)" R"(
)" R"(        var vertex_shader = this.gl.createShader(this.gl.VERTEX_SHADER);
)" R"(        this.gl.shaderSource(vertex_shader, vertex_shader_source);
)" R"(        this.gl.compileShader(vertex_shader);
)" R"(
)" R"(        var fragment_shader_source = [
)" R"(            'precision mediump float;' +
)" R"(            'uniform sampler2D Texture;' +
)" R"(            'varying vec2 Frag_UV;' +
)" R"(            'varying vec4 Frag_Color;' +
)" R"(            'void main() {' +
)" R"(            '	gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);' +
)" R"(            '}'
)" R"(        ];
)" R"(
)" R"(        var fragment_shader = this.gl.createShader(this.gl.FRAGMENT_SHADER);
)" R"(        this.gl.shaderSource(fragment_shader, fragment_shader_source);
)" R"(        this.gl.compileShader(fragment_shader);
)" R"(
)" R"(        this.shader_program = this.gl.createProgram();
)" R"(        this.gl.attachShader(this.shader_program, vertex_shader);
)" R"(        this.gl.attachShader(this.shader_program, fragment_shader);
)" R"(        this.gl.linkProgram(this.shader_program);
)" R"(
)" R"(        this.attribute_location_tex      = this.gl.getUniformLocation(this.shader_program,   "Texture");
)" R"(        this.attribute_location_proj_mtx = this.gl.getUniformLocation(this.shader_program,   "ProjMtx");
)" R"(        this.attribute_location_position = this.gl.getAttribLocation(this.shader_program,    "Position");
)" R"(        this.attribute_location_uv       = this.gl.getAttribLocation(this.shader_program,    "UV");
)" R"(        this.attribute_location_color    = this.gl.getAttribLocation(this.shader_program,    "Color");
)" R"(
)" R"(        this.gl.enable(this.gl.BLEND);
)" R"(        this.gl.blendEquation(this.gl.FUNC_ADD);
)" R"(        this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE_MINUS_SRC_ALPHA);
)" R"(        this.gl.disable(this.gl.CULL_FACE);
)" R"(        this.gl.disable(this.gl.DEPTH_TEST);
)" R"(        this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);
)" R"(
)" R"(        var clip_off_x = 0.0;
)" R"(        var clip_off_y = 0.0;
)" R"(
)" R"(        const L = clip_off_x;
)" R"(        const R = clip_off_x + this.canvas.width;
)" R"(        const T = clip_off_y;
)" R"(        const B = clip_off_y + this.canvas.height;
)" R"(
)" R"(        const ortho_projection = new Float32Array([
)" R"(            2.0 / (R - L), 0.0, 0.0, 0.0,
)" R"(            0.0, 2.0 / (T - B), 0.0, 0.0,
)" R"(            0.0, 0.0, -1.0, 0.0,
)" R"(            (R + L) / (L - R), (T + B) / (B - T), 0.0, 1.0,
)" R"(        ]);
)" R"(        this.gl.useProgram(this.shader_program);
)" R"(        this.gl.uniform1i(this.attribute_location_tex, 0);
)" R"(        this.gl.uniformMatrix4fv(this.attribute_location_proj_mtx, false, ortho_projection);
)" R"(
)" R"(        this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.vertex_buffer);
)" R"(        this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, this.index_buffer);
)" R"(
)" R"(        this.gl.enableVertexAttribArray(this.attribute_location_position);
)" R"(        this.gl.enableVertexAttribArray(this.attribute_location_uv);
)" R"(        this.gl.enableVertexAttribArray(this.attribute_location_color);
)" R"(        this.gl.vertexAttribPointer(this.attribute_location_position, 2, this.gl.FLOAT,         false, 5*4, 0);
)" R"(        this.gl.vertexAttribPointer(this.attribute_location_uv,       2, this.gl.FLOAT,         false, 5*4, 2*4);
)" R"(        this.gl.vertexAttribPointer(this.attribute_location_color,    4, this.gl.UNSIGNED_BYTE, true,  5*4, 4*4);
)" R"(    },
)" R"(
)" R"(    incppect_textures: function(incppect) {
)" R"(        var n_textures = incppect.get_int32('imgui.n_textures');
)" R"(
)" R"(        for (var i = 0; i < n_textures; ++i) {
)" R"(            var tex_id = incppect.get_int32('imgui.texture_id[%d]', i);
)" R"(            var tex_rev = incppect.get_int32('imgui.texture_revision[%d]', tex_id);
)" R"(
)" R"(            if (this.tex_map_abuf[tex_id] == null || this.tex_map_abuf[tex_id].byteLength < 1) {
)" R"(                this.tex_map_abuf[tex_id] = incppect.get_abuf('imgui.texture_data[%d]', tex_id);
)" R"(            } else if (this.tex_map_abuf[tex_id] && (this.tex_map_id[tex_id] == null || this.tex_map_rev[tex_id] != tex_rev)) {
)" R"(                this.tex_map_abuf[tex_id] = incppect.get_abuf('imgui.texture_data[%d]', tex_id);
)" R"(                imgui_ws.init_tex(tex_id, tex_rev, this.tex_map_abuf[tex_id]);
)" R"(            }
)" R"(        }
)" R"(    },
)" R"(
)" R"(    init_tex: function(tex_id, tex_rev, tex_abuf) {
)" R"(        var tex_abuf_uint8 = new Uint8Array(tex_abuf);
)" R"(        var tex_abuf_int32 = new Int32Array(tex_abuf);
)" R"(
)" R"(        const type = tex_abuf_int32[1];
)" R"(        const width = tex_abuf_int32[2];
)" R"(        const height = tex_abuf_int32[3];
)" R"(        const revision = tex_abuf_int32[4];
)" R"(
)" R"(        if (this.tex_map_rev[tex_id] && revision == this.tex_map_rev[tex_id]) {
)" R"(            return;
)" R"(        }
)" R"(
)" R"(        var pixels = new Uint8Array(4*width*height);
)" R"(
)" R"(        if (type == 0) { // Alpha8
)" R"(            for (var i = 0; i < width*height; ++i) {
)" R"(                pixels[4*i + 0] = 0xFF;
)" R"(                pixels[4*i + 1] = 0xFF;
)" R"(                pixels[4*i + 2] = 0xFF;
)" R"(                pixels[4*i + 3] = tex_abuf_uint8[20 + i];
)" R"(            }
)" R"(        } else if (type == 1) { // Gray8
)" R"(            for (var i = 0; i < width*height; ++i) {
)" R"(                pixels[4*i + 0] = tex_abuf_uint8[20 + i];
)" R"(                pixels[4*i + 1] = tex_abuf_uint8[20 + i];
)" R"(                pixels[4*i + 2] = tex_abuf_uint8[20 + i];
)" R"(                pixels[4*i + 3] = 0xFF;
)" R"(            }
)" R"(        } else if (type == 2) { // RGB24
)" R"(            for (var i = 0; i < width*height; ++i) {
)" R"(                pixels[4*i + 0] = tex_abuf_uint8[20 + 3*i + 0];
)" R"(                pixels[4*i + 1] = tex_abuf_uint8[20 + 3*i + 1];
)" R"(                pixels[4*i + 2] = tex_abuf_uint8[20 + 3*i + 2];
)" R"(                pixels[4*i + 3] = 0xFF;
)" R"(            }
)" R"(        } else if (type == 3) { // RGBA32
)" R"(            for (var i = 0; i < width*height; ++i) {
)" R"(                pixels[4*i + 0] = tex_abuf_uint8[20 + 4*i + 0];
)" R"(                pixels[4*i + 1] = tex_abuf_uint8[20 + 4*i + 1];
)" R"(                pixels[4*i + 2] = tex_abuf_uint8[20 + 4*i + 2];
)" R"(                pixels[4*i + 3] = tex_abuf_uint8[20 + 4*i + 3];
)" R"(            }
)" R"(        }
)" R"(
)" R"(        this.tex_map_rev[tex_id] = tex_rev;
)" R"(
)" R"(        if (this.tex_map_id[tex_id]) {
)" R"(            this.gl.bindTexture(this.gl.TEXTURE_2D, this.tex_map_id[tex_id]);
)" R"(            this.gl.texImage2D(this.gl.TEXTURE_2D, 0, this.gl.RGBA, width, height, 0, this.gl.RGBA, this.gl.UNSIGNED_BYTE, pixels);
)" R"(        } else {
)" R"(            this.tex_map_id[tex_id] = this.gl.createTexture();
)" R"(            this.gl.bindTexture(this.gl.TEXTURE_2D, this.tex_map_id[tex_id]);
)" R"(            this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_MIN_FILTER, this.gl.LINEAR);
)" R"(            this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_MAG_FILTER, this.gl.LINEAR);
)" R"(            //this.gl.pixelStorei(gl.UNPACK_ROW_LENGTH); // WebGL2
)" R"(            this.gl.texImage2D(this.gl.TEXTURE_2D, 0, this.gl.RGBA, width, height, 0, this.gl.RGBA, this.gl.UNSIGNED_BYTE, pixels);
)" R"(        }
)" R"(    },
)" R"(
)" R"(    incppect_draw_lists: function(incppect) {
)" R"(        this.n_draw_lists = incppect.get_int32('imgui.n_draw_lists');
)" R"(        if (this.n_draw_lists < 1) return;
)" R"(
)" R"(        for (var i = 0; i < this.n_draw_lists; ++i) {
)" R"(            this.draw_lists_abuf[i] = incppect.get_abuf('imgui.draw_list[%d]', i);
)" R"(        }
)" R"(    },
)" R"(
)" R"(    render: function(n_draw_lists, draw_lists_abuf) {
)" R"(        if (typeof n_draw_lists === "undefined" && typeof draw_lists_abuf === "undefined") {
)" R"(            if (this.n_draw_lists === null) return;
)" R"(            if (this.draw_lists_abuf === null) return;
)" R"(
)" R"(            this.render(this.n_draw_lists, this.draw_lists_abuf);
)" R"(
)" R"(            return;
)" R"(        }
)" R"(
)" R"(        var clip_off_x = 0.0;
)" R"(        var clip_off_y = 0.0;
)" R"(
)" R"(        this.gl.enable(this.gl.SCISSOR_TEST);
)" R"(
)" R"(        for (var i_list = 0; i_list < n_draw_lists; ++i_list) {
)" R"(            if (draw_lists_abuf[i_list].byteLength < 1) continue;
)" R"(
)" R"(            var draw_data_offset = 0;
)" R"(
)" R"(            var p = new Float32Array(draw_lists_abuf[i_list], draw_data_offset, 2);
)" R"(            var offset_x = p[0]; draw_data_offset += 4;
)" R"(            var offset_y = p[1]; draw_data_offset += 4;
)" R"(
)" R"(            p = new Uint32Array(draw_lists_abuf[i_list], draw_data_offset, 1);
)" R"(            var n_vertices = p[0]; draw_data_offset += 4;
)" R"(
)" R"(            var av = new Float32Array(draw_lists_abuf[i_list], draw_data_offset, 5*n_vertices);
)" R"(
)" R"(            for (var k = 0; k < n_vertices; ++k) {
)" R"(                av[5*k + 0] += offset_x;
)" R"(                av[5*k + 1] += offset_y;
)" R"(            }
)" R"(
)" R"(            this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.vertex_buffer);
)" R"(            this.gl.bufferData(this.gl.ARRAY_BUFFER, av, this.gl.STREAM_DRAW);
)" R"(
)" R"(            for (var k = 0; k < n_vertices; ++k) {
)" R"(                av[5*k + 0] -= offset_x;
)" R"(                av[5*k + 1] -= offset_y;
)" R"(            }
)" R"(
)" R"(            draw_data_offset += 5*4*n_vertices;
)" R"(
)" R"(            p = new Uint32Array(draw_lists_abuf[i_list], draw_data_offset, 1);
)" R"(            var n_indices = p[0]; draw_data_offset += 4;
)" R"(
)" R"(            var ai = new Uint16Array(draw_lists_abuf[i_list], draw_data_offset, n_indices);
)" R"(            this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, this.index_buffer);
)" R"(            this.gl.bufferData(this.gl.ELEMENT_ARRAY_BUFFER, ai, this.gl.STREAM_DRAW);
)" R"(            draw_data_offset += 2*n_indices;
)" R"(
)" R"(            p = new Uint32Array(draw_lists_abuf[i_list], draw_data_offset, 1);
)" R"(            var n_cmd = p[0]; draw_data_offset += 4;
)" R"(
)" R"(            for (var i_cmd = 0; i_cmd < n_cmd; ++i_cmd) {
)" R"(                var pi = new Uint32Array(draw_lists_abuf[i_list], draw_data_offset, 4);
)" R"(                var n_elements = pi[0]; draw_data_offset += 4;
)" R"(                var texture_id = pi[1]; draw_data_offset += 4;
)" R"(                var offset_vtx = pi[2]; draw_data_offset += 4;
)" R"(                var offset_idx = pi[3]; draw_data_offset += 4;
)" R"(
)" R"(                var pf = new Float32Array(draw_lists_abuf[i_list], draw_data_offset, 4);
)" R"(                var clip_x = pf[0] - clip_off_x; draw_data_offset += 4;
)" R"(                var clip_y = pf[1] - clip_off_y; draw_data_offset += 4;
)" R"(                var clip_z = pf[2] - clip_off_x; draw_data_offset += 4;
)" R"(                var clip_w = pf[3] - clip_off_y; draw_data_offset += 4;
)" R"(
)" R"(                if (clip_x < this.canvas.width && clip_y < this.canvas.height && clip_z >= 0.0 && clip_w >= 0.0) {
)" R"(                    this.gl.scissor(clip_x, this.canvas.height - clip_w, clip_z - clip_x, clip_w - clip_y);
)" R"(                    if (texture_id in this.tex_map_id) {
)" R"(                        this.gl.activeTexture(this.gl.TEXTURE0);
)" R"(                        this.gl.bindTexture(this.gl.TEXTURE_2D, this.tex_map_id[texture_id]);
)" R"(                    }
)" R"(                    this.gl.drawElements(this.gl.TRIANGLES, n_elements, this.gl.UNSIGNED_SHORT, 2*offset_idx);
)" R"(                }
)" R"(            }
)" R"(        }
)" R"(
)" R"(        this.gl.disable(this.gl.SCISSOR_TEST);
)" R"(    },
)" R"(
)" R"(    canvas_on_keyup: function(event) {},
)" R"(    canvas_on_keydown: function(event) {},
)" R"(    canvas_on_keypress: function(event) {},
)" R"(    canvas_on_pointermove: function(event) {},
)" R"(    canvas_on_pointerdown: function(event) {},
)" R"(    canvas_on_pointerup: function(event) {},
)" R"(    canvas_on_wheel: function(event) {},
)" R"(    canvas_on_touch: function(event) {},
)" R"(
)" R"(    set_incppect_handlers: function(incppect) {
)" R"(        this.canvas_on_keyup = function(event) {
)" R"(            incppect.send('6 ' + event.keyCode);
)" R"(        };
)" R"(
)" R"(        this.canvas_on_keydown = function(event) {
)" R"(            incppect.send('5 ' + event.keyCode);
)" R"(        };
)" R"(
)" R"(        this.canvas_on_keypress = function(event) {
)" R"(            incppect.send('4 ' + event.keyCode);
)" R"(
)" R"(            if (this.io.want_capture_keyboard) {
)" R"(                event.preventDefault();
)" R"(            }
)" R"(        };
)" R"(
)" R"(        this.canvas_on_pointermove = function(event) {
)" R"(            this.io.mouse_x = event.offsetX * this.device_pixel_ratio;
)" R"(            this.io.mouse_y = event.offsetY * this.device_pixel_ratio;
)" R"(
)" R"(            incppect.send('0 ' + this.io.mouse_x + ' ' + this.io.mouse_y);
)" R"(
)" R"(            if (this.io.want_capture_mouse) {
)" R"(                event.preventDefault();
)" R"(            }
)" R"(        };
)" R"(
)" R"(        this.canvas_on_pointerdown = function(event) {
)" R"(            this.io.mouse_x = event.offsetX * this.device_pixel_ratio;
)" R"(            this.io.mouse_y = event.offsetY * this.device_pixel_ratio;
)" R"(
)" R"(            incppect.send('1 ' + event.button + ' ' + this.io.mouse_x + ' ' + this.io.mouse_y);
)" R"(        };
)" R"(
)" R"(        this.canvas_on_pointerup = function(event) {
)" R"(            incppect.send('2 ' + event.button + ' ' + this.io.mouse_x + ' ' + this.io.mouse_y);
)" R"(
)" R"(            if (this.io.want_capture_mouse) {
)" R"(                event.preventDefault();
)" R"(            }
)" R"(        };
)" R"(
)" R"(        this.canvas_on_wheel = function(event) {
)" R"(            let scale = 1.0;
)" R"(            switch (event.deltaMode) {
)" R"(                case event.DOM_DELTA_PIXEL:
)" R"(                    scale = 0.01;
)" R"(                    break;
)" R"(                case event.DOM_DELTA_LINE:
)" R"(                    scale = 0.2;
)" R"(                    break;
)" R"(                case event.DOM_DELTA_PAGE:
)" R"(                    scale = 1.0;
)" R"(                    break;
)" R"(            }
)" R"(
)" R"(            var wheel_x =  event.deltaX * scale;
)" R"(            var wheel_y = -event.deltaY * scale;
)" R"(
)" R"(            incppect.send('3 ' + wheel_x + ' ' + wheel_y);
)" R"(
)" R"(            if (this.io.want_capture_mouse) {
)" R"(                event.preventDefault();
)" R"(            }
)" R"(        };
)" R"(
)" R"(        this.canvas_on_touch = function (event) {
)" R"(            if (event.touches.length > 1) {
)" R"(                return;
)" R"(            }
)" R"(
)" R"(            var touches = event.changedTouches,
)" R"(                first = touches[0],
)" R"(                type = "";
)" R"(
)" R"(            switch(event.type) {
)" R"(                case "touchstart": type = "mousedown"; break;
)" R"(                case "touchmove":  type = "mousemove"; break;
)" R"(                case "touchend":   type = "mouseup";   break;
)" R"(                default:           return;
)" R"(            }
)" R"(
)" R"(            // initMouseEvent(type, canBubble, cancelable, view, clickCount,
)" R"(            //                screenX, screenY, clientX, clientY, ctrlKey,
)" R"(            //                altKey, shiftKey, metaKey, button, relatedTarget);
)" R"(
)" R"(            var simulatedEvent = document.createEvent("MouseEvent");
)" R"(            simulatedEvent.initMouseEvent(type, true, true, window, 1, first.screenX, first.screenY, first.clientX, first.clientY, false, false, false, false, 0/*left*/, null);
)" R"(
)" R"(            first.target.dispatchEvent(simulatedEvent);
)" R"(            event.preventDefault();
)" R"(        };
)" R"(    },
)" R"(
)" R"(}
)" R"()";

// the main html page
constexpr auto kImGuiWS_htmll = R"(<html>
)" R"(<head>
)" R"(    <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
)" R"(    <title>imgui-ws : demo-null</title>
)" R"(
)" R"(    <script src="incppect.js"></script>
)" R"(    <script src="imgui-ws.js"></script>
)" R"(</head>
)" R"(
)" R"(<body style="font-family: Georgia, serif;">
)" R"(    <script>
)" R"(        function init() {
)" R"(            var output = document.getElementById('client-info');
)" R"(
)" R"(            incppect.render = function () {
)" R"(                imgui_ws.gl.clearColor(0.45, 0.55, 0.60, 1.00);
)" R"(                imgui_ws.gl.clear(imgui_ws.gl.COLOR_BUFFER_BIT);
)" R"(
)" R"(                imgui_ws.incppect_textures(this);
)" R"(                imgui_ws.incppect_draw_lists(this);
)" R"(                imgui_ws.render();
)" R"(
)" R"(                var my_id = this.get_int32('my_id[%d]', -1) || 0;
)" R"(                output.innerHTML = 'Your client Id: ' + my_id;
)" R"(            }
)" R"(
)" R"(            incppect.onerror = function (evt) {
)" R"(                if (typeof evt === 'object') {
)" R"(                    output.innerHTML = 'Error: check console for more information';
)" R"(                    console.error(evt);
)" R"(                } else {
)" R"(                    output.innerHTML = evt;
)" R"(                }
)" R"(            }
)" R"(
)" R"(            incppect.k_requests_update_freq_ms = document.getElementById('update_freq_ms').value;
)" R"(            incppect.init();
)" R"(
)" R"(            imgui_ws.set_incppect_handlers(incppect);
)" R"(            imgui_ws.init('canvas_main');
)" R"(        }
)" R"(
)" R"(        window.addEventListener('load', init);
)" R"(
)" R"(    </script>
)" R"(
)" R"(    <div id=main-container align=left width=900px style='padding-left: 16px; padding-top: 1px;'>
)" R"(        <h2>imgui-ws : demo-null</h2>
)" R"(        <div style='padding: 3px; width: 800px; word-wrap: break-word;'>
)" R"(            The vertex and index arrays for the Dear ImGui scene below are generated server-side.
)" R"(            The arrays are streamed to the WebSocket clients and rendered in the browser using WebGL.
)" R"(        </div>
)" R"(        <br>
)" R"(        <div style='padding: 3px; width: 800px; word-wrap: break-word;'>
)" R"(            There can be multiple clients connected simultaneously to the same server (see the "WebSocket clients" window below).
)" R"(            Wait for your client to take control and try playing with the widgets.
)" R"(            Your actions will be visible to all currently connected clients.
)" R"(        </div>
)" R"(        <br>
)" R"(        <div id="client-info"></div>
)" R"(        Update freq: <input type="range" min="16" max="200" value="16" class="slider" id="update_freq_ms"
)" R"(                            onChange="incppect.k_requests_update_freq_ms = this.value; update_freq_ms_out.value = this.value;">
)" R"(        <output id="update_freq_ms_out">16</output>[ms]<br>
)" R"(        <br>
)" R"(        <canvas id="canvas_main" width="1200px" height="800px" style="background-color: black;" tabindex="0">Your browser does not support the HTML5 canvas tag.</canvas>
)" R"(
)" R"(        <br>
)" R"(        <a href="https://github.com/ggerganov/imgui-ws">
)" R"(            <span class="icon icon--github">
)" R"(                <svg viewBox="0 0 16 16" width="16px" height="16px"><path fill="#828282" d="M7.999,0.431c-4.285,0-7.76,3.474-7.76,7.761 c0,3.428,2.223,6.337,5.307,7.363c0.388,0.071,0.53-0.168,0.53-0.374c0-0.184-0.007-0.672-0.01-1.32 c-2.159,0.469-2.614-1.04-2.614-1.04c-0.353-0.896-0.862-1.135-0.862-1.135c-0.705-0.481,0.053-0.472,0.053-0.472 c0.779,0.055,1.189,0.8,1.189,0.8c0.692,1.186,1.816,0.843,2.258,0.645c0.071-0.502,0.271-0.843,0.493-1.037 C4.86,11.425,3.049,10.76,3.049,7.786c0-0.847,0.302-1.54,0.799-2.082C3.768,5.507,3.501,4.718,3.924,3.65 c0,0,0.652-0.209,2.134,0.796C6.677,4.273,7.34,4.187,8,4.184c0.659,0.003,1.323,0.089,1.943,0.261 c1.482-1.004,2.132-0.796,2.132-0.796c0.423,1.068,0.157,1.857,0.077,2.054c0.497,0.542,0.798,1.235,0.798,2.082 c0,2.981-1.814,3.637-3.543,3.829c0.279,0.24,0.527,0.713,0.527,1.437c0,1.037-0.01,1.874-0.01,2.129 c0,0.208,0.14,0.449,0.534,0.373c3.081-1.028,5.302-3.935,5.302-7.362C15.76,3.906,12.285,0.431,7.999,0.431z" /></svg>
)" R"(            </span><span class="repo">Source code</span>
)" R"(        </a>
)" R"(    </div>
)" R"(
)" R"(
)" R"(</body>
)" R"(</html>
)" R"()";
