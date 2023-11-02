for i, v in pairs(_G) do
    io.write(i, "\n")
end

function fib(n)
    if n < 2 then
        return 1
    end
    return fib(n - 1) + fib(n - 2)
end


fib(35)