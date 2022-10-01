-- Tests the thread.currentid() function.

local thread = require("thread");

-- Variable which the sub thread will save its id in.
local threadId;

-- Create a new thread which will save its id to the threadId variable.
-- The thread.sleep is to give the main thread more time to retrieve the id.
-- This can be removed once some kind of thread synchronisation is added.
local thread1 = thread.new(function()
	thread.sleep(0.1);
	threadId = thread.currentid();
end);

-- thread:id() can't be called if the thread is joined already so save its id to another variable.
local actualThreadId = thread1:id();

thread1:join();

-- Make sure the ids are the same.
assert(actualThreadId == threadId);
