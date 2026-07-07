#include "resmack/logo.hpp"


namespace resmack {

  const char* GetResmackLogo() {
    static const char* banner = 
      "\n"
      "                                 ░    \n"
      "             ╭───┐╭─┬─┐╭───┐╭───┐▒  ╱ \n"
      "   █━━━┓┏━━━┓│    │ │ │    ││   │▓ ╱  \n"
      "   █    ┣━━━┛└───┐│ │ │┌───┤│    █◀   \n"
      "   ▓    ┃        ││   ││   ││    █ ╲  \n"
      "   ▒    ┗━━━┛└───╯│   │└───┤└───╯█  ╲ \n"
      "   ░              │   │              ╲\n"
      "\n"
      "      by Jess 'd0c-s4vage' Johnson   \n";
    return banner;
  }

}
