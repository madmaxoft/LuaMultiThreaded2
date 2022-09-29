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
