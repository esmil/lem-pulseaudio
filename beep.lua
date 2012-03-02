#!/usr/bin/env lem

local utils = require 'lem.utils'
local pa    = require 'lem.pulseaudio'

local c = assert(pa.connect('LEM PulseAudio Beep'))

local rate = 48000
local s = assert(c:stream('Beep!', 'float32le', rate, 1))
assert(s:connect_playback())

local sin, tau = math.sin, 2*math.pi
local fmod, floor = math.fmod, math.floor
local t, step = 0, 1/rate

repeat
	local samples = assert(s:writable_wait())

	print(samples)

	local buf = {}
	for i = 1, samples do
		--[[ Sinus
		buf[i] = sin(440 * tau * t)
		--]]
		--[[ Firkant
		buf[i] = 2*floor(2*fmod(440*t, 1)) - 1
		--]]
		--[[ Savtak
		buf[i] = 2*fmod(440*t, 1) - 1
		--]]
		t = t + step
	end

	s:write(buf)
until t > 10

print "draining.."
assert(s:drain())
assert(s:disconnect())
assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
