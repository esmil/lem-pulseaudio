#!/usr/bin/env lem

local utils = require 'lem.utils'
local pa    = require 'lem.pulseaudio'

local c = assert(pa.connect('LEM PulseAudio Beep'))

local rate = 48000
local s = assert(c:stream('Beep!', 's16le', rate, 1))
assert(s:connect_playback())

local sin, tau = math.sin, 2*math.pi
local t, step = 0, 1/rate

repeat
	local samples = assert(s:writable_wait())

	print(samples)

	local buf = {}
	for i = 1, samples do
		local tt = tau*t
		buf[i] = 10000 * sin(440 * (tt + 0.05*sin(4*tt)))
		t = t + step
	end

	s:write(buf)
until t > 10

print "draining.."
assert(s:drain())
assert(s:disconnect())
assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
