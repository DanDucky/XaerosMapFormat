# XaerosMapFormat
A C++23 library for reading, writing, and rendering the Xaero's Map save format.

| NOT FUNCTIONAL YET |
|--------------------|

continuation and libraryfication of [XaerotoJourneyMap](https://github.com/DanDucky/XaerotoJourneyMap)

## Building
```shell
cmake -S <path/to/source> -B <path/to/build/dir> -DJAVA_HOME="<path/to/java21/install>"
cmake --build <path/to/build/dir>
```

### Options
| Name                       | Description                                                               | Default Value |
|:---------------------------|:--------------------------------------------------------------------------|:--------------|
| `XAERO_BUILD_TESTS`        | Builds test executable                                                    | `ON`          |
| `XAERO_GENERATE_RESOURCES` | Generates Minecraft data lookup tables for rendering and format upgrades. | `ON`          |

## Dependencies

### Build Dependencies

- CMake
- C++23 compiler
- Catch2 (if `XAERO_BUILD_TESTS`) *provided through CPM CMake, I haven't provided an override for this because it should be turned off in production anyways*

> I'm sorry for these, but they are unfortunately necessary for this process unless we want to extract obfuscated Minecraft data and process textures in raw CMake. They can be turned off with `XAERO_GENERATE_RESOURCES` but this would just offload that work onto users, which would be a huge pain and provides ample room for mistakes. It might be possible to remove the Java dependency, but I don't know how to get the block ids from the version jar.

- Python (if `XAERO_GENERATE_RESOURCES`) *this has associated dependencies which are downloaded by pip in a .venv, so don't worry!*
- Java 21 (if `XAERO_GENERATE_RESOURCES`)

### Dependencies

All of these dependencies are managed and downloaded by CPM in the provided CMake file. However, they can be optionally provided at configure time.

- [libnbt++](https://github.com/PrismLauncher/libnbtplusplus)
- [ztd.text](https://github.com/soasis/text)
- [minizip-ng](https://github.com/zlib-ng/minizip-ng)

## Versioning

tldr: Xaero's Map format version `6.8`

This is a little bit difficult because the versioning is dependent on an internal Xaero's Map format version number. 
Currently, this supports reading from all versions up to 6.8 and writes version 6.8 files.