-- 基础函数tostring
local function tostring(value)
    if type(value) == "string" then
        return value
    elseif type(value) == "number" then
        return string.format("%f", value)
    elseif type(value) == "table" then
        return "<table>"
    elseif type(value) == "function" then
        return "<function>"
    elseif type(value) == "userdata" then
        return "userdata"
    else
        return "<nil>"
    end
end

-- 基础函数print
local function print(...)
    local args = { ... }
    for i, v in ipairs(args) do
        io.write(tostring(v))
        if i < #args then
            io.write("\t")
        end
    end

    io.write("\n")
end

-- lua版本
print("======== _VERSION ==========")
print("Version:", _VERSION)
print("======== _VERSION ==========\n")

-- lua全局变量
print("======== _G ==========")
for i, _ in pairs(_G) do
    print(i)
end
print("======== _G ==========\n")

print("======== fib ==========")
local function fib(n)
    if n < 2 then
        return 1
    end
    return fib(n - 1) + fib(n - 2)
end
local begin = os.clock()
fib(30)
local finish = os.clock()
print("fib(30):", finish - begin)
print("======== fib ==========\n")

-- 闭包测试
print("======== counter ==========")
local function counter()
    local n = 0
    return function()
        n = n + 1
        return n
    end
end

local c1 = counter()
print(c1())
print(c1())
print(c1())

local c2 = counter()
print(c2())
print(c2())
print(c2())
print("======== counter ==========\n")
