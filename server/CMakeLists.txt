add_executable(server 
    main.cpp 
    Server.cpp 
    Game.cpp
    ../include/SharedMemory.cpp
)

if(APPLE OR UNIX)
    target_link_libraries(server Threads::Threads)
else()
    target_link_libraries(server pthread)
endif()
