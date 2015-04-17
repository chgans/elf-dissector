#ifndef CONFIG_ELFDISSECTOR_H
#define CONFIG_ELFDISSECTOR_H

// TODO actually detect the binutils version...
#define BINUTILS_VERSION_MAJOR ${Binutils_VERSION_MAJOR}
#define BINUTILS_VERSION_MINOR ${Binutils_VERSION_MINOR}

#define BINUTILS_VERSION ((BINUTILS_VERSION_MAJOR << 8) | BINUTILS_VERSION_MINOR)
#define BINUTILS_VERSION_CHECK(maj, min) ((maj << 8) | min)

#endif
