#include "viewer.hpp"
#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>
#include "a2.hpp"
#include "draw.hpp"

Viewer::Viewer() {
  Glib::RefPtr<Gdk::GL::Config> glconfig;

  // Ask for an OpenGL Setup with
  //  - red, green and blue component colour
  //  - a depth buffer to avoid things overlapping wrongly
  //  - double-buffered rendering to avoid tearing/flickering
  glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB   |
                                     Gdk::GL::MODE_DEPTH |
                                     Gdk::GL::MODE_DOUBLE);
  if (glconfig == 0) {
    // If we can't get this configuration, die
    std::cerr << "Unable to setup OpenGL Configuration!" << std::endl;
    abort();
  }

  // Accept the configuration
  set_gl_capability(glconfig);

  // Register the fact that we want to receive these events
  add_events(Gdk::BUTTON1_MOTION_MASK    |
             Gdk::BUTTON2_MOTION_MASK    |
             Gdk::BUTTON3_MOTION_MASK    |
             Gdk::BUTTON_PRESS_MASK      |
             Gdk::BUTTON_RELEASE_MASK    |
             Gdk::VISIBILITY_NOTIFY_MASK);


  // Construct scene graph (it's linear):
  // (rootNode, Node, perspective mat)
  // ->(worldNode, Gnomon, viewing mat)
  //   ->(modelNode, Gnomon, model trans/rot mat)
  //     ->(cube, Cube, model scaling mat)

  const Colour WHITE = Colour(1);
  const Colour RED = Colour(1, 0, 0);
  const Colour BLUE = Colour(0, 0, 1);

  cube = new Cube();
  cube->setColour(BLUE);

  Gnomon* modelGnomon = new Gnomon();
  modelGnomon->setColour(RED);
  modelNode = modelGnomon; // This gnomon will act as model node.

  modelNode->addChild(cube);

  Gnomon* worldGnomon = new Gnomon();
  worldGnomon->setColour(WHITE);
  worldNode = worldGnomon; // This gnomon will act as world node.

  worldNode->addChild(modelNode);

  rootNode = new Node();
  rootNode->addChild(worldNode);
}

Viewer::~Viewer() {
  // Will cascade delete everything else.
  delete rootNode;
  worldNode = NULL;
  modelNode = NULL;
  cube = NULL;
}

void Viewer::invalidate() {
  // Force a rerender
  Gtk::Allocation allocation = get_allocation();
  get_window()->invalidate_rect( allocation, false);
}

void Viewer::set_perspective(double fov, double aspect,
                             double near, double far) {
  Matrix4x4 m = perspective(fov, aspect, near, far);
  rootNode->setTransform(m);
}

void Viewer::reset_view() {
  set_perspective(M_PI/4.0, 1.0, 1.0, 10.0);
  Matrix4x4 viewingMatrix = translation(Vector3D(-0.5, -0.5, -8.0));
  worldNode->setTransform(viewingMatrix);
  modelNode->resetTransform();
  cube->resetTransform();

  m_screen = translation(Vector3D(get_width()/2.0, get_height()/2.0, 0.0))
    * scaling(Vector3D(get_width(), get_height(), 1.0));
}

void Viewer::on_realize() {
  // Do some OpenGL setup.
  // First, let the base class do whatever it needs to
  Gtk::GL::DrawingArea::on_realize();

  Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();

  if (!gldrawable)
    return;

  if (!gldrawable->gl_begin(get_gl_context()))
    return;

  gldrawable->gl_end();

  reset_view();
  // TEMP
  /*
  std::cout << (m_perspective * Point4D(0, 0, -1.0)).homogonize() << std::endl;
  std::cout << (m_perspective * Point4D(0, 0, -4.0)).homogonize() << std::endl;
  std::cout << (m_perspective * Point4D(0, 0, -10.0)).homogonize() << std::endl;
  std::cout << (m_perspective * Point4D(2.0, 0, -8.0)).homogonize() << std::endl;
  */
}

bool Viewer::on_expose_event(GdkEventExpose* event) {
  Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();

  if (!gldrawable) return false;

  if (!gldrawable->gl_begin(get_gl_context()))
    return false;

  // Here is where your drawing code should go.

  draw_init(get_width(), get_height());

  //set_colour(Colour(0.0, 0.0, 1.0));

  //std::cout << "Drawing cube!" << std::endl;
  //double scale_factor = 60.0;
  //Matrix4x4 scaleit = scaling(Vector3D(scale_factor, scale_factor, scale_factor));

  //Matrix4x4 transformationMatrix = m_perspective * m_view * m_model;

  //cube->setTransform(transformationMatrix); // Convert to homogenous coordinates.
  worldNode->translate(Vector3D(0.1, 0.0, 0.0));
  std::vector<LineSegment4D> lineSegments = rootNode->getTransformedLineSegments();
  renderHomogonousLines(lineSegments);

  /*
  shape->homogonize();
  shape->drawOrtho();
  delete shape;
  */

  /*
  draw_line(Point2D(0.1*get_width(), 0.1*get_height()), 
            Point2D(0.9*get_width(), 0.9*get_height()));
  draw_line(Point2D(0.9*get_width(), 0.1*get_height()),
            Point2D(0.1*get_width(), 0.9*get_height()));

  draw_line(Point2D(0.1*get_width(), 0.1*get_height()),
            Point2D(0.2*get_width(), 0.1*get_height()));
  draw_line(Point2D(0.1*get_width(), 0.1*get_height()), 
            Point2D(0.1*get_width(), 0.2*get_height()));
            */

  draw_complete();

  // Swap the contents of the front and back buffers so we see what we
  // just drew. This should only be done if double buffering is enabled.
  gldrawable->swap_buffers();

  gldrawable->gl_end();

  return true;
}

