
local function func(ip, port)
    local sock = mtcp.socket.tcp()
    sock:settimeout(60000)
    local ok, err = sock:connect("10.10.10.240", "80")
    if not ok then
	    print('err: ', err)
	    return
    end
    mtcp.sleep(10)

    --print('connect successful')
    local ok, err = sock:send('GET / HTTP/1.0\r\nConnection:keepalive\r\n\r\n')
    if not ok then
	    print('send failed: ', err)
	    return
    end
    
    --print('send successful')
    --mtcp.sleep(5)
    ok, err = sock:recv()
    if not ok then
	    print('recv failed: ', err)
	    return
    end
    --print ('recvd:', ok)
    ok, err = sock:close()
    if not ok then
	    print ('close failed: ', err)
    end

    return
end
--[[
local function func2()
    for i = 1, 10 do
        func()
    end
end
]]--
for i = 1, 8000 do
    local t, err = mtcp.thread.spawn(func)
    if not t then
        print("err: ", err, ", typeof: ", type(t))
    end
end

mtcp.sleep(10)
print("Exit")
