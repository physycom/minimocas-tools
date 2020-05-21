/*!
 *  \file   miniplex.cpp
 *  \author A. Fabbri (alessandro.fabbri27@unibo.it), C. Mizzi (chiara.mizzi2@unibo.it)
 *  \brief  Main file for `minitest` app.
 *  \details This file contains the source code for the app `minitest`.
 */

#include <iostream>

#include <multiplex.h>
#include <miniplex_gui.h>

using namespace std;
using namespace jsoncons;

constexpr int MAJOR = 0;
constexpr int MINOR = 0;

void usage(const char* progname)
{
  string pn(progname);
  cerr << "Usage: " << pn.substr(pn.find_last_of("/\\") + 1) << " path/to/json/config" << endl;
  cerr << R"(JSON CONFIG SAMPLE
{
  "layers_conf" : [
    "path/to/conf1.json",
    "path/to/conf2.json"
  ]
}
)";
}

int main(int argc, char** argv)
{
  cout << "miniplex v" << MAJOR << "." << MINOR << endl;

  string conf;
  if (argc == 2)
  {
    conf = argv[1];
  }
  else
  {
    usage(argv[0]);
    exit(1);
  }

  try
  {
    json jconf = json::parse_file(conf);

    multiplex m(jconf);

    // Gui
    gui g(&m);

    // Display stuff
    g.run();
  }
  catch (exception &e)
  {
    cerr << "EXC: " << e.what() << endl;
    exit(1);
  }

  return 0;
}
