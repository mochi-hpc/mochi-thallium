/*
 * Compiled once into a static library shared by all unit test executables.
 * Provides main() and all doctest implementation symbols so that the shared
 * PCH can include <doctest/doctest.h> (declarations only, without the define)
 * and each test TU's own #include is suppressed by the include guard.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
