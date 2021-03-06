#ifndef SCENE_HPP
#define SCENE_HPP

#include <list>
#include <vector>
#include "algebra.hpp"
#include "primitive.hpp"
#include "material.hpp"

enum Axis {
  X = 0,
  Y = 1,
  Z = 2,
  NUM_AXES = 3
};

class SceneNode {
public:
  SceneNode(const std::string& name);
  virtual ~SceneNode();

  virtual void walk_gl(bool picking = false) const;

  const Matrix4x4& get_transform() const { return m_trans; }
  const Matrix4x4& get_inverse() const { return m_invtrans; }

  void set_transform(const Matrix4x4& m) {
    m_trans = m;
    m_invtrans = m.invert();
  }

  void set_transform(const Matrix4x4& m, const Matrix4x4& i) {
    m_trans = m;
    m_invtrans = i;
  }

  // Helper to store current transformation matrix and apply the transformation of this node.
  void push_transform_gl() const {
    glPushMatrix();
    Matrix4x4 columnMajorTrans = m_trans.transpose();
    glMultMatrixd(columnMajorTrans.begin());
  }

  void pop_transform_gl() const {
    glPopMatrix();
  }

  void add_child(SceneNode* child) {
    m_children.push_back(child);
  }

  void remove_child(SceneNode* child) {
    m_children.remove(child);
  }

  // These will be called from Lua.
  // Note that these are post-multiplied.
  void rotate(char axis, double angle);
  void scale(const Vector3D& amount);
  void translate(const Vector3D& amount);

  // Helpers that pre-multiply.
  void preRotate(char axis, double angle);
  void preScale(const Vector3D& amount);
  void preTranslate(const Vector3D& amount);

  // Returns true if and only if this node is a JointNode
  virtual bool is_joint() const;

  /**
   * Walk scene graph, toggling a node that has the set id.
   * Only nodes with joint node parents will be selected.
   * Returns true if the node was found and searching should cease.
   */
  virtual bool togglePick(int id, bool parent_is_joint=false);

  // Joint functions that will traverse over the tree to get all joints.
  virtual void moveJoints(double primaryDelta, double secondaryDelta);
  virtual void resetJoints();
  virtual void saveJointUndoState();
  virtual void undoJoints();
  virtual void redoJoints();

  bool isPicked() {
    return picked;
  }

protected:
  static int nextId;

  // Useful for picking.
  int m_id;
  std::string m_name;

  bool picked;

  // Transformations
  Matrix4x4 m_trans;
  Matrix4x4 m_invtrans;

  // Hierarchy
  typedef std::list<SceneNode*> ChildList;
  ChildList m_children;
};

class JointNode : public SceneNode {
public:
  JointNode(const std::string& name);
  virtual ~JointNode();

  virtual void walk_gl(bool picking = false) const;

  virtual bool is_joint() const;

  void set_joint_x(double min, double init, double max) {
    setJointRange(X, min, init, max);
  }
  void set_joint_y(double min, double init, double max) {
    setJointRange(Y, min, init, max);
  }

  // Set range and set current rotation to init.
  void setJointRange(Axis axis, double min, double init, double max);
  void rotateJoint(Axis axis, double delta);

  virtual void moveJoints(double primaryDelta, double secondaryDelta);
  virtual void resetJoints();
  virtual void saveJointUndoState();
  virtual void undoJoints();
  virtual void redoJoints();

  struct JointRange {
    double min, init, max;
  };

protected:

  JointRange jointRanges[NUM_AXES]; // In degrees.
  std::vector<double> jointRotation;

  // Each node will maintain its own undo and redo stacks.
  std::vector<std::vector<double> > undoStack;
  std::vector<std::vector<double> > redoStack;
};

class GeometryNode : public SceneNode {
public:
  GeometryNode(const std::string& name,
               Primitive* primitive);
  virtual ~GeometryNode();

  virtual void walk_gl(bool picking = false) const;

  const Material* get_material() const {
    return m_material;
  }
  Material* get_material() {
    return m_material;
  }

  void set_material(Material* material)
  {
    m_material = material;
  }

  static void togglePickHighlight() {
    pickHighlight = !pickHighlight;
  }

protected:
  static bool pickHighlight; // Whether selected nodes should be displayed highlighted.
  static PhongMaterial highlightMaterial;
  static PhongMaterial defaultMaterial;

  Material* m_material;
  Primitive* m_primitive;
};

#endif
