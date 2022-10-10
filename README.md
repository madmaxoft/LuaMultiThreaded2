# LuaMultiThreaded2
Trying to run Lua code in parallel in multiple threads, using no locking at all.

This is an experiment / playground to test if it is possible to run Lua code in a single `lua_State` instance in multiple threads, using the `lua_newthread` API. The assumption is that the Lua-builtin threads provide a separate execution stack, so as long as the threads don't access globals, they should be safe.

## The plan
- [x] Get a basic Lua-interpreter to compile, using Lua-static library and a very simple command-line processor
- [x] Write an API to start a new thread, and experiment with Lua code to see if it behaves as expected
- [x] Extend the thread API to provide the regular threading functions - `join`, `id`
- [x] Check that the code actually runs in parallel
- [ ] Check whether there are race conditions possible, and what they do (crash? bad data?)
- [ ] Make sure the thread API plays nice with Lua's GC, esp. wrt. the `lua_newthread` object collection.
- [ ] Add synchronization primitives
   - [x] Mutex
   - [ ] Semaphore?
   - [ ] Atomics?
- [ ] Check whether access to globals protected by mutexes is safe.
- [ ] Split the thread API into a library that can be reused.
