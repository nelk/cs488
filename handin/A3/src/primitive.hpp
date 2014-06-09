#ifndef CS488_PRIMITIVE_HPP
#define CS488_PRIMITIVE_HPP

#include "algebra.hpp"
#include <GL/gl.h>

class Primitive {
public:
  virtual ~Primitive();
  virtual void walk_gl(bool picking) const = 0;
};

class Sphere : public Primitive {
public:
  virtual ~Sphere();
  virtual void walk_gl(bool picking) const;

  static void init();

private:
  static void drawSphere(int lats, int longs);
  static GLuint sphereList;
};

#endif
