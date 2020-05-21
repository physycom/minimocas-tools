#include <iostream>
#include <algorithm>
#include <numeric>
#include <random>

#include <physycom/combinatoric.hpp>
#include <physycom/histo.hpp>
#include <physycom/string.hpp>

#include <carto.h>

using namespace std;
using namespace jsoncons;

constexpr int MAJOR = 0;
constexpr int MINOR = 0;

constexpr int node_station = 1350;
constexpr int node_sanmarco = 4944;

void usage(const char* progname)
{
  string pn(progname);
  cerr << "Usage: " << pn.substr(pn.find_last_of("/\\") + 1) << " path/to/json/config" << endl;
  cerr << R"(JSON CONFIG SAMPLE
{
  "file_pro"     : "roads.pro",      // poly properties input file
  "file_pnt"     : "roads.pnt",      // poly geometry input file
  "explore_node" : [ 1116 ],         // list of bproutes
  "origin"       : [ 1, 2 ],
  "poi"          : [ 3, 4 ]
}
)";
}

int main(int argc, char** argv)
{
  cout << "route_generator v" << MAJOR << "." << MINOR << endl;

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

    // Init cart
    cart c(jconf);
    c.dump_routes("routes_map.csv");

    physycom::multihisto<double> mh;
    mh.add_histo("route-dist-all", 0., 15000., 50, "Distance [m]", "Counter", "Route Length Distribution");
    mh.add_histo("route-dist", 0., 15000., 250., "Distance [m]", "Counter", "Route Length Distribution");
    mh.add_histo("route-ndest", 2., 5+2., 1., "Number of dest", "Counter", "Number of Destination per Route Distribution");

    for(const auto &r : c.routes)
    {
      mh.count("route-dist-all", "route", r.second.dist);
      mh.count("route-dist", "route-" + to_string(r.first.size()), r.second.dist);
      mh.count("route-ndest", "route", r.first.size());
    }

    mh.dump("histo/");
    mh.gnuplot("histo/");
  }
  catch (exception &e)
  {
    cerr << "EXC: " << e.what() << endl;
    exit(1);
  }
}
