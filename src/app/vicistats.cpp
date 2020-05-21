#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include <jsoncons/json.hpp>

#include <physycom/histo.hpp>

#include <simulation.h>

using namespace std;
using namespace jsoncons;

constexpr int MAJOR = 1;
constexpr int MINOR = 1;

void usage(const char* progname)
{
  string pn(progname);
  cerr << "Usage: " << pn.substr(pn.find_last_of("/\\") + 1) << " path/to/json/config" << endl;
  cerr << R"(JSON CONFIG SAMPLE
{
  "file_pro"     : "roads.pro",           // poly properties input file
  "file_pnt"     : "roads.pnt",           // poly geometry input file
  "start_time"   : 0,                     // start unix time
  "stop_time"    : 3600,                  // stop unix time
  "start_date"   : "2018-09-11 11:00:00", // start date time UTC+1 (overwrites the corresponding timestamp)
  "stop_date"    : "2018-09-11 12:00:00", // stop date time UTC+1 (overwrites the corresponding timestamp)
  "dt"           : 5,                     // simulation time step [seconds]
  "sampling_dt"  : 300,                   // data collection subsampling [seconds]
  "out_basename" : "test",                // simulation output data basename
  "pawns"        :                        // pawns spec subjson
  {
    "type_name" : {
      "number"         : 5,
      "start_poly_lid" : 0,
      "speed_mps"      : 1.0
    }
  },
  "sources_" : {
    "railstation" : {
      "poly_lid"      : 6301,
      "creation_dt"   : 600,
      "creation_rate" : 10,
      "pawns": { ... }                    // valid pawn spec subjson
    }
  },
  "server"       :                        // server data
  {
    "ip"      : "127.0.0.1",              // server ip [string]
    "port"    : 12345                     // server port [unsigend short]
  },
  "stats"        :                        // statistics histogram specs
  {
    "trip_time_max"  : 24,                // max for trip time histo range [h]
    "trip_time_binw" : 0.5                // trip time histo binw [h]
  }
}
)";
}

