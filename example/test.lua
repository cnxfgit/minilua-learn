-- �����������tostring
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

-- �����������print
local function print(...)
    local args = { ... } -- ������Ĳ������浽һ������
    for i, v in ipairs(args) do
        io.write(tostring(v)) -- ������ת��Ϊ�ַ��������
        if i < #args then
            io.write("\t") -- �ڲ���֮������Ʊ���ָ�
        end
    end

    io.write("\n") -- ������з�
end

-- ��ӡ�汾
print("======== _VERSION ==========")
print("Version:", _VERSION)
print("======== _VERSION ==========\n")

-- ��ӡȫ�ֱ���
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

