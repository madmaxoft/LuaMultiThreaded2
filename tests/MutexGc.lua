-- Forces a mutex object to be garbage collected by Lua.

local thread = require("thread")

-- Create a new mutex.
local mtx = mutex.new();

-- Immediately set the mutex to nil
mtx = nil;

-- Force the garbage collector to collect everything.
-- Unfortunately there is no way to check if the value was properly destroyed from Lua
-- but it can be used to see if a breakpoint inside the __gc function is reached.
-- If the __gc function crashes it can be detected here as well.
collectgarbage("collect");
