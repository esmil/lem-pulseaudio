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
	local sort = table.sort
	local function list(t, res, f)
		local idx, n = {}, 0
		for _, v in pairs(t) do
			n = n + 1
			idx[n] = v
		end
		if n == 0 then res:add('[]') return end
		sort(idx, function(a, b) return a.index < b.index end)
		res:add('[')
		f(idx[1], res)
		for i = 2, n do
			res:add(',')
			f(idx[i], res)
		end
		res:add(']')
	end

	local now = gettimeofday()
	state = {
		server = {
			stamp  = now,
			data   = assert(c:server_info()),
			update = function(self)
				self.data = assert(c:server_info())
				self.stamp = gettimeofday()
			end,
			tojson = function(self, res)
				local t = self.data
				res:add('{"server_name":"%s",\z
					"server_version":"%s",\z
					"host_name":"%s",\z
					"user_name":"%s"}',
					t.server_name,
					t.server_version,
					t.host_name,
					t.user_name)
			end,
		},
		sink = {
			stamp  = now,
			data   = assert(c:sink_info()),
			update = function(self, typ, idx)
				if typ == 'remove' then
					self.data[idx] = nil
				else
					self.data[idx] = c:sink_info(idx)
				end
				self.stamp = gettimeofday()
			end,
			tojson = function(self, res)
				list(self.data, res, function(t, res)
					res:add('{\z
						"index":%u,\z
						"description":"%s",\z
						"mute":%s,',
						t.index, t.description, t.mute)
					local volume = t.volume
					if volume then
						res:add('"volume":[%u', volume[1])
						for i = 2, #volume do
							res:add(',%u', volume[i])
						end
						res:add('],')
					end
					res:add('"base_volume":%u}', t.base_volume)
				end)
			end,
		},
		source = {
			stamp  = now,
			data   = assert(c:source_info()),
			update = function(self, typ, idx)
				if typ == 'remove' then
					self.data[idx] = nil
				else
					self.data[idx] = c:source_info(idx)
				end
				self.stamp = gettimeofday()
			end,
			tojson = function(self, res)
				list(self.data, res, function(t, res)
					res:add('{\z
						"index":%u,\z
						"description":"%s",\z
						"mute":%s,',
						t.index, t.description, t.mute)
					if t.monitor_of_sink then
						res:add('"monitor_of_sink":%u,', t.monitor_of_sink)
					end
					local volume = t.volume
					if volume then
						res:add('"volume":[%u', volume[1])
						for i = 2, #volume do
							res:add(',%u', volume[i])
						end
						res:add('],')
					end
					res:add('"base_volume":%u}', t.base_volume)
				end)
			end,
		},
		sink_input = {
			stamp  = now,
			data   = assert(c:sink_input_info()),
			update = function(self, typ, idx)
				if typ == 'remove' then
					self.data[idx] = nil
				else
					self.data[idx] = c:sink_input_info(idx)
				end
				self.stamp = gettimeofday()
			end,
			tojson = function(self, res)
				list(self.data, res, function(t, res)
					res:add('{\z
						"index":%u,\z
						"name":"%s",\z
						"mute":%s,\z
						"has_volume":%s,',
						t.index, t.name,
						t.mute, t.has_volume)
					local volume = t.volume
					if volume then
						res:add('"volume":[%u', volume[1])
						for i = 2, #volume do
							res:add(',%u', volume[i])
						end
						res:add('],')
					end
					res:add('"corked":%s}', t.corked)
				end)
			end,
		},
		source_output = {
			stamp  = now,
			data   = assert(c:source_output_info()),
			update = function(self, typ, idx)
				if typ == 'remove' then
					self.data[idx] = nil
				else
					self.data[idx] = c:source_output_info(idx)
				end
				self.stamp = gettimeofday()
			end,
			tojson = function(self, res)
				list(self.data, res, function(t, res)
					res:add('{}')
				end)
			end,
		},
		sample = {
			stamp  = now,
			data   = assert(c:sample_info()),
			update = function(self, typ, idx)
				if typ == 'remove' then
					self.data[idx] = nil
				else
					self.data[idx] = c:sample_info(idx)
				end
				self.stamp = gettimeofday()
			end,
			tojson = function(self, res)
				list(self.data, res, function(t, res)
					res:add('{}')
				end)
			end,
		},
		module = {
			stamp  = now,
			data   = assert(c:module_info()),
			update = function(self, typ, idx)
				if typ == 'remove' then
					self.data[idx] = nil
				else
					self.data[idx] = c:module_info(idx)
				end
				self.stamp = gettimeofday()
			end,
			tojson = function(self, res)
				list(self.data, res, function(t, res)
					res:add('{\z
						"index":%u,\z
						"name":"%s",\z
						"description":"%s",\z
						"author":"%s",\z
						"version":"%s"}',
						t.index, t.name,
						t.proplist['module.description'],
						t.proplist['module.author'],
						t.proplist['module.version'])
				end)
			end,
		},
		client = {
			stamp  = now,
			data   = assert(c:client_info()),
			update = function(self, typ, idx)
				if typ == 'remove' then
					self.data[idx] = nil
				else
					self.data[idx] = c:client_info(idx)
				end
				self.stamp = gettimeofday()
			end,
			tojson = function(self, res)
				list(self.data, res, function(t, res)
					res:add('{\z
						"index":%u,\z
						"name":"%s"}',
						t.index, t.name)
				end)
			end,
		},
	}
