var state = {},
    output = {
        server:        document.getElementById('server'),
        sink_input:    document.getElementById('sink-inputs'),
        source_output: document.getElementById('source-outputs'),
        sink:          document.getElementById('sinks'),
        source:        document.getElementById('sources'),
        card:          document.getElementById('cards'),
        client:        document.getElementById('clients'),
        module:        document.getElementById('modules'),
        sample_cache:  document.getElementById('samples')
    },
    domify = function(t) {
        var e = document.createElement(t.name),
            i, ilen;

        if (t.attr) {
            for (i in t.attr) {
                e.setAttribute(i, t.attr[i]);
            }
        }
        if (t.child) {
            for (i = 0, ilen = t.child.length; i < ilen; i++) {
                e.appendChild(domify(t.child[i]));
            }
        }
        if (t.text) {
            e.innerText = t.text;
        }
        return e;
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
        var x, i, ilen;

        if (vol) {
            for (i = 0, ilen = vol.length; i < ilen; i++) {
                x = vol[i];

                r.push({ name: 'p', text: x.name + ' ' + volToPercent(x.value) + '%' });
                r.push({
                    name: 'div', attr: { 'class': 'progress' },
                    child: [{
                        name: 'div',
                        attr: { 'class': 'bar', 'style': 'width:' + volToPercent(x.value) + '%' }
                    }],
                });
            }
        }

        x = [{
            name: 'div', attr: { 'class': 'btn-group' },
            child: [{
                name: 'button',
                attr: {
                    'class': (mute ? 'btn active' : 'btn'),
                    'onclick': 'mute(\'' + typ + '\',' + idx + ',' + !mute + ')',
                },
                child: [
                    { name: 'i', attr: { 'class': 'icon-volume-off' } },
                    { name: 'span', text: ' Mute' },
                ]
            }],
        }];

        if (vol) {
            x.push({
                name: 'div', attr: { 'class': 'btn-group' },
                child: [
                    {
                        name: 'button',
                        attr: {
                            'class': 'btn',
                            'onclick': 'volDown(\'' + typ + '\',' + idx + ')',
                        },
                        child: [
                            { name: 'i', attr: { 'class': 'icon-volume-down' } },
                        ]
                    }, {
                        name: 'button',
                        attr: {
                            'class': 'btn',
                            'onclick': 'volUp(\'' + typ + '\',' + idx + ')',
                        },
                        child: [
                            { name: 'i', attr: { 'class': 'icon-volume-up' } },
                        ]
                    }
                ],
            });
        }

        r.push({
            name: 'div', attr: { 'class': 'btn-toolbar' },
            child: x,
        });
    },
    template = {
        server: function(s) {
            return {
                name: 'dl',
                child: [
                    { name: 'dt', text: 'Server Name' },    { name: 'dd', text: s.server_name },
                    { name: 'dt', text: 'Server Version' }, { name: 'dd', text: s.server_version },
                    { name: 'dt', text: 'Host Name' },      { name: 'dd', text: s.host_name },
                    { name: 'dt', text: 'User Name' },      { name: 'dd', text: s.user_name },
                ],
            };
        },
        sink_input: function(si) {
            var b = [
                {
                    name: 'a',
                    attr: {
                        'class': 'close',
                        'onclick': 'kill(\'sink-input\',' + si.index + ')',
                    },
                },
                { name: 'h3', text: si.name },
            ];
            addVolumeControl(b, 'sink-input', si.index, si.mute, si.volume);

            return {
                name: 'div',
                attr: { 'class': 'alert alert-info' },
                child: b,
            };
        },
        source_output: function(so) {
            var b = [
                {
                    name: 'a',
                    attr: {
                        'class': 'close',
                        'onclick': 'kill(\'source-output\',' + so.index + ')',
                    },
                },
                { name: 'h3', text: so.name },
            ];
            addVolumeControl(b, 'source-output', so.index, so.mute, so.volume);

            return {
                name: 'div',
                attr: { 'class': 'alert alert-info' },
                child: b,
            };
        },
        sink: function(s) {
            var b = [ { name: 'h3', text: s.description } ];

            if (s.base_volume) {
                b.push({ name: 'p', text: 'Base Volume: ' + volToPercent(s.base_volume) + '%' });
            }
            addVolumeControl(b, 'sink', s.index, s.mute, s.volume);

            return {
                name: 'div',
                attr: { 'class': 'alert alert-info' },
                child: b,
            };
        },
        source: function(s) {
            var b = [ { name: 'h3', text: s.description } ];

            if (s.base_volume) {
                b.push({ name: 'p', text: 'Base Volume: ' + volToPercent(s.base_volume) + '%' });
            }
            addVolumeControl(b, 'sink', s.index, s.mute, s.volume);

            return {
                name: 'div',
                attr: { 'class': 'alert alert-info' },
                child: b,
            };
        },
        card: function(c) {
            return {
                name: 'div',
                attr: { 'class': 'alert alert-info' },
                child: [
                    { name: 'h3', text: c.description },
                    { name: 'p',  text: c.name },
                    { name: 'p',  text: c.card_name },
                    { name: 'p',  text: c.product_name },
                ],
            };
        },
        client: function(c) {
            return {
                name: 'div',
                attr: { 'class': 'alert alert-info' },
                child: [
                    {
                        name: 'a',
                        attr: {
                            'class': 'close',
                            'onclick': 'kill(\'client\',' + c.index + ')',
                        },
                        text: '×',
                    }, {
                        name: 'h3',
                        text: c.name,
                    }, {
                        name: 'dl',
                        child: [
                            { name: 'dt', text: 'Host' },   { name: 'dd', text: c.host },
                            { name: 'dt', text: 'User' },   { name: 'dd', text: c.user },
                            { name: 'dt', text: 'Binary' }, { name: 'dd', text: c.binary },
                        ],
                    },
                ],
            };
        },
        module: function(m) {
            return {
                name: 'div',
                attr: { 'class': 'alert alert-info' },
                child: [
                    {
                        name: 'a',
                        attr: {
                            'class': 'close',
                            'onclick': 'unload(' + m.index + ')',
                        },
                        text: '×',
                    },
                    { name: 'h3', text: m.name },
                    { name: 'p',  text: m.description },
                    { name: 'p',  text: m.author },
                    { name: 'p',  text: m.version },
                ],
            };
        },
        sample_cache: function(s) {
            return {
                name: 'div',
                attr: { 'class': 'alert alert-info' },
                child: [ { name: 'h3', text: s.description } ],
            };
        }
    },
    raw = document.getElementById('raw');

$(function() {
    function onDataReceived(ret) {
        var p, e, el, f, i, ilen;

        for (p in ret) {
            e = ret[p];
            state[p] = e;
            f = template[p];
            if (f) {
                el = output[p];
                el.innerHTML = '';
                if (e.constructor === Array) {
                    for (i = 0, ilen = e.length; i < ilen; i++) {
                        el.appendChild(domify(f(e[i])));
                    }
                } else {
                    el.appendChild(domify(f(e)));
                }
            }
        }
        raw.innerText = JSON.stringify(state, null, ' ');
        prettyPrint();
        $.getJSON('/poll/' + ret.stamp, onDataReceived);
    }

    $.getJSON('/poll/0', onDataReceived);
});

// vim: set ts=4 sw=4 et:
