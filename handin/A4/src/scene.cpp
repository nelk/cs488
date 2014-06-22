#include "scene.hpp"
#include <iostream>
#include "matrices.hpp"

SceneNode::SceneNode(const std::string& name)
  : m_name(name) {
}

SceneNode::~SceneNode() {
}

void SceneNode::rotate(char axis, double angleDegrees) {
  double angle = angleDegrees * M_PI / 180.0;
  m_trans = m_trans * rotation(axis, angle);
  m_invtrans = m_trans.invert();
}

void SceneNode::scale(const Vector3D& amount) {
  m_trans = m_trans * scaling(amount);
  m_invtrans = m_trans.invert();
}

void SceneNode::translate(const Vector3D& amount) {
  m_trans = m_trans * translation(amount);
  m_invtrans = m_trans.invert();
}

bool SceneNode::is_joint() const {
  return false;
}

std::vector<Intersection> SceneNode::findIntersections(const Ray& ray) {
  std::vector<Intersection> intersections;
  for(std::list<SceneNode*>::const_iterator it = m_children.begin(); it != m_children.end(); it++) {
    std::vector<Intersection> childIntersections = (*it)->findIntersections(ray);
    intersections.insert(intersections.end(), childIntersections.begin(), childIntersections.end());
  }
  return intersections;
}

JointNode::JointNode(const std::string& name)
  : SceneNode(name) {
}

JointNode::~JointNode() {
}

bool JointNode::is_joint() const {
  return true;
}

void JointNode::set_joint_x(double min, double init, double max) {
  m_joint_x.min = min;
  m_joint_x.init = init;
  m_joint_x.max = max;
}

void JointNode::set_joint_y(double min, double init, double max) {
  m_joint_y.min = min;
  m_joint_y.init = init;
  m_joint_y.max = max;
}

GeometryNode::GeometryNode(const std::string& name, Primitive* primitive)
  : SceneNode(name), m_primitive(primitive) {
}

GeometryNode::~GeometryNode() {
}

std::vector<Intersection> GeometryNode::findIntersections(const Ray& ray) {
  std::vector<Intersection> intersections = m_primitive->findIntersections(ray);
  for (std::vector<Intersection>::iterator it = intersections.begin(); it != intersections.end(); it++) {
    it->material = m_material;
  }
  return intersections;
}