int main(int argc, char** argv)
{
  cout << "ViCi Stats v" << MAJOR << "." << MINOR << endl;

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
    json jconf;
    jconf = json::parse_file(conf);

    // Init cart
    cart c(jconf);

    if (jconf.has_member("histo"))
    {
      cout << "---- HISTO mode ----" << endl;
      // init simulation
      simulation s(jconf, &c);

      // histo config
      physycom::multihisto<double> mh;
      auto time_max  = jconf["histo"].has_member("trip_time_max")  ? jconf["histo"]["trip_time_max"].as<double>()  : 24.0;
      auto time_min  = jconf["histo"].has_member("trip_time_min")  ? jconf["histo"]["trip_time_min"].as<double>()  : 0.0;
      auto time_binw = jconf["histo"].has_member("trip_time_binw") ? jconf["histo"]["trip_time_binw"].as<double>() : 0.5;
      auto life_max  = jconf["histo"].has_member("life_time_max")  ? jconf["histo"]["life_time_max"].as<double>()  : 24.0;
      auto life_min  = jconf["histo"].has_member("life_time_min")  ? jconf["histo"]["life_time_min"].as<double>()  : 0.0;
      auto life_binw = jconf["histo"].has_member("life_time_binw") ? jconf["histo"]["life_time_binw"].as<double>() : 0.5;
      auto dist_max  = jconf["histo"].has_member("cov_dist_max")   ? jconf["histo"]["cov_dist_max"].as<double>()   : 10000.0;
      auto dist_min  = jconf["histo"].has_member("cov_dist_min")   ? jconf["histo"]["cov_dist_min"].as<double>()   : 0.0;
      auto dist_binw = jconf["histo"].has_member("cov_dist_binw")  ? jconf["histo"]["cov_dist_binw"].as<double>()  : 200;

      mh.add_histo("triptime", time_min, time_max, time_binw, "time [h]", "Counter", "Trip Time Distribution");
      mh.add_histo("lifetime", life_min, life_max, life_binw, "time [h]", "Counter", "Life Time Distribution");
      mh.add_histo("covdist", dist_min, dist_max, dist_binw, "covered distance [m]", "Counter", "Covered Distance Distribution");

      // diffusion output file
      ofstream outdiff("stats/diffusion.csv");
      char sep = ';';
      outdiff << "time";
      for(const auto &t : s.pawns) outdiff << sep << "ave_dist_" << t.front().tag;
      outdiff << sep << "ave_dist_tot" << std::endl;

      // population output file
      ofstream pop("stats/population.csv");
      pop << "timestamp[h]" << sep;
      for(const auto &t : s.pawns) pop << t.front().tag << sep;
      pop << endl;

      // run
      s.run([&pop, &s, &outdiff, &sep](){

        pop << (s.sim_time - s.start_time) / 3600.0;
        outdiff << (s.sim_time - s.start_time);

        //double ave_dist = 0.;
        std::map<string, double> dist_tot;
        std::map<string, int> alive_tot;
        for (auto &type : s.pawns)
        {
          auto tag = type.front().tag;
          for (auto &p : type)
          {
            if ( p.status != PAWN_DEAD )
            {
              alive_tot[p.tag]++;
              dist_tot[p.tag] += p.totdist;
            }
          }
          pop << sep << alive_tot[tag];
          outdiff << sep << dist_tot[tag] / double(alive_tot[tag]);
        }
        outdiff << sep <<
          std::accumulate( dist_tot.begin(), dist_tot.end(), 0., [](double sum, auto i){
            return sum + i.second;
          })
          /
          std::accumulate( alive_tot.begin(), alive_tot.end(), 0., [](double sum, auto i){
            return sum + i.second;
          });

        pop << endl;
        outdiff << endl;

      }, [](){});
      outdiff.close();

      // Raw data (external fitting purposes)
      std::ofstream outraw("stats/pawn_raw.csv");
      outraw << "id" << sep << "triptime" << sep << "lifetime" << sep << "dist" << sep << "tag" << std::endl;

      // Collect results
      for (auto &type : s.pawns)
      {
        for (auto &p : type)
        {
          outraw << p.id << sep << p.triptime << sep << p.lifetime << sep << p.totdist << sep << p.tag << std::endl;

          mh.count("covdist", p.tag, p.totdist);
          mh.count("triptime", p.tag, p.triptime / 3600.0);
          mh.count("lifetime", p.tag, p.lifetime / 3600.0);
          mh.count("covdist", p.tag + "-route", c.routes[p.route_id].dist);
        }
      }
      pop.close();
      outraw.close();

      mh.dump("histo/");
      mh.gnuplot("histo/");
      //mh.gnuplot_cdf("histo/");
    }
    else if (jconf.has_member("attr_stats"))
    {
      cout << "---- ATTR_STATS mode ----" << endl;
      // init simulation
      simulation s(jconf, &c);

      // attractions counter
      struct attrcnt
      {
        int queued, visiting;
        attrcnt() : queued(0), visiting(0) {}
      };
      std::map<int, std::string> attr_node;
      for(const auto &a : s.attractions) attr_node[a.node_lid] = a.tag;

      // attractions output
      char sep = ';';
      std::ofstream attrout("stats/attraction_pop.csv");
      attrout << "time";
      for(const auto &a : attr_node) attrout << sep << a.second + "-queued" << sep << a.second + "-visiting" << sep << a.second + "-mqt";
      attrout << std::endl;

      s.run([&s, &sep, &attrout, &attr_node]()
      {
        std::map<std::string, attrcnt> attr_cnt;
        for(const auto &type : s.pawns)
        {
          for(const auto &p : type)
          {
            switch(p.status)
            {
              case PAWN_QUEUED:
                attr_cnt[attr_node[p.last_node]].queued++;
                break;
              case PAWN_VISITING:
                attr_cnt[attr_node[p.last_node]].visiting++;
                break;
              default:
                break;
            }
          }
        }

        attrout << (s.sim_time - s.start_time);
        for(const auto &a : attr_node)
        {
          auto attr_obj = s.node_attractions[a.first].second;
          auto mean_queue_time = attr_obj->rate_in == 0 ? -1 : (attr_obj->queue.size() * s.sampling_dt) / attr_obj->rate_in;
          attrout << sep << attr_cnt[a.second].queued << sep << attr_cnt[a.second].visiting << sep << mean_queue_time;
          attr_obj->rate_in = 0;
        }
        attrout << std::endl;
      }, [](){});
      attrout.close();
    }
    else if (jconf.has_member("stats"))
    {
      cout << "---- STATS mode ----" << endl;
      // parse zones stats
      map<string, int> zone_cnt;
      map<string, int> zone_mask;
      if (jconf["stats"].has_member("zones"))
      {
        for (const auto &cl : jconf["stats"]["zones"].object_range())
        {
          zone_cnt[string(cl.key())] = cl.value()["poly_lid"].as<int>();
          zone_mask[string(cl.key())] = cl.value()["mask_size"].as<int>();
        }
      }

      // population output file
      ofstream pop("stats/population.txt");
      pop << "timestamp[h];total;";
      for (const auto &p : zone_cnt) pop << p.first << ";";
      pop << endl;

      // init and run simulation
      simulation s(jconf, &c);

      // eta output file
      ofstream eta("stats/eta.txt");
      eta << "timestamp[h];";
      for (const auto &r : s.routes) eta << r.first << ";";
      eta << endl;

      // cam output file
      ofstream cam("stats/cam.txt");
      cam << "timestamp;";
      for (const auto &c : s.locations) cam << c.first << ";";
      cam << endl;

      s.run([&pop, &c, &s, &zone_cnt, &zone_mask, &eta, &cam]() {
      // s.run([&pop, &c, &s, &zone_cnt, &eta, &cam]() {
        // populate grid counter
        auto grid_state = vector<int>(c.grid.gcol*c.grid.grow, 0);
        for (const auto &type : s.pawns)
          for (const auto &p : type)
          {
            auto pt = c.poly[p.current_poly].get_point(p.current_s);
            int row = (pt.ilat - c.grid.gilatmin) / c.grid.gdlat;
            int col = (pt.ilon - c.grid.gilonmin) / c.grid.gdlon;
            ++grid_state[row*c.grid.gcol + col];
          }

        // dump total population
        pop << (s.sim_time - s.start_time) / 3600.0 << ";";
        pop << accumulate(s.pawns.begin(), s.pawns.end(), 0, [](int sum, const vector<pawn> &type) {
          return sum + int(type.size());
        }) << ";";

        // accumulate and dump monitoring zone population
        int cnt_row, cnt_col;
        map<string, int> cnt;
        for (const auto &p : zone_cnt)
        {
          c.get_node_cell(c.poly[p.second].nT, cnt_row, cnt_col);
          int mask_size = zone_mask[p.first];
          // constexpr int mask_size = 20;
          for (int row = cnt_row - mask_size; row < cnt_row + mask_size + 1; ++row)
            for (int col = cnt_col - mask_size; col < cnt_col + mask_size + 1; ++col)
              cnt[p.first] += grid_state[row*c.grid.gcol + col];

          pop << cnt[p.first] << ";";
        }
        pop << endl;

        // evaluate routes eta and dump
        eta << (s.sim_time - s.start_time) / 3600.0 << ";";
        for (const auto &r : s.routes)
        {
          double time = 0.0;
          for (const auto &p : r.second)
            time += c.poly[p].length / c.poly[p].speed;
          eta << time / 3600.0 << ";";
        }
        eta << endl;

        // dump location counter
        cam << (s.sim_time - s.start_time) / 3600.0 << ";";
        for (const auto &c : s.locations)
          cam << c.second.a->p->counter << ";";
        cam << endl; }, []() {});

      pop.close();
      eta.close();

      c.dump_polyusage();
    }
    else if ( jconf.has_member("control_poly") )
    {
      cout << "---- CONTROL mode ----" << endl;

      // control container
      vector<int> controlcnt(c.poly.size());

      // control output file
      ofstream control("control/control.txt");
      if(!control)
      {
        cerr << "Unable to create control file 'control/control.txt'. Missing folder?" << endl;
        exit(2);
      }
      control << "timestamp;trel;";
      for(const auto &c : jconf["control_poly"].object_range()) control << c.key() << ";";
      control << endl;

      simulation s(jconf, &c);
      s.run([&s, &controlcnt, &jconf, &control]() {
        fill(controlcnt.begin(), controlcnt.end(), 0);
        for(const auto &type : s.pawns)
          for(const auto &p : type)
            ++controlcnt[p.current_poly];

        control << s.sim_time << ";" << s.sim_time - s.start_time << ";";
        for(const auto &c : jconf["control_poly"].object_range())
          control << controlcnt[c.value().as<int>()] << ";";
        control << endl;
      }, [](){});

      control.close();
    }
    else
    {
      cout << "---- GRID mode ----" << endl;

      simulation s(jconf, &c);
      s.run([&s]() {
        s.dump_influxgrid();
      }, [&s](){
        s.dump_state_json();
      });
    }
  }
  catch (exception &e)
  {
    cerr << "[vicistats] EXC: " << e.what() << endl;
  }
}
