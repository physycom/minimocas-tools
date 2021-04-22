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
#include <physycom/string.hpp>

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

  "enable_merge_subgraph" : false,
  "enable_remove_degree2" : false,
  "enable_attach_nodes"   : false,
  "enable_reduction"      : false
}
)";
}

struct ace
{
  std::vector<point_base> points;
  double area, perimeter;
  point_base centroid;
  std::map<std::string, std::string> pro;
  int ilat_max, ilat_min, ilon_max, ilon_min;
  std::vector<node_it> node_in;
  int population;
  double score;
  ace() {};
  ace(const jsoncons::json &feature);
};

ace::ace(const jsoncons::json &polygonlist)
{
  for (const auto &pt : polygonlist[0].array_range())
  {
    points.emplace_back(int(pt[1].as<double>() * 1e6), int(pt[0].as<double>() * 1e6));
  }
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

    if (jconf.has_member("enable_merge_subgraph") && jconf["enable_merge_subgraph"].as<bool>())
      c.merge_subgraph();

    if (jconf.has_member("enable_remove_degree2") && jconf["enable_remove_degree2"].as<bool>())
      c.remove_degree2();

    if (jconf.has_member("enable_attach_nodes") && jconf["enable_attach_nodes"].as<bool>())
    {
      c.attach_nodes();
      c.remove_degree2();
      c.remove_shortp();
    }

    if (jconf.has_member("enable_assign_level"))
      c.assign_level_ps(jconf["enable_assign_level"]["grid_file"].as<std::string>());

    if (jconf.has_member("bridge_file"))
      c.assign_level_bridge(jconf["bridge_file"].as<std::string>());

    if (jconf.has_member("enable_reduction"))
      c.reduce_area(jconf["enable_reduction"]);

    if (jconf.has_member("subgraph_removal"))
      c.remove_subgraph(jconf["subgraph_removal"]);

    if (jconf.has_member("enable_merge_carto")) {
      auto fname = jconf["second_carto_json"].as<std::string>();
      json jconf2 = json::parse_file(fname);
      cart c2(jconf2);
      merge_carto(c, c2);
    }

    if (jconf.has_member("enable_merge_ace")) {

      std::cout << "--- Merge ACE modality ---" << std::endl;

      std::map<std::string, ace> polygons_ace;
      int polygon_cnt = 0;

      std::ifstream ace_j(jconf["ace_geojson"].as<std::string>());
      if (!ace_j)
        throw std::runtime_error("unable to open polygons file " + jconf["ace_geojson"].as<std::string>());

      auto jpoly = jsoncons::json::parse(ace_j);

      for (const auto &feature : jpoly["features"].array_range())
      {
        if (!feature["geometry"]["coordinates"].size()) continue;
        auto type = feature["geometry"]["type"].as<std::string>();
        auto reg = feature["properties"]["REG_NAME"].as<std::string>();

        if (reg == "Emilia-Romagna")
        {
          if (type == "MultiPolygon")
          {
            //std::cout << "feature type MultiPolygon size " << feature["geometry"]["coordinates"].size() << std::endl;
            for (const auto &pol : feature["geometry"]["coordinates"].array_range())
            {
              ++polygon_cnt;
              ace aw = ace(pol);
              for (const auto &pro : feature["properties"].object_range())
                aw.pro[std::string(pro.key())] = pro.value().as<std::string>();
              string uid = aw.pro["UID"];
              aw.centroid = physycom::centroid(aw.points);
              if (polygons_ace.find(uid) == polygons_ace.end())
                polygons_ace[uid] = aw;
            }
          }
          else if (type == "Polygon")
          {
            //std::cout << "feature type Polygon size " << feature["geometry"]["coordinates"].size() << std::endl;
            ++polygon_cnt;
            auto pol = feature["geometry"]["coordinates"];
            ace aw = ace(pol);
            for (const auto &pro : feature["properties"].object_range())
              aw.pro[std::string(pro.key())] = pro.value().as<std::string>();
            string uid = aw.pro["UID"];
            aw.centroid = physycom::centroid(aw.points);
            if (polygons_ace.find(uid) == polygons_ace.end())
              polygons_ace[uid] = aw;
          }
          else
          {
            std::cerr << "GEOJSON feature type " << type << " unsupported" << std::endl;
          }
        }
      }

      std::cout << "num ace        : " << polygons_ace.size() << std::endl;

      // read popultion properties
      std::ifstream ace_pop(jconf["population_file"].as<std::string>());
      if (!ace_pop)
        throw std::runtime_error("unable to open polygons file " + jconf["population_file"].as<std::string>());
      
      std::string line;
      std::getline(ace_pop, line); // skip header

      int population_total = 0;
      while (std::getline(ace_pop, line))
      {
        std::vector<std::string> tok;
        std::string sep = ";";
        physycom::split(tok, line, sep, physycom::token_compress_off);
        if (polygons_ace.find(tok[0])!=polygons_ace.end()){
          polygons_ace[tok[0]].population = stoi(tok[1]);
          population_total += stoi(tok[1]);
        }
      }

      std::cout << "totale pop     : " << population_total << std::endl;

      // find intersection ace-nodes
      point_base corn_box_p;
      corn_box_p.ilat = c.ilat_max + 10;
      corn_box_p.ilon = c.ilon_max + 10;

      for (auto &pga : polygons_ace)
      {
        //assign relative score
        pga.second.score = (pga.second.population*1e4) / (float)population_total;
        std::set<node_it> node_closest;
        pga.second.ilat_max = 0;
        pga.second.ilat_min = c.ilat_max;
        pga.second.ilon_max = 0;
        pga.second.ilon_min = c.ilon_max;

        for (const auto &cord : pga.second.points) {
          if (cord.ilat > pga.second.ilat_max) pga.second.ilat_max = cord.ilat;
          if (cord.ilat < pga.second.ilat_min) pga.second.ilat_min = cord.ilat;
          if (cord.ilon > pga.second.ilon_max) pga.second.ilon_max = cord.ilon;
          if (cord.ilon < pga.second.ilon_min) pga.second.ilon_min = cord.ilon;
        }
        int row_max, col_max;
        int row_min, col_min;
        c.grid.coord_to_grid(pga.second.ilat_max, pga.second.ilon_max, row_max, col_max);
        c.grid.coord_to_grid(pga.second.ilat_min, pga.second.ilon_min, row_min, col_min);


        for (int rw = row_min; rw < row_max; ++rw) 
        {
          for (int cl = col_min; cl < col_max; ++cl)
          {
            for (const auto &n : c.grid.grid[rw][cl].node)
              node_closest.insert(n);
          }
        }

        // for each node find membership
        for (const auto &n : node_closest)
        {
          int n_intersect = 0;
          for (int bc = 0; bc < int(pga.second.points.size() - 1); ++bc)
          {
            point_base pw;
            pw.ilat = n->ilat;
            pw.ilon = n->ilon;
            n_intersect += c.find_intersection(pw, corn_box_p, pga.second.points[bc], pga.second.points[bc + 1]);
            
          }
          if (n_intersect > 0) pga.second.node_in.push_back(n);
        }
      }


      string basename;
      if (jconf.has_member("cartout_basename"))
        basename = jconf["cartout_basename"].as<std::string>();
      else
        basename = "ace_score";
      
      ofstream out_score(basename + ".csv");
      out_score << "node_cid;node_lid;score;city" << std::endl;
      for (const auto &pg : polygons_ace)
        for (const auto &n : pg.second.node_in)
          out_score << n->cid << ";" << n->lid << ";" << pg.second.score<<";"<< pg.second.pro.at("COM_NAME") << std::endl;
        
      std::cout << "Score ace file dumped in: " << basename + ".csv" << std::endl;
      return 0;
    }
    
    c.dump_edited();
  }
  catch (exception &e)
  {
    cerr << "EXC: " << e.what() << endl;
    exit(1);
  }

  return 0;
}
