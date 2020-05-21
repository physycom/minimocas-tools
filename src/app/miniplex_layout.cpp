/*!
 *  \file   miniplex_layout.cpp
 *  \author A. Fabbri (alessandro.fabbri27@unibo.it), C. Mizzi (chiara.mizzi2@unibo.it)
 *  \brief  Graphical widget implementation file for app `miniplex`.
 *  \details This file contains the implementation of the widgets' structure and details realated to `miniplex` app.
 */

#include <iostream>
#include <miniplex_gui.h>

Fl_Window *form;
gui::gl_scene *scene;
Fl_Text_Display *info_box;
Fl_Value_Slider *zoom_slider;
Fl_Button *button_exit;
Fl_Text_Buffer *text_buff;

gui::gui(multiplex *m)
{
  // Get screen resolution and adjust window dimension to 90% of width and height
  int form_x, form_y, form_w, form_h;
  Fl::screen_work_area(form_x, form_y, form_w, form_h, 0);
  form_w = int(form_w * 0.9);
  form_h = int(form_h * 0.9);

  // Geometric composition parameters
  int scene_h = int(form_h * 0.9);
  int scene_w = int(form_w * 0.7);
  int button_h = int(form_h * 0.03);
  int button_w = int(form_w * 0.26);
  int spacing = int(form_h * 0.02);
  int widget_offset_h = spacing + scene_w + spacing;
  int widget_offset_v = spacing;

  // Populating widget
  text_buff = new Fl_Text_Buffer();
  form = new Fl_Window(form_x, form_y, form_w, form_h, "miniplex debug box"); // Main window
  scene = new gl_scene(m, spacing, spacing, scene_w, scene_h);              // Map box

  // coordinates box
  info_box = new Fl_Text_Display(widget_offset_h, widget_offset_v, button_w, 10 * button_h, "info");
  info_box->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, Fl_Text_Display::WRAP_AT_COLUMN);
  info_box->align(FL_ALIGN_BOTTOM);
  info_box->textfont(FL_COURIER);
  info_box->textsize(11);
  text_buff->text("info box : mouse over the map to view content");
  info_box->buffer(text_buff);
  widget_offset_v += (spacing + 10 * button_h + spacing);

  // zoom button
  zoom_slider = new Fl_Value_Slider(widget_offset_h, widget_offset_v, button_w, button_h, "zoom");
  zoom_slider->type(FL_HOR_SLIDER);
  zoom_slider->bounds(0.1, 100.0);
  zoom_slider->value(scene->zoom_val);
  zoom_slider->callback([](Fl_Widget *)
  {
    scene->zoom_val = zoom_slider->value();
    scene->reDraw = true;
  });
  widget_offset_v += (spacing + button_h + spacing);

  // Exit button
  button_exit = new Fl_Button(form_w - button_w / 2 - spacing, form_h - button_h - spacing, button_w / 2, button_h, "exit");
  button_exit->callback([](Fl_Widget *)
  {
    form->hide();
  });

  form->end();
  form->show();
  scene->show();

  // Register idle callback
  Fl::add_idle([](void *)
  {
    if (scene->reDraw)
    {
      scene->redraw();
    }
  });
}

void gui::run() { Fl::run(); }