bool Viewer::homogonousClip(LineSegment4D& line) {
  // TODO.
  double w1 = line.getP1()[3];
  double w2 = line.getP2()[3];
  if (w1 >= -0.001 && w1 <= 0.001) {
    return false;
  } else if (w2 >= -0.001 && w2 <= 0.001) {
    return false;
  }
  return true;
}

void Viewer::renderHomogonousLines(std::vector<LineSegment4D> lineSegments) {
  std::cout << "rendering!" << std::endl;
  for (std::vector<LineSegment4D>::iterator lineIt = lineSegments.begin(); lineIt != lineSegments.end(); lineIt++) {
    LineSegment4D& line = *lineIt;
    bool keep = homogonousClip(line);
    if (!keep) {
      continue;
    }
    Point3D p1 = m_screen * line.getP1().homogonize();
    Point3D p2 = m_screen * line.getP2().homogonize();
    std::cout << "draw_line " << p1 << ", " << p2 << std::endl;
    set_colour(line.getColour());
    draw_line(
      Point2D(p1[0], p1[1]),
      Point2D(p2[0], p2[1])
    );
  }
  std::cout << "done!" << std::endl;
}

/*
std::vector<Point4D> Viewer::createCube() {
  // Create Cube, going around points ccw as per opengl standard.

  std::vector<Point3D> points(12);

  // Back.
  //glNormal3d(0.0, 0.0, -1.0);
  glVertex3d(0.0, 0.0, 0.0);
  glVertex3d(1.0, 0.0, 0.0);
  glVertex3d(1.0, 1.0, 0.0);
  glVertex3d(0.0, 1.0, 0.0);
  draw_line();

  // Front.
  //glNormal3d(0.0, 0.0, 1.0);
  glVertex3d(0.0, 0.0, 1.0);
  glVertex3d(0.0, 1.0, 1.0);
  glVertex3d(1.0, 1.0, 1.0);
  glVertex3d(1.0, 0.0, 1.0);

  // Left.
  //glNormal3d(-1.0, 0.0, 0.0);
  glVertex3d(0.0, 0.0, 0.0);
  glVertex3d(0.0, 1.0, 0.0);
  glVertex3d(0.0, 1.0, 1.0);
  glVertex3d(0.0, 0.0, 1.0);

  // Right.
  //glNormal3d(1.0, 0.0, 0.0);
  glVertex3d(1.0, 0.0, 0.0);
  glVertex3d(1.0, 0.0, 1.0);
  glVertex3d(1.0, 1.0, 1.0);
  glVertex3d(1.0, 1.0, 0.0);

  // Bottom.
  //glNormal3d(0.0, -1.0, 0.0);
  glVertex3d(0.0, 0.0, 0.0);
  glVertex3d(0.0, 0.0, 1.0);
  glVertex3d(1.0, 0.0, 1.0);
  glVertex3d(1.0, 0.0, 0.0);

  // Top.
  //glNormal3d(0.0, 1.0, 0.0);
  glVertex3d(0.0, 1.0, 0.0);
  glVertex3d(1.0, 1.0, 0.0);
  glVertex3d(1.0, 1.0, 1.0);
  glVertex3d(0.0, 1.0, 1.0);
}
*/


bool Viewer::on_configure_event(GdkEventConfigure* event) {
  Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();

  if (!gldrawable) return false;

  if (!gldrawable->gl_begin(get_gl_context()))
    return false;

  gldrawable->gl_end();

  return true;
}

bool Viewer::on_button_press_event(GdkEventButton* event) {
  std::cerr << "Stub: Button " << event->button << " pressed" << std::endl;
  return true;
}

bool Viewer::on_button_release_event(GdkEventButton* event) {
  std::cerr << "Stub: Button " << event->button << " released" << std::endl;
  invalidate(); // TODO: Temp.
  return true;
}

bool Viewer::on_motion_notify_event(GdkEventMotion* event) {
  std::cerr << "Stub: Motion at " << event->x << ", " << event->y << std::endl;
  return true;
}
