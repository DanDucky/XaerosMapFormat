# XaerosMapFormat
A C++ library for reading and writing to the Xaero's Map save format.

| NOT FUNCTIONAL YET |
|--------------------|

continuation and libraryfication of [XaerotoJourneyMap](https://github.com/DanDucky/XaerotoJourneyMap)

## Dependencies

### Build Dependencies

- CMake
- C++23 compiler
- Catch2 (if `XAERO_BUILD_TESTS`) *provided through CPM CMake, I haven't provided an override for this because it should be turned off in production anyways*

> I'm sorry for these, but they are unfortunately necessary for this process unless we want to extract obfuscated Minecraft data and process textures in raw CMake. They can be turned off with `XAERO_GENERATE_RESOURCES` but this would just offload that work onto users, which would be a huge pain and provides ample room for mistakes. 

- Python (if `XAERO_GENERATE_RESOURCES`) *this has associated dependencies which are downloaded by pip in a .venv, so don't worry!*
- Java 21 (if `XAERO_GENERATE_RESOURCES`)

### Dependencies

All of these dependencies are managed and downloaded by CPM in the provided CMake file. However, they can be optionally provided at configure time.

- libnbt++