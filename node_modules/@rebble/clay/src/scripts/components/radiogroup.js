'use strict';

module.exports = {
  name: 'radiogroup',
  template: require('../../templates/components/radiogroup.tpl'),
  style: require('../../../tmp/radiogroup.css'),
  manipulator: 'radiogroup',
  defaults: {
    label: '',
    options: [],
    serializeValueAs: 'string',
    description: '',
    attributes: {}
  }
};
