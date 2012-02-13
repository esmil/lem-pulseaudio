var state = {},
    output = {
        server:        document.getElementById('server'),
        sink:          document.getElementById('sinks'),
        source:        document.getElementById('sources'),
        sink_input:    document.getElementById('sink-inputs'),
        source_output: document.getElementById('source-outputs'),
        sample:        document.getElementById('samples'),
        module:        document.getElementById('modules'),
        client:        document.getElementById('clients')
    },
    volToPercent = function(vol) {
        return Math.round(100 * vol / 65536);
    },
    template = {
        server: function(s) {
            return '<dl><dt>Server Name</dt><dd>' + s.server_name + '</dd>'
                + '<dt>Server Version</dt><dd>' + s.server_version + '</dd>'
                + '<dt>Host Name</dt><dd>' + s.host_name + '</dd>'
                + '<dt>User Name</dt><dd>' + s.user_name + '</dd>'
                + '</dl>';
        },
        sink: function(sink) {
            var r = [], s, v,
                i, ilen,
                j, jlen;

            for (i = 0, ilen = sink.length; i < ilen; i++) {
                s = sink[i]
                r.push('<h3>', s.description, '</h3><p>Volume:');
                for (j = 0, jlen = s.volume.length; j < jlen; j++) {
                    v = s.volume[j];
                    r.push(' ', v.name, ':', volToPercent(v.value), '%');
                }
                r.push('</p>');
                for (j = 0, jlen = s.volume.length; j < jlen; j++) {
                    v = s.volume[j];
                    r.push('<div id="sink-', s.index, '-', v.name, '"></div>');
                }
                if (s.base_volume) {
                    r.push('<p>Base Volume: ', volToPercent(s.base_volume), '%</p>');
                }
            }
            return r.join('');
        },
        source: function(source) {
            var r = [], s, v,
                i, ilen,
                j, jlen;

            for (i = 0, ilen = source.length; i < ilen; i++) {
                s = source[i]
                r.push('<h3>', s.description, '</h3><p>Volume:');
                for (j = 0, jlen = s.volume.length; j < jlen; j++) {
                    v = s.volume[j];
                    r.push(' ', v.name, ':', volToPercent(v.value), '%');
                }
                r.push('</p>');
                for (j = 0, jlen = s.volume.length; j < jlen; j++) {
                    v = s.volume[j];
                    r.push('<div id="source-', s.index, '-', v.name, '"></div>');
                }
                if (s.base_volume) {
                    r.push('<p>Base Volume: ', volToPercent(s.base_volume), '%</p>');
                }
            }
            return r.join('');
        },
        sink_input: function(sink_input) {
            var r = [], si, v,
                i, ilen,
                j, jlen;

            for (i = 0, ilen = sink_input.length; i < ilen; i++) {
                si = sink_input[i];
                r.push('<h3>', si.name, '</h3>');
                if (si.volume) {
                    r.push('<p>Volume:');
                    for (j = 0, jlen = si.volume.length; j < jlen; j++) {
                        v = si.volume[j];
                        r.push(' ', v.name, ':', v.value);
                    }
                    r.push('</p>');
                    for (j = 0, jlen = si.volume.length; j < jlen; j++) {
                        v = si.volume[j];
                        r.push('<div id="sink-input-', si.index, '-', v.name, '"></div>');
                    }
                }
            }
            return r.join('');
        },
        source_output: function(source_output) {
            var r = [], so,
                i, ilen;

            for (i = 0, ilen = source_output.length; i < ilen; i++) {
                so = source_output[i];
                r.push('<h3>', so.description, '</h3>');
            }
            return r.join('');
        },
        sample: function(sample) {
            var r = [], s,
                i, ilen;

            for (i = 0, ilen = sample.length; i < ilen; i++) {
                s = sample[i];
                r.push('<h3>', s.description, '</h3>');
            }
            return r.join('');
        },
        module: function(module) {
            var r = [], m,
                i, ilen;

            for (i = 0, ilen = module.length; i < ilen; i++) {
                m = module[i];
                r.push('<h3>', m.name, '</h3><p>',
                    m.description, '<br />',
                    m.author, '<br />',
                    m.version, '</p>');
            }
            return r.join('');
        },
        client: function(client) {
            var r = [], c,
                i, ilen;

            for (i = 0, ilen = client.length; i < ilen; i++) {
                c = client[i];
                r.push('<h3>', c.name, '</h3>');
            }
            return r.join('');
        }
    },
    slider_callback = function(typ, idx, name) {
        return function(event, ui) {
            var obj = {};
            obj[name] = ui.value;
            $.post('/' + typ + '/' + idx, obj);
        };
    },
    render = {
        sink: function(sink) {
            var s, volume, v,
                i, ilen,
                j, jlen;

            for (i = 0, ilen = sink.length; i < ilen; i++) {
                s = sink[i];
                volume = s.volume;
                for (j = 0, jlen = volume.length; j < jlen; j++) {
                    v = volume[j];
                    $('#sink-' + s.index + '-' + v.name).slider({
                        min: 0,
                        max: 65536,
                        value: v.value,
                        change: slider_callback('sink', s.index, v.name),
                    });
                }
            }
        },
        source: function(source) {
            var s, v,
                i, ilen,
                j, jlen;

            for (i = 0, ilen = source.length; i < ilen; i++) {
                s = source[i];
                for (j = 0, jlen = s.volume.length; j < jlen; j++) {
                    v = s.volume[j];
                    $('#source-' + s.index + '-' + v.name).slider({
                        min: 0,
                        max: 65536,
                        value: v.value,
                        change: slider_callback('source', s.index, v.name),
                    });
                }
            }
        },
        sink_input: function(sink_input) {
            var si, volume, v,
                i, ilen,
                j, jlen;

            for (i = 0, ilen = sink_input.length; i < ilen; i++) {
                si = sink_input[i];
                volume = si.volume;
                for (j = 0, jlen = volume.length; j < jlen; j++) {
                    v = volume[j];
                    $('#sink-input-' + si.index + '-' + v.name).slider({
                        min: 0,
                        max: 65536,
                        value: v.value,
                        change: slider_callback('sink-input', si.index, v.name),
                    });
                }
            }
        }
    },
    raw = document.getElementById('raw');

$(function() {
    $("#tabs").tabs();

    function onDataReceived(ret) {
        var p, e, f;

        for (p in ret) {
            e = ret[p];
            state[p] = e;
            f = template[p];
            if (f) {
                output[p].innerHTML = f(e);
            }
            f = render[p];
            if (f) {
                f(e);
            }
        }
        raw.innerHTML = '<![CDATA[' + JSON.stringify(state, null, ' ') + ']]>';
        $.getJSON('/poll/' + ret.stamp, onDataReceived);
    };

    $.getJSON('/poll/0', onDataReceived);
});

// vim: set ts=4 sw=4 et:
