#!/usr/bin/env lem

local utils = require 'lem.utils'
local pa    = require 'lem.pulseaudio'

local c = assert(pa.connect('LEM PulseAudio Beep'))

print(c:beep())

utils.sleeper():sleep(10)

assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
