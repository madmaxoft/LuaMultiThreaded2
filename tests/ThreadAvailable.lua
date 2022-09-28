-- Tests the availability of the "thread" library

local thread = require("thread")
print("thread library: " .. tostring(thread))
print("  thread.new = " .. tostring(thread.new))
print("  thread.sleep = " .. tostring(thread.sleep))
