// RUN: split-file %s %t
// RUN: %clang --no-default-config --target=x86_64-unknown-motor \
// RUN:   -ivfsoverlay %t/overlay.yaml -fsyntax-only %t/check.c
// RUN: %clang --no-default-config --target=x86_64-unknown-motor \
// RUN:   -ivfsoverlay %t/overlay.yaml -x c++ -fsyntax-only %t/check.c

//--- check.c
#include <stddef.h>
#if __has_include(<motor-host-only.h>)
#error Host system headers are visible to the Motor target
#endif
#if __has_include(<motor-local-only.h>)
#error Host local headers are visible to the Motor target
#endif

//--- overlay.yaml
{
  'version': 0,
  'overlay-relative': true,
  'roots': [
    { 'name': '/usr/include', 'type': 'directory', 'contents': [
      { 'name': 'motor-host-only.h', 'type': 'file', 'external-contents': 'host.h' }
    ] },
    { 'name': '/usr/local/include', 'type': 'directory', 'contents': [
      { 'name': 'motor-local-only.h', 'type': 'file', 'external-contents': 'host.h' }
    ] }
  ]
}

//--- host.h