end

utils.spawn(function()
	while true do
		local src, typ, idx = assert(c:get_event())

		print(src, typ, idx)
		state[src]:update(typ, idx)

		-- wake up long-polling clients
		for sleeper, _ in pairs(queue) do
			sleeper:wakeup()
		end
	end
end)

hathaway.import()

GET('/index.html', function(req, res)
	res.headers['Content-Type'] = 'text/html; charset=UTF-8'
	res.file = 'index.html'
end)

MATCH('^/sink/(%d+)/volume/(%d+)$', function(req, res, index, vol)
	index = tonumber(index)
	vol = tonumber(vol)
	c:set_sink_volume(index, { vol, vol })
end)

MATCH('^/sink/(%d+)/mute$', function(req, res, index)
	index = tonumber(index)
	c:set_sink_mute(index)
end)

MATCH('^/sink/(%d+)/unmute$', function(req, res, index)
	index = tonumber(index)
	c:set_sink_unmute(index)
end)

MATCH('^/module/(%d+)/unload$', function(req, res, index)
	index = tonumber(index)
	c:unload_module(index)
end)

MATCH('^/(js/.+)$', function(req, res, file)
	if req.method ~= 'GET' and req.method ~= 'HEAD' then
		return hathaway.method_not_allowed(req, res)
	end
	res.headers['Content-Type'] = 'text/javascript; charset=UTF-8'
	res.file = file
end)

MATCH('^/(css/.+)$', function(req, res, file)
	if req.method ~= 'GET' and req.method ~= 'HEAD' then
		return hathaway.method_not_allowed(req, res)
	end
	res.headers['Content-Type'] = 'text/css; charset=UTF-8'
	res.file = file
end)

local function addchanged(res, stamp)
	local ret = false
	for k, v in pairs(state) do
		if v.stamp > stamp then
			res:add('"%s":', k)
			v:tojson(res)
			res:add(',')
			ret = true
		end
	end
	return ret
end

MATCH('^/poll/(%d+%.?%d*)$', function(req, res, stamp)
	if req.method ~= 'GET' and req.method ~= 'HEAD' then
		return hathaway.method_not_allowed(req, res)
	end
	res.headers['Content-Type'] = 'text/javascript; charset=UTF-8'
	res.headers['Cache-Control'] = 'max-age=0, must-revalidate'
	res:add('{')
	stamp = tonumber(stamp)
	if not addchanged(res, stamp) then
		local sleeper = utils.sleeper()
		queue[sleeper] = true
		sleeper:sleep(60)
		queue[sleeper] = nil
		addchanged(res, stamp)
	end
	res:add('"stamp":"%.4f"}', gettimeofday())
end)

hathaway.debug = print
Hathaway('*', arg[1] or 8080)

assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
