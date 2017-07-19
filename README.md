# mtcp-lua
Asynchronous tcp client in lua, based on mtcp and intel dpdk. It's a demo.


# How To Use

```
$ cd mtcp-lua
$ mdkir build && cd build
$ cmake ..
$ make
```
config file like mtcp project

#Example

```lua
--test.lua
-- extra: route and arp needed for use mtcp.
local function func(ip, port)
  local sock = mtcp.socket.tcp()
  local ok, err = sock:connect(ip, port)
  if not ok then
    print("connect failed, err: ", err)
    return
  end
  
  ok, err = sock:send("GET / HTTP/1.0\r\nConnection:close\r\n\r\n")
  if not ok then
    print("send failed, err: ", err)
    return
  end
  
  ok, err = sock:recv()
  if not ok then
    print("recv failed, err: ", err)
    return
  end
  
  print("receive data: ", ok)
  
  sock:close()
  mtcp.sleep(5)
  
  print("exit")
  return
end

for i = 1, 10 do
  ip = "1.1.1." .. tostring(i)
  port = "80"
  
  local ok, err = mtcp.thread.spawn(func, ip, port)
  if not ok then
    print("spawn thread failed, err: ", err)
    return
  end
end

mtcp.sleep(10)
print("Main thread exit")
```
