local a = 5
local b = 6
local c = a + b
print('c:', c)
print("mtcp.welcome", mtcp.welcome)
mtcp.sleep(1)
print("mtcp.welcome2", mtcp.welcome)


--local function func()
    print("Come Here")
local sock = mtcp.socket.tcp()
local ok, err = sock:connect('192.168.0.105', 9000)
if not ok then
	print('err: ', err)
	return
end
print('connect successful')
local ok, err = sock:send('GET / HTTP/1.1\r\nConnection:close\r\n\r\n')
if not ok then
	print('send failed: ', err)
	return
end
print('send successful')
--mtcp.sleep(5)
ok, err = sock:recv()
if not ok then
	print('recv failed: ', err)
	return
end
print ('recvd:', ok)

ok, err = sock:close()
if not ok then
	print ('close failed: ', err)
end

print("Exit")
