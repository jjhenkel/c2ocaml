# Docker for Static Analysis

This document will describe two tools developed at UW Madison to facilitate program analysis by leveraging Docker. In addition, this document will cover some of the challenges we've faced using Docker at extremely large scales (tens of thousands of images). Finally, this document includes some brief summaries of technology related to automating or streamlining the process of managing thousands of individual source projects and their builds. 

## Groundwork

When we started incorporating Docker we focused on encapsulating source repositories from GitHub along with the steps to build them. We focused on this as our program analysis infrastructure requires a project to be buildable. This focus lead to the following architecture: 

 - A one-to-one correspondence between Docker images and source repositories
 - An application to build such a Docker image given a repository's URL
 - An application to build a Docker image given more detailed information (the steps to build the given source repository)

The underlying idea of each project being _frozen_ into a Docker image leads to several advantages: 

 - The ability to export a image to a binary format and give it to someone else
 - The ability to push/pull images from a central (private or global) registry 
 - The ability to 'start from scratch' by destroying the container running off of the given image and creating a new one 

These advantages greatly ease experimentation as the use of images forces repeatable steps to be written down that, with the help of the Docker client/daemon, can be used to restore an environment to a known-good state.

## Building Images from Repositories (Lightweight)

### NOTE: this tool not included in c2ocaml distribution

Given this groundwork, we can focus on the first of two tools developed to create and manage images. This tool, called [repo2image](src/repo2image/Readme.md), operates on a URL for a given GitHub repository. Given this fragment of information, and assuming the presence of Docker on the host machine, the tool produces an image containing the repository and some useful meta-data.  

**Note:** this tool was used to create thousands of Docker images on a host server we have. We found that, after a certain point, the performance of Docker degrades when storing thousands of images. In some sense, this is a very exceptional use case for Docker. Docker is great at running as many lightweight containers as possible (usually machine / kernel limits are the only limiting factor to how many containers can be run). On the other hand, storing thousands of unique images seemed to burden our Docker installation and, in general, lead to a noticeable performance penalty for most common operations such as `docker images` or `docker run`.

To combat this, experiments ran on 'batches' of a few hundred images at a time. This 
batching was made feasible via the addition of a Docker registry that could, with ease,
store thousands of unique images (and take advantage of common layers to avoid storing
more than necessary). We would use a registry to store all of our images and then our 
application would pull a batch down to the host machine for use. Following that, the 
batch was removed and a new batched pulled until the whole set of images had been 
enumerated.

## Building Images from a Specification

One thing Docker is missing is a clear way to 'template' images; instead, Docker provides a method of specifying `build args` that can act, in limited ways, as parameters to the image. This tool is a way to take small parameter files (stored as key/value pairs) and produce images by leveraging `build args`. This tool is called `spec2image`.

To use this tool, the user would provide a small specification file, such as the one below:

```bash
#!/bin/bash

PROJECT="nmap"
PROJECT_GIT_URL="https://github.com/nmap/nmap.git"

PROJECT_DEPS="make libtool autoconf automake libssl-dev"

PROJECT_SETUP="./configure"

PROJECT_CLEAN="make clean"
PROJECT_MAKE="make -j$(nproc)"

PLUGIN_NAME="letstransform"
PLUGIN_PATH="/common/plugins/$PLUGIN_NAME"
PLUGIN_ARGS="-fplugin-arg-$PLUGIN_NAME-project=$PROJECT"
PLUGIN_SPEC="-fplugin=$PLUGIN_PATH $PLUGIN_ARGS"
```

Running the tool on this file produces an image that has the source for `nmap` cloned and two scripts generated (`lpl-make` / `lpl-clean`) that either make or clean the project. In addition, this spec includes information for a GCC plugin and the  image has the ability to capture any calls to `gcc`/`g++`/`gnat`/`gccgo`.

A key feature of this tool is the generation of an image where critical applications can be 'replaced' to facilitate analysis. Normally, running our analysis infrastructure would have required _some_ modification to the original source repository but now we can simply provide 'shims' for things like `gcc` that redirect to custom-compiled versions with analysis enabled.

Furthermore, we've found that writing _the analysis tools themselves_ into Docker images gives us even more flexibility. With the help of `volumes` we are able to repeatably build our analysis infrastructure and `mount` the compiled tools for use in other containers. 
 

**Note:** In docker, `$(nproc)` returns the _number of processors on the host machine_. This is fine, as long as you haven't set resource limits. There [may be some issues](https://github.com/moby/moby/pull/31756) with having `$(nproc)` reflect limits (or they may be resolved by now, I just remember reading an [issue related to this](https://github.com/moby/moby/issues/31358)).

**Note:** This lack of template support is likely due to the fact that most production usages of Docker focus on a small set of images and thousands of running containers whereas the analysis community has needs for many unique images and containers. 
