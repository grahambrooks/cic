#pragma once

namespace ci {
  struct ANSI_ESCAPE {
    constexpr static auto GREEN = "\x1b[32m";
    constexpr static auto RED = "\x1b[31m";
    constexpr static auto RESET = "\x1b[0m";
  };
}
