# Genesis project

Genesis is an early-stage emulator for the Sega Genesis (Mega Drive) console. While it is still under development, the project aims to provide an accurate and high-performance emulation of the classic Sega Genesis system. Despite its early state, the emulator is already capable of running most Genesis games, though users may encounter some bugs and incomplete features. Ongoing development is focused on enhancing emulation accuracy, improving performance, and expanding games compatibility.

## Emulator Demo Video

This demo video showcases an early version of emulator running various Sega Genesis games. Please note that the emulator is still in development, and some features may be incomplete (e.g. the sound subsystem is currently under development) or not fully optimized.

[demo.webm](https://github.com/Darrer/genesis/assets/20683759/da8a8910-36de-4862-9b33-0c02e6aed62f)

Games featured in this demo:

- [PapiRium Demo](https://vetea.itch.io/papirium-official-demo)
- [FoxyLand Demo](https://pscdgames.itch.io/foxyland)
- [PapiCommandoReload Demo](https://vetea.itch.io/papi-commando-reload-free-demo-tectoy)

Feel free to explore the current capabilities of the emulator through these gameplay demonstrations.

## Building and Testing

To build the project using `make`, follow these steps:

```console
mkdir build && cd build
cmake .. -G "Unix Makefiles"
make
```

To run tests, execute the following command:

```console
ctest
```

For a more detailed output, you can alternatively run:

```console
make && ./tests/genesis_tests
```

## Build Requirements

To build the project, you need the following:

- C++ compiler that supports C++23
- CMake version 3.5 or higher

All other dependencies will be loaded automatically.

## License

This project is licensed under the GNU GPLv3 - see the LICENSE file for details.
