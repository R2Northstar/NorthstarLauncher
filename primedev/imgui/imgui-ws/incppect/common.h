#pragma once

// the main js module
constexpr auto kIncppect_js = R"(var incppect = {
)" R"(    // websocket data
)" R"(    ws: null,
)" R"(
)" R"(    // default ws url - change to fit your needs
)" R"(    ws_uri: 'ws://' + window.location.hostname + ':' + window.location.port + '/incppect',
)" R"(
)" R"(    // vars data
)" R"(    nvars: 0,
)" R"(    vars_map: {},
)" R"(    var_to_id: {},
)" R"(    id_to_var: {},
)" R"(    last_data: null,
)" R"(
)" R"(    // requests data
)" R"(    requests: [],
)" R"(    requests_old: [],
)" R"(    requests_new_vars: false,
)" R"(    requests_regenerate: true,
)" R"(
)" R"(    // timestamps
)" R"(    t_start_ms: null,
)" R"(    t_frame_begin_ms: null,
)" R"(    t_requests_last_update_ms: null,
)" R"(
)" R"(    // constants
)" R"(    k_var_delim: ' ',
)" R"(    k_auto_reconnect: true,
)" R"(    k_requests_update_freq_ms: 50,
)" R"(
)" R"(    // stats
)" R"(    stats: {
)" R"(        tx_n: 0,
)" R"(        tx_bytes: 0,
)" R"(
)" R"(        rx_n: 0,
)" R"(        rx_bytes: 0,
)" R"(    },
)" R"(
)" R"(    timestamp: function() {
)" R"(        return window.performance && window.performance.now && window.performance.timing &&
)" R"(            window.performance.timing.navigationStart ? window.performance.now() + window.performance.timing.navigationStart : Date.now();
)" R"(    },
)" R"(
)" R"(    init: function() {
)" R"(        var onopen = this.onopen.bind(this);
)" R"(        var onclose = this.onclose.bind(this);
)" R"(        var onmessage = this.onmessage.bind(this);
)" R"(        var onerror = this.onerror.bind(this);
)" R"(
)" R"(        this.ws = new WebSocket(this.ws_uri);
)" R"(        this.ws.binaryType = 'arraybuffer';
)" R"(        this.ws.onopen = function(evt) { onopen(evt) };
)" R"(        this.ws.onclose = function(evt) { onclose(evt) };
)" R"(        this.ws.onmessage = function(evt) { onmessage(evt) };
)" R"(        this.ws.onerror = function(evt) { onerror(evt) };
)" R"(
)" R"(        this.t_start_ms = this.timestamp();
)" R"(        this.t_requests_last_update_ms = this.timestamp() - this.k_requests_update_freq_ms;
)" R"(
)" R"(        window.requestAnimationFrame(this.loop.bind(this));
)" R"(    },
)" R"(
)" R"(    loop: function() {
)" R"(        if (this.ws == null) {
)" R"(            if (this.k_auto_reconnect) {
)" R"(                setTimeout(this.init.bind(this), 1000);
)" R"(            }
)" R"(            return;
)" R"(        }
)" R"(
)" R"(        if (this.ws.readyState !== this.ws.OPEN) {
)" R"(            window.requestAnimationFrame(this.loop.bind(this));
)" R"(            return;
)" R"(        }
)" R"(
)" R"(        this.t_frame_begin_ms = this.timestamp();
)" R"(        this.requests_regenerate = this.t_frame_begin_ms - this.t_requests_last_update_ms > this.k_requests_update_freq_ms;
)" R"(
)" R"(        if (this.requests_regenerate) {
)" R"(            this.requests_old = this.requests;
)" R"(            this.requests = [];
)" R"(        }
)" R"(
)" R"(        try {
)" R"(            this.render();
)" R"(        } catch(err) {
)" R"(            this.onerror('Failed to render state: ' + err);
)" R"(        }
)" R"(
)" R"(        if (this.requests_regenerate) {
)" R"(            if (this.requests_new_vars) {
)" R"(                this.send_var_to_id_map();
)" R"(                this.requests_new_vars = false;
)" R"(            }
)" R"(            this.send_requests();
)" R"(            this.t_requests_last_update_ms = this.timestamp();
)" R"(        }
)" R"(
)" R"(        window.requestAnimationFrame(this.loop.bind(this));
)" R"(    },
)" R"(
)" R"(    get: function(path, ...args) {
)" R"(        for (var i = 1; i < arguments.length; i++) {
)" R"(            path = path.replace('%d', arguments[i]);
)" R"(        }
)" R"(
)" R"(        if (!(path in this.vars_map)) {
)" R"(            this.vars_map[path] = new ArrayBuffer();
)" R"(            this.var_to_id[path] = this.nvars;
)" R"(            this.id_to_var[this.nvars] = path;
)" R"(            ++this.nvars;
)" R"(
)" R"(            this.requests_new_vars = true;
)" R"(        }
)" R"(
)" R"(        if (this.requests_regenerate) {
)" R"(            this.requests.push(this.var_to_id[path]);
)" R"(        }
)" R"(
)" R"(        return this.vars_map[path];
)" R"(    },
)" R"(
)" R"(    get_abuf: function(path, ...args) {
)" R"(        return this.get(path, ...args);
)" R"(    },
)" R"(
)" R"(    get_int8: function(path, ...args) {
)" R"(        return this.get_int8_arr(path, ...args)[0];
)" R"(    },
)" R"(
)" R"(    get_int8_arr: function(path, ...args) {
)" R"(        var abuf = this.get(path, ...args);
)" R"(        return new Int8Array(abuf);
)" R"(    },
)" R"(
)" R"(    get_uint8: function(path, ...args) {
)" R"(        return this.get_uint8_arr(path, ...args)[0];
)" R"(    },
)" R"(
)" R"(    get_uint8_arr: function(path, ...args) {
)" R"(        var abuf = this.get(path, ...args);
)" R"(        return new Uint8Array(abuf);
)" R"(    },
)" R"(
)" R"(    get_int16: function(path, ...args) {
)" R"(        return this.get_int16_arr(path, ...args)[0];
)" R"(    },
)" R"(
)" R"(    get_int16_arr: function(path, ...args) {
)" R"(        var abuf = this.get(path, ...args);
)" R"(        return new Int16Array(abuf);
)" R"(    },
)" R"(
)" R"(    get_uint16: function(path, ...args) {
)" R"(        return this.get_uint16_arr(path, ...args)[0];
)" R"(    },
)" R"(
)" R"(    get_uint16_arr: function(path, ...args) {
)" R"(        var abuf = this.get(path, ...args);
)" R"(        return new Uint16Array(abuf);
)" R"(    },
)" R"(
)" R"(    get_int32: function(path, ...args) {
)" R"(        return this.get_int32_arr(path, ...args)[0];
)" R"(    },
)" R"(
)" R"(    get_int32_arr: function(path, ...args) {
)" R"(        var abuf = this.get(path, ...args);
)" R"(        return new Int32Array(abuf);
)" R"(    },
)" R"(
)" R"(    get_uint32: function(path, ...args) {
)" R"(        return this.get_uint32_arr(path, ...args)[0];
)" R"(    },
)" R"(
)" R"(    get_uint32_arr: function(path, ...args) {
)" R"(        var abuf = this.get(path, ...args);
)" R"(        return new Uint32Array(abuf);
)" R"(    },
)" R"(
)" R"(    get_float: function(path, ...args) {
)" R"(        return this.get_float_arr(path, ...args)[0];
)" R"(    },
)" R"(
)" R"(    get_float_arr: function(path, ...args) {
)" R"(        var abuf = this.get(path, ...args);
)" R"(        return new Float32Array(abuf);
)" R"(    },
)" R"(
)" R"(    get_double: function(path, ...args) {
)" R"(        return this.get_double_arr(path, ...args)[0];
)" R"(    },
)" R"(
)" R"(    get_double_arr: function(path, ...args) {
)" R"(        var abuf = this.get(path, ...args);
)" R"(        return new Float64Array(abuf);
)" R"(    },
)" R"(
)" R"(    get_str: function(path, ...args) {
)" R"(        var abuf = this.get(path, ...args);
)" R"(        var enc = new TextDecoder("utf-8");
)" R"(        var res = enc.decode(new Uint8Array(abuf));
)" R"(        var output = "";
)" R"(        for (var i = 0; i < res.length; i++) {
)" R"(            if (res.charCodeAt(i) == 0) break;
)" R"(            if (res.charCodeAt(i) <= 127) {
)" R"(                output += res.charAt(i);
)" R"(            }
)" R"(        }
)" R"(        return output;
)" R"(    },
)" R"(
)" R"(    send: function(msg) {
)" R"(        var enc_msg = new TextEncoder().encode(msg);
)" R"(        var data = new Int8Array(4 + enc_msg.length + 1);
)" R"(        data[0] = 4;
)" R"(        data.set(enc_msg, 4);
)" R"(        data[4 + enc_msg.length] = 0;
)" R"(        this.ws.send(data);
)" R"(
)" R"(        this.stats.tx_n += 1;
)" R"(        this.stats.tx_bytes += data.length;
)" R"(    },
)" R"(
)" R"(    send_var_to_id_map: function() {
)" R"(        var msg = '';
)" R"(        var delim = this.k_var_delim;
)" R"(        for (var key in this.var_to_id) {
)" R"(            var nidxs = 0;
)" R"(            var idxs = delim;
)" R"(            var keyp = key.replace(/\[-?\d*\]/g, function(m) { ++nidxs; idxs += m.replace(/[\[\]]/g, '') + delim; return '[%d]'; });
)" R"(            msg += keyp + delim + this.var_to_id[key].toString() + delim + nidxs + idxs;
)" R"(        }
)" R"(        var data = new Int8Array(4 + msg.length + 1);
)" R"(        var enc = new TextEncoder();
)" R"(        data[0] = 1;
)" R"(        data.set(enc.encode(msg), 4);
)" R"(        data[4 + msg.length] = 0;
)" R"(        this.ws.send(data);
)" R"(
)" R"(        this.stats.tx_n += 1;
)" R"(        this.stats.tx_bytes += data.length;
)" R"(    },
)" R"(
)" R"(    send_requests: function() {
)" R"(        var same = true;
)" R"(        if (this.requests_old === null || this.requests.length !== this.requests_old.length){
)" R"(            same = false;
)" R"(        } else {
)" R"(            for (var i = 0; i < this.requests.length; ++i) {
)" R"(                if (this.requests[i] !== this.requests_old[i]) {
)" R"(                    same = false;
)" R"(                    break;
)" R"(                }
)" R"(            }
)" R"(        }
)" R"(
)" R"(        if (same) {
)" R"(            var data = new Int32Array(1);
)" R"(            data[0] = 3;
)" R"(            this.ws.send(data);
)" R"(
)" R"(            this.stats.tx_n += 1;
)" R"(            this.stats.tx_bytes += data.length;
)" R"(        } else {
)" R"(            var data = new Int32Array(this.requests.length + 1);
)" R"(            data.set(new Int32Array(this.requests), 1);
)" R"(            data[0] = 2;
)" R"(            this.ws.send(data);
)" R"(
)" R"(            this.stats.tx_n += 1;
)" R"(            this.stats.tx_bytes += data.length;
)" R"(        }
)" R"(    },
)" R"(
)" R"(    onopen: function(evt) {
)" R"(    },
)" R"(
)" R"(    onclose: function(evt) {
)" R"(        this.nvars = 0;
)" R"(        this.vars_map = {};
)" R"(        this.var_to_id = {};
)" R"(        this.id_to_var = {};
)" R"(        this.requests = null;
)" R"(        this.requests_old = null;
)" R"(        this.ws = null;
)" R"(    },
)" R"(
)" R"(    onmessage: function(evt) {
)" R"(        this.stats.rx_n += 1;
)" R"(        this.stats.rx_bytes += evt.data.byteLength;
)" R"(
)" R"(        var type_all = (new Uint32Array(evt.data))[0];
)" R"(
)" R"(        if (this.last_data != null && type_all == 1) {
)" R"(            var ntotal = evt.data.byteLength/4 - 1;
)" R"(
)" R"(            var src_view = new Uint32Array(evt.data, 4);
)" R"(            var dst_view = new Uint32Array(this.last_data, 4);
)" R"(
)" R"(            var k = 0;
)" R"(            for (var i = 0; i < ntotal/2; ++i) {
)" R"(                var n = src_view[2*i + 0];
)" R"(                var c = src_view[2*i + 1];
)" R"(                for (var j = 0; j < n; ++j) {
)" R"(                    dst_view[k] = dst_view[k] ^ c;
)" R"(                    ++k;
)" R"(                }
)" R"(            }
)" R"(        } else {
)" R"(            this.last_data = evt.data;
)" R"(        }
)" R"(
)" R"(        var int_view = new Uint32Array(this.last_data);
)" R"(        var offset = 1;
)" R"(        var offset_new = 0;
)" R"(        var total_size = this.last_data.byteLength;
)" R"(        var id = 0;
)" R"(        var type = 0;
)" R"(        var len = 0;
)" R"(        while (4*offset < total_size) {
)" R"(            id = int_view[offset + 0];
)" R"(            type = int_view[offset + 1];
)" R"(            len = int_view[offset + 2];
)" R"(            offset += 3;
)" R"(            offset_new = offset + len/4;
)" R"(            if (type == 0) {
)" R"(                this.vars_map[this.id_to_var[id]] = this.last_data.slice(4*offset, 4*offset_new);
)" R"(            } else {
)" R"(                var src_view = new Uint32Array(this.last_data, 4*offset);
)" R"(                var dst_view = new Uint32Array(this.vars_map[this.id_to_var[id]]);
)" R"(
)" R"(                var k = 0;
)" R"(                for (var i = 0; i < len/8; ++i) {
)" R"(                    var n = src_view[2*i + 0];
)" R"(                    var c = src_view[2*i + 1];
)" R"(                    for (var j = 0; j < n; ++j) {
)" R"(                        dst_view[k] = dst_view[k] ^ c;
)" R"(                        ++k;
)" R"(                    }
)" R"(                }
)" R"(            }
)" R"(            offset = offset_new;
)" R"(        }
)" R"(    },
)" R"(
)" R"(    onerror: function(evt) {
)" R"(        console.error("[incppect]", evt);
)" R"(    },
)" R"(
)" R"(    render: function() {
)" R"(    },
)" R"(}
)" R"()";
