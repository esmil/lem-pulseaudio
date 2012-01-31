#!/usr/bin/env lem

local utils = require 'lem.utils'
local pa    = require 'lem.pulseaudio'

local c = assert(pa.connect('LEM PulseAudio TestApp'))

print "Server info:"
for k, v in pairs(assert(c:server_info())) do
	print(k, v)
end

print "\nStat:"
for k, v in pairs(assert(c:stat())) do
	print(k, v)
end

print "\nSink info list:"
for k, sink in pairs(assert(c:sink_info())) do
	print(k)
	for k, v in pairs(sink) do
		print('', k, v)
	end
end

print "\nSource info list:"
for k, source in pairs(assert(c:source_info())) do
	print(k)
	for k, v in pairs(source) do
		print('', k, v)
	end
end

print "\nSink input info list:"
for k, input in pairs(assert(c:sink_input_info())) do
	print(k)
	for k, v in pairs(input) do
		print('', k, v)
	end
end

print "\nSource output info list:"
for k, output in pairs(assert(c:source_output_info())) do
	print(k)
	for k, v in pairs(output) do
		print('', k, v)
	end
end

print "\nSample info list:"
for k, sample in pairs(assert(c:sample_info())) do
	print(k)
	for k, v in pairs(sample) do
		print('', k, v)
	end
end

print "\nModule info list:"
for k, module in pairs(assert(c:module_info())) do
	print(k)
	for k, v in pairs(module) do
		print('', k, v)
	end
end

print "\nClient info list:"
for k, client in pairs(assert(c:client_info())) do
	print(k)
	for k, v in pairs(client) do
		print('', k, v)
	end
end

assert(c:subscribe(pa.mask.ALL))
local lookup = {
	sink          = function(c, typ, idx) return c:sink_info(idx) end,
	source        = function(c, typ, idx) return c:source_info(idx) end,
	sink_input    = function(c, typ, idx) return c:sink_input_info(idx) end,
	source_output = function(c, typ, idx) return c:source_output_info(idx) end,
	sample        = function(c, typ, idx) return c:sample_info(idx) end,
	module        = function(c, typ, idx) return c:module_info(idx) end,
	client        = function(c, typ, idx) return c:client_info(idx) end,
}

while true do
	local src, typ, idx = assert(c:get_event())

	print(src, typ, idx)
	if typ ~= 'remove' then
		local handler = lookup[src]
		if handler then
			for k, v in pairs(assert(handler(c, typ, idx))) do
				print(k, v)
			end
		end
	end
end

assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
