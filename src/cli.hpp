#ifndef OPENCK_CLI_HPP
#define OPENCK_CLI_HPP

#include <QString>
#include <QStringList>

namespace OpenCK::Cli {

// Runs the headless command-line interface. Returns the process exit code
// (0 = success, non-zero = error). Supported commands:
//   openck --cli export <plugin> --format json|csv|xml --out <path> [--types T1,T2]
//   openck --cli info <plugin>
//   openck --cli selftest
//   openck --cli help
int run(int argc, char* argv[]);

// Returns a one-line usage description.
QString usage();

} // namespace OpenCK::Cli

#endif // OPENCK_CLI_HPP
