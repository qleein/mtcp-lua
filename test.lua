local a = 5
local b = 6
local c = a + b
print('c:', c)
print("mtcp.welcome", mtcp.welcome)


local function func()
    mtcp.sleep(2)
    print("sleepppp")
end


co = coroutine.create(func)
print("crteate cortoutine")

local res, err = coroutine.resume(co)
print('res:', res, " err:", err)
mtcp.sleep(6)
print("exit")
