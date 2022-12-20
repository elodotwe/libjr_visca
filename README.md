# `libjr_visca`

A library for communicating with pan-tilt-zoom cameras that support the VISCA protocol.

My motivation for this project is to communicate with a PTZOptics branded camera, so I tend to reference the [PTZOptics VISCA reference](https://f.hubspotusercontent20.net/hubfs/418770/PTZOptics%20Documentation/Misc/PTZOptics%20VISCA%20Commands.pdf) for syntax information. However, this protocol appears to be supported by other camera brands, so this may prove useful outside PTZOptics hardware.

This is a hobby project; I have a day job and a house full of children, so I am unable to guarantee support. That said, well-crafted pull requests and bug reports absolutely light up my reward center, so feel free to submit them and I'll likely take a swing at addressing them.

## Building

This project uses CMake, but otherwise has no dependencies.

This is how I do it, but I'm a CMake noob, so feel free to file an issue/PR if there's a better way to set it up :D

```
mkdir build
cd build
cmake ..
make # builds the library + `jr_visca_tester`, a binary that runs some (currently rudimentary) unit tests
make test # optional, runs `jr_visca_tester`
```
