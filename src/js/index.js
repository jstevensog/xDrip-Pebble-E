var Clay = require('@rebble/clay');
var messageKeys = require('message_keys');
var clayConfig = require('./config.json');
var clayCustom = require('./custom.js');
var clay = new Clay(clayConfig, clayCustom, { autoHandleEvents: false });

// avoid having to deal with am/pm, let claude output a converter function (should be in 24h format but who knows)
function toMinutes(str) {
    var m = /^\s*(\d{1,2})(?::(\d{2}))?(?::\d{2})?\s*([ap])?\.?m?\.?\s*$/i.exec(String(str));
    if (!m) return null;

    var h = parseInt(m[1], 10);
    var min = m[2] ? parseInt(m[2], 10) : 0;
    var ap = m[3] ? m[3].toLowerCase() : null;

    if (ap) {
        if (h < 1 || h > 12) return null;
        h = (h % 12) + (ap === 'p' ? 12 : 0);   // 12am -> 0, 12pm -> 12
    }
    if (h > 23 || min > 59) return null;
    return h * 60 + min;
}

Pebble.addEventListener('showConfiguration', function() {
    Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
    if (!e || !e.response) return;
    var values = clay.getSettings(e.response);

    ["night_window_start", "night_window_end"].forEach(function(name) {
        let id = messageKeys[name];
        values[id] = toMinutes(values[id]);
    });

    Pebble.sendAppMessage(values, function(e) {
        console.log('Sent config data to Pebble');
    }, function(e) {
        console.log('Failed to send config data!');
        console.log(JSON.stringify(e));
    });
});
