
#ifndef CANARY_H
#define CANARY_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// The semantic version number components.
#define CANARY_VERSION_MAJOR 0
#define CANARY_VERSION_MINOR 0
#define CANARY_VERSION_PATCH 0

#define CANARY_STRINGIFY(str) #str

// A human-friendly string representation of the version.
#define CANARY_VERSION_STRING                                                  \
  CANARY_STRINGIFY(CANARY_VERSION_MAJOR) "."                                   \
  CANARY_STRINGIFY(CANARY_VERSION_MINOR) "."                                   \
  CANARY_STRINGIFY(CANARY_VERSION_PATCH)

#define CANARY_MAKE_VERSION(major, minor, patch)                               \
  ((major) * 10000 + (minor) * 100 + (patch))

// A monotonically increasing numeric representation of the version number. Use
// this if you want to do range checks over versions.
#define CANARY_VERSION                                                         \
  (CANARY_MAKE_VERSION(CANARY_VERSION_MAJOR,                                   \
                       CANARY_VERSION_MINOR,                                   \
                       CANARY_VERSION_PATCH))

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // CANARY_H
