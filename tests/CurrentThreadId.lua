-- Tests the thread.currentid() function.

local thread = require("thread");

-- Variable which the sub thread will save it's id in.
local threadId;

-- Create a new thread which will save it's id to the threadId variable after one second.
-- The extra second is to give the main thread more time to retrieve the id.
local thread1 = thread.new(function()
	thread.sleep(1);
	threadId = thread.currentid();
end);

-- thread:id() can't be called if the thread is joined already so save it's id to another variable.
local actualThreadId = thread1:id();

thread1:join();

-- Make sure the id's are the same.
assert(actualThreadId == threadId);
