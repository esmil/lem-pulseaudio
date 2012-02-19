var state = {},
    output = {
        server:        document.getElementById('server'),
        sink:          document.getElementById('sinks'),
        source:        document.getElementById('sources'),
        card:          document.getElementById('cards'),
        sink_input:    document.getElementById('sink-inputs'),
        source_output: document.getElementById('source-outputs'),
        module:        document.getElementById('modules'),
        client:        document.getElementById('clients'),
        sample_cache:  document.getElementById('samples')
    },
    mute = function(typ, idx, v) {
        $.post('/' + typ + '/' + idx, { mute: v });
    },
    volUp = function(typ, idx) {
        $.post('/' + typ + '/' + idx, { rel: 1000 });
    },
    volDown = function(typ, idx) {
        $.post('/' + typ + '/' + idx, { rel: -1000 });
    },
    kill = function(typ, idx) {
        $.post('/' + typ + '/' + idx, { kill: true });
    },
    unload = function(idx) {
        $.post('/module/' + idx, { unload: true });
    },
    volToPercent = function(vol) {
        return Math.round(100 * vol / 65536);
    },
    addVolumeControl = function(r, typ, idx, mute, vol) {
        var v,
            i, ilen;

        if (vol) {
            for (i = 0, ilen = vol.length; i < ilen; i++) {
                v = vol[i];
                r.push('<p>', v.name, ' ', volToPercent(v.value),
                        '% <div class="progress"><div class="bar" style="width:',
                        volToPercent(v.value), '%"></div></div></p>');
            }
        }

        r.push('<div class="btn-toolbar"><div class="btn-group"><button class="btn');
        if (mute) {
            r.push(' active');
        }
        r.push('" onclick="mute(\'', typ, '\',', idx, ',', !mute,
                ')"><i class="icon-volume-off"></i> Mute</button></div>');
        if (vol) {
            r.push('<div class="btn-group"><button class="btn" onclick="volDown(\'',
                    typ, '\',', idx, ')"><i class="icon-volume-down"></i></button><button class="btn" onclick="volUp(\'',
                    typ, '\',', idx, ')"><i class="icon-volume-up"></i></button></div>');
        }
        r.push('</div>');
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
            var r = [], s,
                i, ilen;

            for (i = 0, ilen = sink.length; i < ilen; i++) {
                s = sink[i]
                r.push('<div class="alert alert-info"><h3>', s.description, '</h3>');
                if (s.base_volume) {
                    r.push('<p>Base Volume: ', volToPercent(s.base_volume), '%</p>');
                }
                addVolumeControl(r, 'sink', s.index, s.mute, s.volume);
                r.push('</div>');
            }
            return r.join('');
        },
        source: function(source) {
            var r = [], s,
                i, ilen;

            for (i = 0, ilen = source.length; i < ilen; i++) {
                s = source[i]
                r.push('<div class="alert alert-info"><h3>', s.description, '</h3>');
                if (s.base_volume) {
                    r.push('<p>Base Volume: ', volToPercent(s.base_volume), '%</p>');
                }
                addVolumeControl(r, 'source', s.index, s.mute, s.volume);
                r.push('</div>');
            }
            return r.join('');
        },
        card: function(card) {
            var r = [], c,
                i, ilen;

            for (i = 0, ilen = card.length; i < ilen; i++) {
                c = card[i];
                r.push('<div class="alert alert-info"><h3>', c.description, '</h3><p>',
                        c.name, '<br/>',
                        c.card_name, '<br/>',
                        c.product_name, '</p></div>');
            }
            return r.join('');
        },
        sink_input: function(sink_input) {
            var r = [], si,
                i, ilen;

            for (i = 0, ilen = sink_input.length; i < ilen; i++) {
                si = sink_input[i];
                r.push('<div class="alert alert-info"><a class="close" onclick="kill(\'sink-input\',',
                        si.index, ')">×</a><h3>', si.name, '</h3>');
                addVolumeControl(r, 'sink-input', si.index, si.mute, si.volume);
                r.push('</div>');
            }
            return r.join('');
        },
        source_output: function(source_output) {
            var r = [], so,
                i, ilen;

            for (i = 0, ilen = source_output.length; i < ilen; i++) {
                so = source_output[i];
                r.push('<div class="alert alert-info"><a class="close" onclick="kill(\'source-output\',',
                        so.index, ')">×</a><h3>', so.name, '</h3>');
                addVolumeControl(r, 'source-output', so.index, so.mute, so.volume);
                r.push('</div>');
            }
            return r.join('');
        },
        module: function(module) {
            var r = [], m,
                i, ilen;

            for (i = 0, ilen = module.length; i < ilen; i++) {
                m = module[i];
                r.push('<div class="alert alert-info"><a class="close" onclick="unload(',
                        m.index, ')">×</a><h3>', m.name, '</h3><p>',
                        m.description, '<br/>',
                        m.author, '<br/>',
                        m.version, '</p></div>');
            }
            return r.join('');
        },
        client: function(client) {
            var r = [], c,
                i, ilen;

            for (i = 0, ilen = client.length; i < ilen; i++) {
                c = client[i];
                r.push('<div class="alert alert-info"><a class="close" onclick="kill(\'client\',',
                        c.index, ')">×</a><h3>', c.name, '</h3><dl><dt>Host</dt><dd>',
                        c.host, '</dd><dt>User</dt><dd>',
                        c.user, '</dd><dt>Binary</dt><dd>',
                        c.binary, '</dd></dl></div>');
            }
            return r.join('');
        },
        sample_cache: function(sample) {
            var r = [], s,
                i, ilen;

            for (i = 0, ilen = sample.length; i < ilen; i++) {
                s = sample[i];
                r.push('<div class="alert alert-info"><h3>', s.description,
                        '</h3></div>');
            }
            return r.join('');
        }
    },
    raw = document.getElementById('raw');

$(function() {
    function onDataReceived(ret) {
        var p, e, f;

        for (p in ret) {
            e = ret[p];
            state[p] = e;
            f = template[p];
            if (f) {
                output[p].innerHTML = f(e);
            }
        }
        //raw.innerHTML = '<![CDATA[' + JSON.stringify(state, null, ' ') + ']]>';
        raw.innerHTML = JSON.stringify(state, null, ' ');
        prettyPrint();
        $.getJSON('/poll/' + ret.stamp, onDataReceived);
    }

    $.getJSON('/poll/0', onDataReceived);
});

// vim: set ts=4 sw=4 et:
