#!/bin/sh
find . -path ./example/node_modules -prune -o \( -name '*.h' -or -name '*.cpp' \) \
  -exec clang-format -i {} \;
