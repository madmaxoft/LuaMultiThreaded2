-- Tests a race condition - reading from and writing to a global variable from multiple threads

local thread = require("thread")

--- The global variable being read and written
local a = 0

--- The number of iterations of the inner loop
local MAX_COUNTER = 100000





--- The thread function, executed in two threads in parallel
local function thrFunc()
	for i = 1, MAX_COUNTER do
		a = a + 1
	end
end





local thread1 = thread.new(thrFunc)
local thread2 = thread.new(thrFunc)

thread1:join()
thread2:join()

if (a == 2 * MAX_COUNTER) then
	print("No race condition detected")
else
	print("Race condition detected:")
	print("a = " .. tostring(a) .. "(expected " .. 2 * MAX_COUNTER .. ")")
end
