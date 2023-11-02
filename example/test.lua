for i, v in pairs(bit) do
    io.write(i, "\n")
end

local function fib(n)
    if n < 2 then
        return 1
    end
    return fib(n - 1) + fib(n - 2)
end

-- fib(35)

local function counter()
    local n = 0
    return function()
        n = n + 1
        return n
    end
end

local c = counter()
io.write(c(), "\n")
io.write(c(), "\n")
io.write(c(), "\n")

io.write(bit.tohex(12), "\n")