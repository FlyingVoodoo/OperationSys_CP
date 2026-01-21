## Bulls and Cows (Console OS Pet Project)

Console implementation of the classic "Bulls and Cows" game in C++ with a client–server architecture and operating system mechanisms (shared memory, inter‑process communication). The project was originally created as a course work in operating systems.

### Main Features
- Console "Bulls and Cows" game.
- Separation into two applications: `client` and `server`.
- Shared types and utilities extracted into `include/` (`SharedMemory`, `SharedTypes`, etc.).
- CMake-based build system.
- Repository structure suitable for further development and experimentation.

### Tech Stack
- **Language:** C++
- **Build system:** CMake
- **OS concepts:** shared memory, inter‑process communication, synchronization
- **Environment:** console applications

### Project Structure
- `client/` – client application (`Client`, client `main.cpp`).
- `server/` – server application (`Server`, `Game`, server `main.cpp`).
- `include/` – shared headers and implementations (`SharedMemory`, `SharedTypes`, etc.).
- `CMakeLists.txt` – root CMake configuration.

### Build
From the project root directory:

```powershell
mkdir build
cd build
cmake ..
cmake --build .
```

After a successful build, separate client and server executables will be generated (exact paths depend on your CMake configuration).

### Run
1. Build the project (see above).
2. Start the **server** in one console.
3. Start the **client** in another console and connect to the server.
4. Play "Bulls and Cows" through the console interface.

### What I Learned
- Practical experience with OS concepts: shared memory and inter‑process communication.
- Designing a small but complete client–server system in C++.
- Organizing a C++ project with modular structure and CMake build system.