set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set_target_properties(Threads::Threads PROPERTIES IMPORTED_GLOBAL TRUE)
