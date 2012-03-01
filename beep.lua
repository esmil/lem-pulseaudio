#!/usr/bin/env lem

local utils = require 'lem.utils'
local pa    = require 'lem.pulseaudio'

local c = assert(pa.connect('LEM PulseAudio Beep'))

local stream = c:stream();

print(stream)

local sin = math.sin
local t, step = 0, 1 / 44100

print("step:", step)

repeat
	local samples = stream:writable_wait()

	print(samples)

	local buf = {}
	for i = 1, samples do
		buf[i] = 10000 * sin(220 * 2 * 3.1415 * t)
		t = t + step
	end

	stream:write(buf)
until t > 10

assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
