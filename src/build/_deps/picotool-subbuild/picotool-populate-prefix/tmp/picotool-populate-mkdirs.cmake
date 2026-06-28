# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/workspaces/rp2040/src/build/_deps/picotool-src"
  "/workspaces/rp2040/src/build/_deps/picotool-build"
  "/workspaces/rp2040/src/build/_deps/picotool-subbuild/picotool-populate-prefix"
  "/workspaces/rp2040/src/build/_deps/picotool-subbuild/picotool-populate-prefix/tmp"
  "/workspaces/rp2040/src/build/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp"
  "/workspaces/rp2040/src/build/_deps/picotool-subbuild/picotool-populate-prefix/src"
  "/workspaces/rp2040/src/build/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspaces/rp2040/src/build/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspaces/rp2040/src/build/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
