#!/usr/bin/env lem

local utils        = require 'lem.utils'
local gettimeofday = require 'gettimeofday'
local hathaway     = require 'lem.hathaway'
local pa           = require 'lem.pulseaudio'

local assert = assert

local c = assert(pa.connect('LEM PulseAudio TestApp'))

local queue = {}

assert(c:subscribe(pa.mask.ALL))
local state
do
	local now = gettimeofday()
	state = {
		server        = { stamp = now, data = assert(c:server_info()) },
		sink          = { stamp = now, data = assert(c:sink_info()) },
		source        = { stamp = now, data = assert(c:source_info()) },
		sink_input    = { stamp = now, data = assert(c:sink_input_info()) },
		source_output = { stamp = now, data = assert(c:source_output_info()) },
		sample        = { stamp = now, data = assert(c:sample_info()) },
		module        = { stamp = now, data = assert(c:module_info()) },
		client        = { stamp = now, data = assert(c:client_info()) },
	}
end

utils.spawn(function()
	local getinfo = {
		sink          = function(c, idx) return c:sink_info(idx) end,
		source        = function(c, idx) return c:source_info(idx) end,
		sink_input    = function(c, idx) return c:sink_input_info(idx) end,
		source_output = function(c, idx) return c:source_output_info(idx) end,
		sample        = function(c, idx) return c:sample_info(idx) end,
		module        = function(c, idx) return c:module_info(idx) end,
		client        = function(c, idx) return c:client_info(idx) end,
	}

	while true do
		local src, typ, idx = assert(c:get_event())

		print(src, typ, idx)
		local t = state[src]
		if src == 'server' then
			t.data = c:server_info()
		else
			if typ == 'remove' then
				t.data[idx] = nil
			else
				t.data[idx] = getinfo[src](c, idx)
			end
		end
		t.stamp = gettimeofday()

		-- wake up long-polling clients
		for sleeper, _ in pairs(queue) do
			sleeper:wakeup()
		end
	end
end)

local function json_addobj(res, t)
	res:add('{')
	local f, s, var = pairs(t)
	local k, v = f(s, var)
	if k then
		var = k
		local typ
		res:add('"%s":', k)
		typ = type(v)
		if typ == 'number' then
			res:add('%d', v)
		elseif typ == 'boolean' then
			res:add('%s', v)
		else
			res:add('"%s"', tostring(v):gsub('"', '\\"'))
		end
		while true do
			k, v = f(s, var)
			if k == nil then break end
			var = k
			res:add(',"%s":', k)
			typ = type(v)
			if typ == 'number' then
				res:add('%d', v)
			elseif typ == 'boolean' then
				res:add('%s', v)
			else
				res:add('"%s"', tostring(v):gsub('"', '\\"'))
			end
		end
	end
	res:add('}')
end

local function json_addlist(res, t)
	local idx, n = {}, 0
	for k, v in pairs(t) do
		n = n + 1
		idx[n] = v
	end
	res:add('[')
	if n > 0 then
		table.sort(idx, function(a, b) return a.index < b.index end)

		json_addobj(res, idx[1])
		for i = 2, n do
			res:add(',')
			json_addobj(res, idx[i])
		end
	end
	res:add(']')
end

local function json_addchanged(res, stamp)
	local ret = false
	for k, v in pairs(state) do
		if v.stamp > stamp then
			res:add('"%s":', k)
			if k == 'server' then
				json_addobj(res, v.data)
			else
				json_addlist(res, v.data)
			end
			res:add(',')
			ret = true
		end
	end
	return ret
end

local function sendfile(content, path)
	return function(req, res)
		res.headers['Content-Type'] = content
		res.file = path
	end
end

hathaway.import()

GET('/index.html', sendfile('text/html; charset=UTF-8',       'index.html'))
GET('/jquery.js',  sendfile('text/javascript; charset=UTF-8', 'jquery-1.7.1.min.js'))
GET('/json2.js',   sendfile('text/javascript; charset=UTF-8', 'json2.js'))

MATCH('/poll/(%d+%.?%d*)', function(req, res, stamp)
	if req.method ~= 'GET' and req.method ~= 'HEAD' then
		return hathaway.method_not_allowed(req, res)
	end
	res.headers['Content-Type'] = 'text/javascript; charset=UTF-8'
	res.headers['Cache-Control'] = 'max-age=0, must-revalidate'
	res:add('{')
	stamp = tonumber(stamp)
	if not json_addchanged(res, stamp) then
		local sleeper = utils.sleeper()
		queue[sleeper] = true
		sleeper:sleep(60)
		queue[sleeper] = nil
		json_addchanged(res, stamp)
	end
	res:add('"stamp":"%.4f"}', gettimeofday())
end)

hathaway.debug = print
Hathaway('*', arg[1] or 8080)

assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
