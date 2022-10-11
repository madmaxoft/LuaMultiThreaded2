-- Tests running the garbage collector in parallel with creating a lot of local temporary objects

local thread = require("thread")




--- Creates lots of local temporary objects that can be GC-ed
local function thrCreateObjects()
	for j = 1, 10 do
		for i = 1, 100 do
			table.concat({"a", i}, ", ")  -- Create a table, then a string, and lose all references to them immediately
		end
		thread.sleep(0.05)  -- Sleep for 50 ms between creations
	end
end





--- Collects garbage several times, with pauses in between
function thrCollectGarbage()
	for i = 1, 20 do
		collectgarbage("collect")
		thread.sleep(0.03)  -- Sleep for 30 ms between GC runs
	end
end





-- Do both object creation and garbage collection in parallel, each in a separate thread:
local thread1 = thread.new(thrCreateObjects)
local thread2 = thread.new(thrCollectGarbage)


-- Wait for the threads to finish running
thread1:join()
thread2:join()

print("Successfully finished.")
