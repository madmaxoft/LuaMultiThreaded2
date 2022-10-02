-- Tests being able to send parameters to new threads on creation.

local thread = require("thread");

-- The amount of threads that will be created.
local NUM_THREADS = 4;

-- The maximum value to look for primes at.
local CHECK_PRIMES_UNTIL = 400

-- The amount of numbers each thread will have to check.
local BATCH_SIZE_PER_THREAD = math.floor(CHECK_PRIMES_UNTIL / NUM_THREADS)


-- Will contain all thread objects and their output.
local threads = {}

-- Checks if the provided number is a prime.
-- Not an optimal implementation, but that's not the point of this test.
local function isPrime(n)
	if ((n == 2) or (n == 5)) then
		-- 2 and 5 are primes.
		return true;
	elseif ((n % 2 == 0) or (n % 5 == 0)) then
		-- Anything divisible by 2 or 5 is not a prime.
		return false;
	elseif (n == 1) then
		-- 1 isn't a prime either.
		return false;
	end
	
    for i = 3, n / 2, 2 do
        if (n % i) == 0 then
            return false
        end
    end
    return true
end

-- Thread function. Will check for primes in the provided limits.
local thrFunc = function(output, min, max)
	-- Go through every number between min and max.
	-- If the number is a prime save it in output.
	for number = min, max do
		if (isPrime(number)) then
			table.insert(output, number);
		end
	end
end

-- Create new threads and provide the limits where they should look for primes as parameters.
for I = 0, NUM_THREADS - 1 do
	local idx = #threads + 1;
	local batchStart = I * BATCH_SIZE_PER_THREAD;
	local output = {}
	local thr = thread.new(thrFunc, output, batchStart, batchStart + BATCH_SIZE_PER_THREAD)
	threads[idx] = {thread = thr, output = output}
end

-- Loop through every created thread and join them. 
-- Since the threads are ordered based on the batch they were working on we can immediately print their output.
for idx, thrObj in ipairs(threads) do
	thrObj.thread:join();
	for _, prime in ipairs(thrObj.output) do
		print("From Thread nr." .. idx .. ': ' .. prime);
	end
end


