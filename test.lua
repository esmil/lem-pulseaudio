#!/usr/bin/env lem

package.path = '?.lua;' .. package.path
package.cpath = '?.so;' .. package.cpath

local utils = require 'lem.utils'
local pa    = require 'lem.pulseaudio'

local c = assert(pa.connect('LEM PulseAudio TestApp'))

local function dump_info(header, info)
	print()
	print(header .. ' info list:')

	for k, v in pairs(info) do
		print(k)

		if v.description then
			print('', 'description', v.description)
		end
		if v.name then
			print('', 'name', v.name)
		end
		if v.index then
			print('', 'index', v.index)
		end

		for k, v in pairs(v) do
			if k ~= 'description'
				and k ~= 'name'
				and k ~= 'index'
				and k ~= 'proplist' then
				print('', k, v)
			end
		end

		if v.proplist then
			print('', 'proplist:')
			for k, v in pairs(v.proplist) do
				print('', '', k, v)
			end
		end
	end
end

print "Server info:"
for k, v in pairs(assert(c:server_info())) do
	print(k, v)
end

dump_info('Sink',   assert(c:sink_info()))
dump_info('Source', assert(c:source_info()))
dump_info('Module', assert(c:module_info()))
dump_info('Client', assert(c:client_info()))
dump_info('Card',   assert(c:card_info()))
dump_info('Sink',   assert(c:sink_input_info()))
dump_info('Source', assert(c:source_output_info()))
dump_info('Sample', assert(c:sample_info()))

print "\nStat:"
for k, v in pairs(assert(c:stat())) do
	print(k, v)
end

--[[
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

print()
for i = 0, 20 do
	print(pa.position_name[i])
end
--]]

assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
