#include <cppunitx/cppunitx.h>

// All test files are compiled into this binary via CMake GLOB.
// This main() just runs them all.

int main(int argc, char* argv[]) {
    return cppunitx::Runner::run(argc, argv);
}
