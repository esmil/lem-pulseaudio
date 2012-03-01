#!/usr/bin/env lem

local utils = require 'lem.utils'
local pa    = require 'lem.pulseaudio'

local c = assert(pa.connect('LEM PulseAudio Beep'))

local s = assert(c:stream('Beep!'))
assert(s:connect_playback())

local sin, pi = math.sin, math.pi
local t, step = 0, 1/44100

repeat
	local samples = assert(s:writable_wait())

	print(samples)

	local buf = {}
	for i = 1, samples, 2 do
		local tt = 2*pi*t
		local v = 30000 * sin(440 * (tt + 0.05 * sin(4*tt)))
		buf[i] = v
		buf[i+1] = v
		t = t + step
	end

	s:write(buf)
until t > 10

assert(s:disconnect())
assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
