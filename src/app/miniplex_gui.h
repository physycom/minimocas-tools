/*!
 *  \file   miniplex_gui.h
 *  \author A. Fabbri (alessandro.fabbri27@unibo.it), C. Mizzi (chiara.mizzi2@unibo.it)
 *  \brief  Support header for the graphical part of `miniplex` app.
 *  \details This support header file contains the principal definition for the graphical part of `miniplex` app.
 */

#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/glu.h>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Window.H>

#include <multiplex.h>

///////////////////////////////////////////////////////////////
////// FLTK GUI CLASS
///////////////////////////////////////////////////////////////

#define FRAME_PER_DT      2000

class gui
{
public:
  gui(multiplex* m);
  void run();

  class gl_scene : public Fl_Gl_Window
  {
  public:
    gl_scene(multiplex* m, int x, int y, int w, int h, const char *l = 0);
    void draw();
    int handle(int event);

    inline void screen_to_coord(int &ilat, int &ilon)
    {
      ilat = int( ((this->h() - Fl::event_y()) * 2. * dilat / this->h() - dilat) / zoom_val + ilatcen );
      ilon = int( (Fl::event_x() * 2. * dilon / this->w() - dilon) / zoom_val + iloncen );
    }
    inline void screen_to_coord(point_base &pt) { screen_to_coord(pt.ilat, pt.ilon); }

    void update_infobox();

  private:
    friend class gui;
    multiplex *m;
    int ilatmin, ilatmax, ilonmin, ilonmax;
    int iloncen, ilatcen, dilat, dilon;
    int lateps, loneps;

    double zoom_val;
    int ilatshift, ilonshift, ilatorig, ilonorig;

    bool reDraw;
    point_base mousept;
    int mouserow, mousecol;
    int frame_number;

    // draw whole scene
    void draw_scene();
    // draw components
    void draw_multiplex();

    // draw basic object
    template<typename... Args> void draw(const Args &...args);
    void draw_line(const point_base &pt1, const point_base &pt2);
  };
};
