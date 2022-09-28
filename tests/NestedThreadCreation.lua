-- Tests creating a thread from another thread

local thread = require("thread")

thread.new(
	function()
		print("First thread started, starting another one.")
		thread.new(
			function()
				print("Second thread started, starting the last one.")
				thread.new(
					function()
						print("Third thread started. All done.")
					end
				)
			end
		)
	end
)
print("main")

-- Sleep for 1 second to let the threads finish in the meantime (we don't have synchronization yet):
thread.sleep(1)
