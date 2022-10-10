# Main LuaMultiThreaded2 CMake file

project (LuaMultiThreaded2 CXX)
set (CMAKE_CXX_STANDARD 17)





set(SRCS
	LuaMultiThreaded2.cpp
	Utils.cpp
)





set(HDRS
	Utils.h
)





add_executable(LuaMultiThreaded2
	${SRCS}
	${HDRS}
)




target_link_libraries(LuaMultiThreaded2
	lua-static
)




set(TESTS
	CurrentThreadId.lua
	HelloThread.lua
	HelloWorld.lua
	NestedThreadCreation.lua
	RaceCondition.lua
	RaceConditionMutex.lua
	ThreadAvailable.lua
	ThreadParameters.lua
	ThreadSleep.lua
)



foreach(test IN LISTS TESTS)
	add_test(
		NAME ${test}
		COMMAND LuaMultiThreaded2 ${test}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../tests
	)
endforeach()
