/*!
 *  \file   miniedit.cpp
 *  \author A. Fabbri (alessandro.fabbri27@unibo.it), C. Mizzi (chiara.mizzi2@unibo.it), S. Sinigardi (stefano.sinigardi@unibo.it)
 *  \brief  Main file for `miniedit` app.
 *  \details This file contains the source code for the app `miniedit`.
 */

#include <iostream>
#include <carto.h>
#include <minitest_gui.h>
#include <physycom/histo.hpp>

using namespace std;
using namespace jsoncons;
using namespace physycom;

constexpr int MAJOR = 1;
constexpr int MINOR = 2;

void usage(const char *progname)
{
  string pn(progname);
  cerr << "Usage: " << pn.substr(pn.find_last_of("/\\") + 1) << " path/to/json/config" << endl;
  cerr << R"(JSON CONFIG SAMPLE
{
  "file_pro"     : "roads.pro",      // poly properties input file
  "file_pnt"     : "roads.pnt",      // poly geometry input file
}
)";
}

int main(int argc, char **argv)
{
  cout << "miniedit v" << MAJOR << "." << MINOR << endl;

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

    cart c(jconf);
    c.update_degree();

    if ( jconf.has_member("enable_merge_subgraph") && jconf["enable_merge_subgraph"].as<bool>() )
      c.merge_subgraph();

    if ( jconf.has_member("enable_remove_degree2") && jconf["enable_remove_degree2"].as<bool>() )
      c.remove_degree2();

    if ( jconf.has_member("enable_attach_nodes") && jconf["enable_attach_nodes"].as<bool>() )
    {
      c.attach_nodes();
      c.remove_degree2();
      c.remove_shortp();
    }

    if ( jconf.has_member("enable_assign_level") )
      c.assign_level_ps(jconf["enable_assign_level"]["grid_file"].as<std::string>());

    if ( jconf.has_member("bridge_file") )
      c.assign_level_bridge(jconf["bridge_file"].as<std::string>());

    if ( jconf.has_member("enable_reduction") )
      c.reduce_area(jconf["enable_reduction"]);

    if ( jconf.has_member("subgraph_removal") )
      c.remove_subgraph(jconf["subgraph_removal"]);

    c.dump_edited();
  }
  catch (exception &e)
  {
    cerr << "EXC: " << e.what() << endl;
    exit(1);
  }

  return 0;
}
