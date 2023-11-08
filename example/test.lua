-- 定义基础函数tostring
local function tostring(value)
    --local mt = getmetatable(value)
    --if mt and mt.__tostring then
    --    return mt.__tostring(value)
    --end
    if type(value) == "string" then
        return value
    elseif type(value) == "number" then
        return string.format("%f", value)
    elseif type(value) == "table" then
        return "<table>"
    elseif type(value) == "function" then
        return "<function>"
    else
        return "<nil>"
    end
end

-- 定义基础函数print
local function print(...)
    local args = { ... } -- 将传入的参数保存到一个表中
    for i, v in ipairs(args) do
        io.write(tostring(v)) -- 将参数转换为字符串并输出
        if i < #args then
            io.write("\t") -- 在参数之间添加制表符分隔
        end
    end

    io.write("\n") -- 输出换行符
end

-- 打印版本
print("======== _VERSION ==========")
print("Version:", _VERSION)
print("======== _VERSION ==========\n")

-- 打印全局变量
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

