/*!
 *  \file   miniplex_scene.cpp
 *  \author A. Fabbri (alessandro.fabbri27@unibo.it), C. Mizzi (chiara.mizzi2@unibo.it)
 *  \brief  OpenGL based methods implementations for app `miniplex`.
 *  \details This file contains the implementation of the graphic method, based on the FLTK wrapper of OpenGL, realated to `miniplex` app.
 */

#include <miniplex_gui.h>
#include <color_palette.hpp>

extern Fl_Value_Slider *zoom_slider;
extern Fl_Text_Display *info_box;
extern Fl_Text_Buffer *text_buff;
extern Fl_Input *select_input;
extern gui::gl_scene *scene;

constexpr double VIEW_BLANK_BOUNDARY_FACTOR = 1.2;
constexpr double SCROLL_TO_ZOOM = 0.125;

gui::gl_scene::gl_scene(multiplex* mu, int x, int y, int w, int h, const char *l) :
Fl_Gl_Window(x, y, w, h, l),
reDraw(true),
frame_number(0)
{
  m = mu;
  zoom_val = 1.0;
  ilatshift = 0;
  ilonshift = 0;
  ilatorig = 0;
  ilonorig = 0;

  // local coordinate geometry
  ilatmin = m->ilat_min;
  ilatmax = m->ilat_max;
  ilonmin = m->ilon_min;
  ilonmax = m->ilon_max;
  iloncen = int(0.5 * (ilonmax + ilonmin));
  ilatcen = int(0.5 * (ilatmax + ilatmin));
  dilat = int(0.5 * (ilatmax - ilatmin) * VIEW_BLANK_BOUNDARY_FACTOR);
  dilon = int(0.5 * (ilonmax - ilonmin) * VIEW_BLANK_BOUNDARY_FACTOR);
  lateps = dilat / 1000;
  loneps = ( lateps * this->w() ) / this->h();
  mousept = point_base();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////// DRAW BASIC OBJECT
////////////////////////////////////////////////////////////////////////////////////////////////////

template<>
void gui::gl_scene::draw(const int &x, const int &y, const int &w, const int &h, const int &r, const int &g, const int &b, const double &z)
{
  glPushMatrix();
  glBegin(GL_QUADS);
  glColor3d(r/255.f, g/255.f, b/255.f);
  glVertex3d(GLdouble(x - iloncen),     GLdouble(y - ilatcen)    , GLdouble(z));
  glVertex3d(GLdouble(x + w - iloncen), GLdouble(y - ilatcen)    , GLdouble(z));
  glVertex3d(GLdouble(x + w - iloncen), GLdouble(y + h - ilatcen), GLdouble(z));
  glVertex3d(GLdouble(x - iloncen),     GLdouble(y + h - ilatcen), GLdouble(z));
  glEnd();
  glPopMatrix();
}

template<>
void gui::gl_scene::draw(const int &x1, const int &y1, const int &x2, const int &y2)
{
  glBegin(GL_LINE_STRIP);
  glVertex3d(GLdouble(x1 - iloncen), GLdouble(y1 - ilatcen), 0.);
  glVertex3d(GLdouble(x2 - iloncen), GLdouble(y2 - ilatcen), 0.);
  glEnd();
}

void gui::gl_scene::draw_line(const point_base &pt1, const point_base &pt2)
{
  draw(pt1.ilon, pt1.ilat, pt2.ilon, pt2.ilat);
}

template<>
void gui::gl_scene::draw(const arc_base &a)
{
  draw_line(*a.ptF, *a.ptT);
}

template<>
void gui::gl_scene::draw(const poly_base &p)
{
  glBegin(GL_LINE_STRIP);
  for (const auto &pt : p.point) glVertex3d(GLdouble(pt->ilon - iloncen), GLdouble(pt->ilat - ilatcen), 0.);
  glEnd();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////// DRAW SCENE COMPONENTS
////////////////////////////////////////////////////////////////////////////////////////////////////

// draw multiplex
void gui::gl_scene::draw_multiplex()
{
  glPushMatrix();
  double red_canal = 0.4;
  for (const auto &l : m->layers)
  {
    for (const auto &p : l.second.poly)
    {
      glLineWidth(1.);
      //glColor3d(0.4, 0.4, 0.4);
      glColor3d(red_canal, 0.4, 0.4);
      draw(p);
    }
    red_canal += 0.2;
  }
  glPopMatrix();
}

// draw scene
void gui::gl_scene::draw_scene()
{
  ++frame_number;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glScaled(zoom_val, zoom_val, 1);
  glTranslated(ilonshift, ilatshift, 0);
  draw_multiplex();
  glPopMatrix();
}

void gui::gl_scene::draw()
{
  if (!valid())
  {
    glClearColor(242.f / 255, 230.f / 255, 196.f / 255, 1.0f);   // Set background color : giallino
    glViewport(0, 0, this->w(), this->h());                        // Make our viewport the whole window
    glMatrixMode(GL_PROJECTION);                                   // Select The Projection Matrix
    glLoadIdentity();                                              // Reset The Projection Matrix
    gluOrtho2D(-dilon, dilon, -dilat, dilat);                      // Left Right Bottom Top
    glMatrixMode(GL_MODELVIEW);                                    // Select The Modelview Matrix
    glLoadIdentity();                                              // Reset The Modelview Matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);            // Clear The Screen And The Depth Buffer
    glLoadIdentity();
    // Reset The View
    glEnable(GL_DEPTH_TEST);
    valid(true);
  }
  draw_scene();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////// USER INTERACTION
////////////////////////////////////////////////////////////////////////////////////////////////////

void gui::gl_scene::update_infobox()
{
  std::stringstream infoss;
  infoss << "Coords           : " << mousept.ilat << " " << mousept.ilon << std::endl
         << "Grid cell        : " << mouserow << ", " << mousecol << std::endl
         << "Frame number     : " << frame_number << std::endl;
  text_buff->text(infoss.str().c_str());
  info_box->buffer(text_buff);
}

int gui::gl_scene::handle(int event)
{
  switch (event)
  {
  case FL_PUSH:
    screen_to_coord(ilatorig, ilonorig);
    ilatorig -= ilatshift;
    ilonorig -= ilonshift;
    break;
  case FL_DRAG:
    if ( Fl::event_button1() )
    {
      screen_to_coord(ilatshift, ilonshift);
      ilonshift -= ilonorig;
      ilatshift -= ilatorig;
      reDraw = true;
    }
    break;
  case FL_MOUSEWHEEL:
    zoom_val -= Fl::event_dy() * SCROLL_TO_ZOOM;
    zoom_val = (zoom_val < zoom_slider->minimum()) ? zoom_slider->minimum() : zoom_val;
    zoom_val = (zoom_val > zoom_slider->maximum()) ? zoom_slider->maximum() : zoom_val;
    zoom_slider->value(zoom_val);
    reDraw = true;
    break;
  case FL_MOVE:
  {
    screen_to_coord(mousept);
    mousept.ilat -= ilatshift;
    mousept.ilon -= ilonshift;
    update_infobox();
    reDraw = true;
    break;
  }
  default:
    return Fl_Gl_Window::handle(event);
  }
  return 1;
}
