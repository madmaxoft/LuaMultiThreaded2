-- Tests the thread creation by simply outputting a string from within the new thread

local thread = require("thread")
thread.new(
	function()
		print("Hello thread")
	end
)
print("Hello main")
