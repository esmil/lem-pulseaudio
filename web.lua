#!/usr/bin/env lem

local utils        = require 'lem.utils'
local gettimeofday = require 'gettimeofday'
local hathaway     = require 'lem.hathaway'
local pa           = require 'lem.pulseaudio'

local assert, tonumber = assert, tonumber

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

	local function volume_add(volume, map, res)
		if not volume then return end

		res:add('"volume":[{"name":"%s","value":%u}',
			pa.position_name[map[1]], volume[1])
		for i = 2, #volume do
			res:add(',{"name":"%s","value":%u}',
				pa.position_name[map[i]], volume[i])
		end
		res:add('],')
	end

	local now = gettimeofday()
	state = {
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
					volume_add(t.volume, t.channel_map, res)
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
					volume_add(t.volume, t.channel_map, res)
					res:add('"base_volume":%u}', t.base_volume)
				end)
			end,
		},
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
		card = {
			stamp  = now,
			data   = assert(c:card_info()),
			update = function(self, typ, idx)
				if typ == 'remove' then
					self.data[idx] = nil
				else
					self.data[idx] = c:card_info(idx)
				end
				self.stamp = gettimeofday()
			end,
			tojson = function(self, res)
				list(self.data, res, function(t, res)
					res:add('{\z
						"index":%u,\z
						"name":"%s",\z
						"description":"%s",\z
						"card_name":"%s",\z
						"product_name":"%s"}',
						t.index, t.name,
						t.proplist['device.description'],
						t.proplist['alsa.card_name'],
						t.proplist['device.product.name'])
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
					volume_add(t.volume, t.channel_map, res)
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

GET('/', function(req, res)
	local accept = req.headers['Accept']
	--[[
	if accept and accept:match('application/xhtml%+xml') then
		--res.headers['Content-Type'] = 'application/xhtml+xml; charset=UTF-8'
		res.headers['Content-Type'] = 'application/xml; charset=UTF-8'
	else
	--]]
		res.headers['Content-Type'] = 'text/html; charset=UTF-8'
	--end
	res.file = 'index.html'
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

GETM('^/poll/(%d+%.?%d*)$', function(req, res, stamp)
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

local parseform
do
	local char = string.char
	local function urldecode(str)
		return str:gsub('+', ' '):gsub('%%(%x%x)', function(str)
			return char(tonumber(str, 16))
		end)
	end

	function parseform(str)
		local t = {}
		for k, v in str:gmatch('([^&]+)=([^&]*)') do
			t[urldecode(k)] = urldecode(v)
		end
		return t
	end
end

local function update_volume(vol, cm, body)
	local r, changed = {}, false
	for i = 1, #vol do
		local n = tonumber(body[pa.position_name[cm[i]]])
		if n then
			r[i] = n
			changed = true
		else
			r[i] = vol[i]
		end
	end
	if changed then return r end
	return nil
end

local function scale_volume(vol, n)
	if not n then return nil end
	local len = #vol
	local s = 0
	for i = 1, len do
		local vi = vol[i]
		if vi > s then s = vi end
	end
	s = (s + n) / s
	local r = {}
	for i = 1, len do
		r[i] = s * vol[i]
	end
	return r
end

POSTM('^/sink/(%d+)$', function(req, res, idx)
	idx = tonumber(idx)
	local sink = state.sink.data[idx]
	if not sink then hathaway.not_found(req, res) return end

	local body = req:body()
	if not body then return end

	body = parseform(body)
	if body.mute ~= nil then
		if body.mute == 'true' then
			c:set_sink_mute(idx)
		else
			c:set_sink_unmute(idx)
		end
	else
		local new
		if body.rel then
			new = scale_volume(sink.volume, tonumber(body.rel))
		else
			new = update_volume(sink.volume, sink.channel_map, body)
		end
		if new then
			c:set_sink_volume(idx, new)
		end
	end
end)

POSTM('^/source/(%d+)$', function(req, res, idx)
	idx = tonumber(idx)
	local source = state.source.data[idx]
	if not source then hathaway.not_found(req, res) return end

	local body = req:body()
	if not body then return end

	body = parseform(body)
	if body.mute ~= nil then
		if body.mute == 'true' then
			c:set_source_mute(idx)
		else
			c:set_source_unmute(idx)
		end
	else
		local new
		if body.rel then
			new = scale_volume(source.volume, tonumber(body.rel))
		else
			new = update_volume(source.volume, source.channel_map, body)
		end
		if new then
			c:set_source_volume(idx, new)
		end
	end
end)

POSTM('^/sink%-input/(%d+)$', function(req, res, idx)
	idx = tonumber(idx)
	local sink_input = state.sink_input.data[idx]
	if not sink_input then hathaway.not_found(req, res) return end

	local body = req:body()
	if not body then return end

	body = parseform(body)
	if body.mute ~= nil then
		if body.mute == 'true' then
			c:set_sink_input_mute(idx)
		else
			c:set_sink_input_unmute(idx)
		end
	elseif body.kill then
		c:kill_sink_input(idx)
	else
		local new
		if body.rel then
			new = scale_volume(sink_input.volume, tonumber(body.rel))
		else
			new = update_volume(sink_input.volume, sink_input.channel_map, body)
		end
		if new then
			c:set_sink_input_volume(idx, new)
		end
	end
end)

POSTM('^/source%-output/(%d+)$', function(req, res, idx)
	idx = tonumber(idx)
	local source_output = state.sink_input.data[idx]
	if not source_output then hathaway.not_found(req, res) return end

	local body = req:body()
	if not body then return end

	if body.kill then
		c:kill_source_output(idx)
	end
end)

POSTM('^/module/(%d+)$', function(req, res, idx)
	idx = tonumber(idx)
	local module = state.module.data[idx]
	if not module then hathaway.not_found(req, res) return end

	local body = req:body()
	if not body then return end

	body = parseform(body)
	if body.unload then
		c:unload_module(idx)
	end
end)

POSTM('^/client/(%d+)$', function(req, res, idx)
	idx = tonumber(idx)
	local client = state.client.data[idx]
	if not client then hathaway.not_found(req, res) return end

	local body = req:body()
	if not body then return end

	body = parseform(body)
	if body.kill then
		c:kill_client(idx)
	end
end)

GETM('^/(js/.+)$', function(req, res, file)
	res.headers['Content-Type'] = 'text/javascript; charset=UTF-8'
	res.file = file
end)

GETM('^/(css/.+)$', function(req, res, file)
	res.headers['Content-Type'] = 'text/css; charset=UTF-8'
	res.file = file
end)

GETM('^/(img/.+)$', function(req, res, file)
	res.headers['Content-Type'] = 'image/png'
	res.file = file
end)

hathaway.debug = print
Hathaway('*', arg[1] or 8080)

assert(c:disconnect())

-- vim: set ts=2 sw=2 noet:
