// Copyright 2018 Filegear - All Rights Reserved

#pragma once

#include <string>
#include <stdint.h>

class CommandLineOptions
{
public:
  static std::string host;
  static std::string uuid;
  static std::string token;
  static uint32_t port;
  static int32_t initial_mtu;
  static bool verify_certificates;
  static int32_t quic_version;

  static bool Usage(const char * format, ...);
  static bool Init(int argc, char ** argv);
};