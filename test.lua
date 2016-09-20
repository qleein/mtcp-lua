local a = 5
local b = 6
local c = a + b
print('c:', c)
print("mtcp.welcome", mtcp.welcome)

local function func()
    print("Come Here")
    --mtcp.sleep(2)
    print("Come Here2")
    return
end

local co = mtcp.thread.spawn(func)
print("Exit")

