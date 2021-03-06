#include "mesh.hpp"
#include <iostream>
#include <algorithm>
#include <list>

//#define MESH_TREE_OPTIMIZATION
#define MESH_TREE_MAX_DEPTH 1
#define MESH_MAX_FACES 20


Mesh::Mesh(const std::vector<Point3D>& verts,
           const std::vector< std::vector<int> >& faces,
           int depth)
  : m_verts(verts), m_faces(faces), m_bound(NULL), m_descendents(NULL) {

  std::cout << "Constructing Mesh with " << m_verts.size() << " verts and " << m_faces.size() << " faces." << std::endl;

  if (m_verts.empty() || m_faces.size() < 6) {
    return;
  }

  bool clearData = false;

#ifdef MESH_TREE_OPTIMIZATION
  // Divide space.
  if (depth < MESH_TREE_MAX_DEPTH && m_faces.size() > MESH_MAX_FACES) {
    std::list<std::vector<Face> > faceChunks;
    faceChunks.push_back(m_faces);

    for (int axis = X; axis <= Z; axis++) {
      const int size = faceChunks.size();
      for (int chunk = 0; chunk < size; chunk++) {
        FaceComparator faceCmp((Axis) axis, this);
        std::vector<Face> faces = faceChunks.front();
        faceChunks.pop_front();
        std::sort(faces.begin(), faces.end(), faceCmp);
        int half = faces.size() / 2;
        faceChunks.push_back(std::vector<Face>(faces.begin(), faces.begin() + half));
        faceChunks.push_back(std::vector<Face>(faces.begin() + half, faces.end()));
      }
    }

    std::cout << "Dividing into " << faceChunks.size() << " descendents" << std::endl;
    m_descendents = new SceneNode("mesh_internal");
    for (std::list<std::vector<Face> >::iterator faceIt = faceChunks.begin(); faceIt != faceChunks.end(); faceIt++) {
      m_descendents->add_child(new GeometryNode("mesh_internal_geom", new Mesh(m_verts, *faceIt, depth+1)));
    }
    clearData = true;
  }
#endif

  Point3D min = m_verts[0];
  Point3D max = m_verts[0];
  for (std::vector<Point3D>::const_iterator it = m_verts.begin(); it != m_verts.end(); it++) {
    min[X] = std::min(min[X], (*it)[X]);
    max[X] = std::max(max[X], (*it)[X]);
    min[Y] = std::min(min[Y], (*it)[Y]);
    max[Y] = std::max(max[Y], (*it)[Y]);
    min[Z] = std::min(min[Z], (*it)[Z]);
    max[Z] = std::max(max[Z], (*it)[Z]);
  }
  m_bound = new GeometryNode("some_bounding_box", new Cube());
  Vector3D size = max - min;
  m_bound->translate(min - Point3D());
  size[X] = std::max(0.001, size[X]);
  size[Y] = std::max(0.001, size[Y]);
  size[Z] = std::max(0.001, size[Z]);
  m_bound->scale(size);

  if (clearData) {
    m_verts.clear();
    m_faces.clear();
  }
}

Mesh::~Mesh() {
  if (m_bound != NULL) {
    delete m_bound;
    m_bound = NULL;
  }
}

std::ostream& operator<<(std::ostream& out, const Mesh& mesh) {
  std::cerr << "mesh({";
  for (std::vector<Point3D>::const_iterator I = mesh.m_verts.begin(); I != mesh.m_verts.end(); ++I) {
    if (I != mesh.m_verts.begin()) std::cerr << ",\n      ";
    std::cerr << *I;
  }
  std::cerr << "},\n\n     {";

  for (std::vector<Mesh::Face>::const_iterator I = mesh.m_faces.begin(); I != mesh.m_faces.end(); ++I) {
    if (I != mesh.m_faces.begin()) std::cerr << ",\n      ";
    std::cerr << "[";
    for (Mesh::Face::const_iterator J = I->begin(); J != I->end(); ++J) {
      if (J != I->begin()) std::cerr << ", ";
      std::cerr << *J;
    }
    std::cerr << "]";
  }
  std::cerr << "});" << std::endl;
  return out;
}


RayResult* Mesh::findIntersections(const Ray& ray) {
  RayResult* result = new RayResult(std::vector<Intersection>(), m_faces.size());
  if (m_bound != NULL) {
    RayResult* boundResult = m_bound->findIntersections(ray);

    result->stats.bounding_box_checks++;
    if (boundResult->isHit()) {
      result->stats.bounding_box_hits++;
    }

    if (!boundResult->isHit() || DRAW_BOUNDING_BOXES) {
      result->merge(*boundResult);
      delete boundResult;
      return result;
    }

    result->stats.merge(boundResult->stats);
    delete boundResult;
  }
  if (m_descendents != NULL) {
    RayResult* descendentResult = m_descendents->findIntersections(ray);
    result->merge(*descendentResult);
    delete descendentResult;
    return result;
  }

  // Reinier van Vliet and Remco Lam angle sums algorithm.
  const double EPSILON = 0.0000001;
  for (std::vector<Face>::const_iterator it = m_faces.begin(); it != m_faces.end(); it++) {
    const Face& face = *it;
    if (face.size() < 3) {
      std::cout << "Warn: Face with less than 3 verts found." << std::endl;
      continue;
    }

    Vector3D v1 = m_verts[face[1]] - m_verts[face[0]];
    Vector3D v2 = m_verts[face[1]] - m_verts[face[2]];
    //Vector3D normal = v1.cross(v2);
    Vector3D normal = v2.cross(v1);
    normal.normalize();

    double t = -(ray.pos - m_verts[face[0]]).dot(normal) / ray.dir.dot(normal);
    if (t < EPSILON) {
      continue; // Pointing away from face.
    }
    Point3D q = ray.pos + t*ray.dir;

    double anglesum = 0.0;
    for (unsigned int i = 0; i < face.size(); i++) {
      Vector3D p1 = m_verts[face[i]] - q;
      Vector3D p2 = m_verts[face[(i+1)%face.size()]] - q;

      double m1 = p1.length();
      double m2 = p2.length();
      if (m1*m2 <= EPSILON) {
        anglesum = M_PI*2; // We are on a node, consider this inside.
      } else {
        anglesum += acos(p1.dot(p2) / (m1*m2));
      }
      if (anglesum >= M_PI*2 - EPSILON && anglesum <= M_PI*2 + EPSILON) {
        result->intersections.push_back(Intersection(q, normal, NULL));
        //std::cout << "INTERSECTION " << t << std::endl;
      }
    }
    //std::cout << "Exitted with anglesum=" << anglesum << std::endl;
  }

  return result;
}

