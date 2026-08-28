module.exports = function(minified) {
  var clayConfig = this;
  var _ = minified._;
  var $ = minified.$;
  var HTML = minified.HTML;

    function show_hide_trend() {
        let items = [
            'use_mmol',
            'trend_critical_color',
            'trend_high_color',
            'trend_average_color',
            'trend_good_color',
            'trend_low_color',
            'trend_high_line_color',
            'trend_low_line_color',
            'trend_low',
            'trend_average',
            'trend_high',
            'trend_critical',
            'trend_line_style',
            'trend_line_width',
            'trend_width',
            'trend_style',
            "trend_hour",
            "trend_hour_width",
            "trend_hour_style",
            "trend_auto_adjust_max"            
        ];
        if (this.get()) {
            for (const index in items) {
                console.debug("Disabling " + items[index]);
                let x = clayConfig.getItemByMessageKey(items[index])
                if (x) x.disable();
            }
        } else {
            for (const index in items) {
                console.debug("Enabling " + items[index]);
                let x = clayConfig.getItemByMessageKey(items[index])
                if (x) x.enable();
            }
        }
    };

    var trend_bgl_values = {
        "trend_low": 0,
        "trend_average": 0,
        "trend_high": 0,
        "trend_critical": 0
    };
    var mmol = false;
    const mmol_mgdl = 18.016;

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
        let deprecated = [ "show_delta", "show_trend", "show_slope", "show_unit" ];
        const bgl_names = [ "trend_low", "trend_high", "trend_average", "trend_critical"]; 
        console.debug("Hiding deprecated items");
        for (const index in deprecated) {
            console.debug("Hiding depricated item: " + deprecated[index]);
            clayConfig.getItemByMessageKey(deprecated[index]).hide();
        }

        var usepng = clayConfig.getItemByMessageKey('use_png');
        usepng.on('change', show_hide_trend);
        usepng.trigger('change');

        // convert function
        var convert_bgl = function () {
            for (index in bgl_names) {
                let x = clayConfig.getItemByMessageKey(bgl_names[index]);
                if (x) {
                    if (mmol) {
                        x.$manipulatorTarget.set('min', 2.2);
                        x.$manipulatorTarget.set('max', 22.2);
                        x.$manipulatorTarget.set('step', 0.1);
                        x.precision = 1;
                    } else {
                        x.$manipulatorTarget.set('min', 40);
                        x.$manipulatorTarget.set('max', 400);
                        x.$manipulatorTarget.set('step', 1);
                        x.precision = 0;
                    }
                    // set must be called after manupulatortargets
                    x.set(mmol ? trend_bgl_values[bgl_names[index]] / mmol_mgdl : trend_bgl_values[bgl_names[index]]);
                }
            }
        }
        // console.debug(clayConfig);

        // get mmol/mgdl toggle and convert on change
        let clay_mmol = clayConfig.getItemByMessageKey('use_mmol');
        mmol = clay_mmol.get();
        if (clay_mmol) clay_mmol.on('change', function() {
            mmol = this.get(); 
            console.debug("MMOL: " + mmol);
            convert_bgl();
            // convert all values
        });

        // store trend values
        for (const index in bgl_names) {
            let x = clayConfig.getItemByMessageKey(bgl_names[index]);
            if (x) {
                trend_bgl_values[bgl_names[index]] = parseInt(x.get());
                x.on('change', function() {
                    const name = this.config.messageKey;
                    const newvalue = mmol ? x.get() * mmol_mgdl : x.get();
                    trend_bgl_values[name] = Math.round(newvalue);
                    console.debug(trend_bgl_values);
                });
            }
        }
        console.debug(trend_bgl_values);

        // save trend values as mgdl and set it
        console.debug(clayConfig);
        var submit = clayConfig.getItemsByType('submit')[0];
        submit.on('click', function() {
            mmol = false;
            convert_bgl();
        });

        // run convert if need be
        convert_bgl();

        // hour line masking
        var hours = clayConfig.getItemByMessageKey('trend_hour');
        hours.on('change', function() {
            const width = clayConfig.getItemByMessageKey('trend_hour_width');
            const style = clayConfig.getItemByMessageKey('trend_hour_style');
            if (hours.get()) {
                width.enable();
                style.enable();
            } else {
                width.disable();
                style.disable();
            }
        });
        hours.trigger('change');


        // this is also done by the "selector" values, but there is overlap in those
        if (!clayConfig.meta.activeWatchInfo || 
            clayConfig.meta.activeWatchInfo.platform === 'aplite' || 
            clayConfig.meta.activeWatchInfo.platform === 'flint' || 
            clayConfig.meta.activeWatchInfo.platform === 'diorite') {
            console.debug("Hiding entries for b/w");
            let items = [
                'foreground',
                'background',
                'monochrome',
                'use_mmol',
                'trend_critical_color',
                'trend_high_color',
                'trend_average_color',
                'trend_good_color',
                'trend_low_color',
                'trend_high_line_color',
                'trend_low_line_color',
                'trend_low',
                'trend_average',
                'trend_high',
                'trend_critical',
                "trend_hour",
                "trend_hour_width",
                "trend_hour_style"
            ];
            for (const index in items) {
                console.debug("Hiding " + items[index]);
                let x = clayConfig.getItemByMessageKey(items[index]);
                if (x) x.hide();
            }

        }
    });
};
