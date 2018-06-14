# GCC 7.2.0 
## Project Directory

This directory contains the specification for the exact configuration of GCC I use to build and run my plugin(s) and analyses. This specification exists within the Dockerfile and, if necessary, the Dockerfile can be built to create an image with the exact copy of GCC in use throughout the rest of the project.

It is **important to note** that this GCC is not quite the same as the GCC you'll find on most distros by default. This GCC has _plugin support enabled_ as well as _all of the possible supported languages enabled_.

## Workflow 

First, you would build this image. From the repository root:

```bash
cd ./gcc-7.2.0 
docker build -t username/gcc7.2.0 .
```

Next, you'll want to _actually run a container off this image and give it a memorable name_:

```bash
docker run --name gcc7.2.0 username/gcc7.2.0
```

When this is complete, docker will have a volume that has all of the executables, libraries, and includes needed to run this custom-compiled version of GCC. This means that containers that build off of or with this GCC version don't need to inherit this image as a base (since it's quite large). Instead, something further in our workflow that needs our custom GCC would do: 

```bash
docker run --volumes-from=gcc7.2.0 username/some-other-image bash
```
