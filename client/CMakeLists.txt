add_executable(client 
    main.cpp 
    Client.cpp
    ../include/SharedMemory.cpp
)

if(APPLE OR UNIX)
    target_link_libraries(client Threads::Threads)
else()
    target_link_libraries(client pthread)
endif()
