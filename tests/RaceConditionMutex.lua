-- Tests a race condition. Identical to RaceCondition.lua except it uses a mutex to prevent the race condition

local thread = require("thread")

--- The global variable being read and written
local a = 0

--- The number of iterations of the inner loop
local MAX_COUNTER = 100000

-- The mutex to allow the threads to write to the global variable.
local mtx = mutex.new();



--- The thread function, executed in two threads in parallel
local function thrFunc()
	for i = 1, MAX_COUNTER do
		mtx:lock(function()
			a = a + 1
		end)
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
