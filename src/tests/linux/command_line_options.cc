#include "command_line_options.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

std::string CommandLineOptions::host;
std::string CommandLineOptions::uuid;
std::string CommandLineOptions::token;
uint32_t CommandLineOptions::port = 0;
int32_t CommandLineOptions::initial_mtu = 0;
int32_t CommandLineOptions::quic_version = -1;
bool CommandLineOptions::verify_certificates = false;

bool CommandLineOptions::Usage(const char * format, ...)
{
  if (format != NULL)
  {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }
  const char* help_str =
    "Usage: quic_client [options] \n"
    "\n"
    "Options:\n"
    "-h, --help                  show this help message and exit\n"
    "--host <host>               specify the IP address of the hostname to connect to (required)\n"
    "--uuid <uuid>               gear device uuid (required)\n"
    "--port <port>               specify the port to connect to (required)\n"
    "--token <token>             specify the security token used to connect relay server (required)\n"
    "--initial-mtu <mtu>         specify the initial MTU of the connection (default: 0)\n"
    "--quic-version <version>    the quic version to use (default: -1)\n"
    "--verify-certs              verify certificates (default: false)\n";
  printf("%s", help_str);
  return false;
}

bool CommandLineOptions::Init(int argc, char** argv)
{
  // parse
  for (int i = 1; i < argc; i++)
  {
    char * arg = argv[i];
    if (arg == NULL || arg[0] == '\0')
    {
      // skip
      continue;
    }
    else if (arg[0] == '-')
    {
      if (strcmp(arg, "-h") == 0 ||
          strcmp(arg, "--help") == 0)
      {
        return Usage(NULL);
      }
      else if (strcmp(arg, "--host") == 0)
      {
        if (++i == argc) { return Usage("Error: Missing argument for 'host'\n"); }
        CommandLineOptions::host = argv[i];
      }
      else if (strcmp(arg, "--uuid") == 0)
      {
        if (++i == argc) { return Usage("Error: Missing argument for 'uuid'\n"); }
        CommandLineOptions::uuid = argv[i];
      }
      else if (strcmp(arg, "--token") == 0)
      {
        if (++i == argc) { return Usage("Error: Missing argument for 'token'\n"); }
        CommandLineOptions::token = argv[i];
      }
      else if (strcmp(arg, "--port") == 0)
      {
        if (++i == argc) { return Usage("Error: Missing argument for 'port'\n"); }
        char * end = NULL;
        CommandLineOptions::port = strtoul(argv[i], &end, 10);
      }
      else if (strcmp(arg, "--initial-mtu") == 0)
      {
        if (++i == argc) { return Usage("Error: Missing argument for 'initial-mtu'\n"); }
        char * end = NULL;
        CommandLineOptions::initial_mtu = strtol(argv[i], &end, 10);
      }
      else if (strcmp(arg, "--quic-version") == 0)
      {
        if (++i == argc) { return Usage("Error: Missing argument for 'quic-version'\n"); }
        char * end = NULL;
        CommandLineOptions::quic_version = strtol(argv[i], &end, 10);
      }
      else if (strcmp(arg, "--verify-certs") == 0)
      {
        CommandLineOptions::verify_certificates = true;
      }
      else
      {
        return Usage("Error: Unrecognized argument: %s\n", arg);
      }
    }
  }

  // Validate

  if (host.size() == 0)
  {
    return Usage("Error: missing required parameter for '--host'\n");
  }

  if (uuid.size() == 0)
  {
    return Usage("Error: missing required parameter for '--uuid'\n");
  }

  if (token.size() == 0)
  {
    return Usage("Error: missing required parameter for '--token'\n");
  }

  if (port == 0)
  {
    return Usage("Error: missing required parameter for '--port'\n");
  }

  return true;
}
