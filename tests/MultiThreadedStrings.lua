-- Tests creating many strings in parallel
-- The assumption is that since Lua internalizes strings (stores values only once, references strings by-hash),
-- it doesn't like multithreading

local thread = require("thread")




--- Creates lots of local temporary strings
local function thrCreateStrings()
	for j = 1, 10 do
		-- print("thread ", thread.currentid(), ", j = ", j)
		for i = 1, 100 do
			table.concat({"a", i}, ", ")  -- Create a table, then a string, and lose all references to them immediately
		end
		thread.sleep(0.05)  -- Sleep for 50 ms between creations
	end
end





-- Disable GC altogether, so that it doesn't interfere:
collectgarbage("stop")


-- Start multiple threads doing practically the same thing:
local threads = {}
for i = 1, 8 do
	threads[i] = thread.new(thrCreateStrings)
end


-- Wait for the threads to finish running
for i = 1, 8 do
	threads[i]:join()
end

print("Successfully finished.")
