#include "swept_surface.h"

Trackball *trackball;
Camera *camera;
Data *data;
Mesh *mesh;

int width, height;
bool isLeftButtonPressed;

vector<ControlPoint> bSpline(vector<ControlPoint> controlPoints) {
  vector<ControlPoint> results;
  
  int numOfControlPoints = controlPoints.size();
  for (int i = 0; i < numOfControlPoints; i++) {
    for (int j = 0; j < STEPS; j++) {
      float t = (float) j / STEPS;
      float U[4] = { t * t * t, t * t, t, 1.0f };
      float G_x[4] = {
        controlPoints.at(circularIndex(numOfControlPoints, i - 3)).getX(),
        controlPoints.at(circularIndex(numOfControlPoints, i - 2)).getX(),
        controlPoints.at(circularIndex(numOfControlPoints, i - 1)).getX(),
        controlPoints.at(circularIndex(numOfControlPoints, i)).getX()
      };
      float G_z[4] = {
        controlPoints.at(circularIndex(numOfControlPoints, i - 3)).getZ(),
        controlPoints.at(circularIndex(numOfControlPoints, i - 2)).getZ(),
        controlPoints.at(circularIndex(numOfControlPoints, i - 1)).getZ(),
        controlPoints.at(circularIndex(numOfControlPoints, i)).getZ()
      };
    
      results.push_back(ControlPoint(bSplinePolynomial(U, G_x), bSplinePolynomial(U, G_z)));
    }
  }
  return results;
}

vector<ControlPoint> catmullRomSpline(vector<ControlPoint> controlPoints) {
  vector<ControlPoint> results;

  int numOfControlPoints = controlPoints.size();
  for (int i = 0; i < numOfControlPoints; i++) {
    for (int j = 0; j < STEPS; j++) {
      float t = (float) j / STEPS;
      float U[4] = { t * t * t, t * t, t, 1.0f };
      float G_x[4] = {
        controlPoints.at(circularIndex(numOfControlPoints, i - 1)).getX(),
        controlPoints.at(circularIndex(numOfControlPoints, i)).getX(),
        controlPoints.at(circularIndex(numOfControlPoints, i + 1)).getX(),
        controlPoints.at(circularIndex(numOfControlPoints, i + 2)).getX()
      };
      float G_z[4] = {
        controlPoints.at(circularIndex(numOfControlPoints, i - 1)).getZ(),
        controlPoints.at(circularIndex(numOfControlPoints, i)).getZ(),
        controlPoints.at(circularIndex(numOfControlPoints, i + 1)).getZ(),
        controlPoints.at(circularIndex(numOfControlPoints, i + 2)).getZ()
      };
      
      results.push_back(ControlPoint(catmullRomSplinePolynomial(U, G_x), catmullRomSplinePolynomial(U, G_z)));
    }
  }
  return results;
}

vector<glm::vec3> interpolatePoints(vector<glm::vec3> points) {
  vector<glm::vec3> results;

  for (int i = 0; i < points.size() - 3; i++) {
    for (int j = 0; j < STEPS; j++) {
      float t = (float) j / STEPS;
      float U[4] = { t * t * t, t * t, t, 1.0f };
      float G_x[4] = { points.at(i).x, points.at(i + 1).x, points.at(i + 2).x, points.at(i + 3).x };
      float G_y[4] = { points.at(i).y, points.at(i + 1).y, points.at(i + 2).y, points.at(i + 3).y };
      float G_z[4] = { points.at(i).z, points.at(i + 1).z, points.at(i + 2).z, points.at(i + 3).z };

      results.push_back(glm::vec3(
            catmullRomSplinePolynomial(U, G_x),
            catmullRomSplinePolynomial(U, G_y),
            catmullRomSplinePolynomial(U, G_z)));
    }
  }
  return results;
}

vector<float> interpolateScalars(vector<float> scalars) {
  vector<float> results;

  for (int i = 0; i < scalars.size() - 3; i++) {
    for (int j = 0; j < STEPS; j++) {
      float t = (float) j / STEPS;
      float U[4] = { t * t * t, t * t, t, 1.0f };
      float G[4] = { scalars.at(i), scalars.at(i + 1), scalars.at(i + 2), scalars.at(i + 3) };

      results.push_back(catmullRomSplinePolynomial(U, G));
    }
  }
  return results;
}

void generatePolygonalMesh() {
  vector<CrossSection> crossSections = data->getCrossSections();

  for (int i = 0; i < crossSections.size(); i++) {
    CrossSection section = crossSections.at(i);
    if (data->getSplineType().compare("BSPLINE") == 0)
      crossSections.at(i).setSplinePoints(bSpline(section.getControlPoints()));
    else if (data->getSplineType().compare("CATMULL_ROM") == 0)
      crossSections.at(i).setSplinePoints(catmullRomSpline(section.getControlPoints()));
  }

  vector< vector<glm::vec3> > lines;
  for (int n = 0; n < STEPS * data->getNumberOfControlPoints(); n++) {
    vector<glm::vec3> line;
    for (int i = 0; i < crossSections.size(); i++) {
      ControlPoint point = crossSections.at(i).getSplinePoints().at(n);
      line.push_back(glm::vec3(point.getX(), 0, point.getZ()));
    }
    lines.push_back(interpolatePoints(line));
  }

  vector<float> scales;
  vector<float> angles;
  vector<glm::vec3> axes;
  vector<glm::vec3> translations;

  for (int i = 0; i < crossSections.size(); i++) {
    CrossSection section = crossSections.at(i);
    vector<float> r = section.getRotationFactor();
    vector<float> t = section.getTranslationFactor();
    
    scales.push_back(section.getScalingFactor());
    angles.push_back(r.at(0));
    axes.push_back(glm::vec3(r.at(1), r.at(2), r.at(3)));
    translations.push_back(glm::vec3(t.at(0), t.at(1), t.at(2)));
  }
  
  scales = interpolateScalars(scales);
  angles = interpolateScalars(angles);
  axes = interpolatePoints(axes);
  translations = interpolatePoints(translations);
  
  for (int i = 0; i < lines.size(); i++) {
    vector<glm::vec3> line = lines.at(i);
    for (int j = 0; j < line.size(); j++) {
      glm::vec3 point = line.at(j);
      glm::mat4 S = glm::scale(glm::vec3(scales.at(j), scales.at(j), scales.at(j)));
      glm::mat4 R = glm::rotate(angles.at(j), axes.at(j));
      glm::mat4 T = glm::translate(glm::vec3(translations.at(j).x, translations.at(j).y, translations.at(j).z));
      mesh->addVertex(glm::vec3(T * R * S * glm::vec4(point.x, point.y, point.z, 1.0f)));
    } 
  }

  int x = STEPS * data->getNumberOfControlPoints();
  int y = STEPS * (data->getNumberOfCrossSections() - 3);
  
  for (int i = 1; i < x; i++) {
    for (int j = 1; j < y; j++) {
      int indices[4] = {
        (i - 1) * y + (j - 1),
        (i - 1) * y + j,
        i * y + (j - 1),
        i * y + j
      };
      vector<int> i1, i2;
      i1.push_back(indices[0]);
      i1.push_back(indices[1]);
      i1.push_back(indices[2]);
      i2.push_back(indices[3]);
      i2.push_back(indices[2]);
      i2.push_back(indices[1]);
      mesh->addFace(Face(i1));
      mesh->addFace(Face(i2));
    }
  }
}

void glutDisplay() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMultMatrixf(glm::value_ptr(camera->calculateProjectionMatrix()));
         
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMultMatrixf(glm::value_ptr(camera->calculateViewMatrix()));

  glShadeModel(GL_SMOOTH);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
                                             
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_AMBIENT, AMBIENT_LIGHT);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, DIFFUSE_LIGHT);
  glLightfv(GL_LIGHT0, GL_SPECULAR, SPECULAR_LIGHT);
  glLightfv(GL_LIGHT0, GL_POSITION, LIGHT_POSITION);
                                                                     
  glEnable(GL_LIGHT1);
  glLightfv(GL_LIGHT1, GL_AMBIENT, AMBIENT_SUB_LIGHT);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, DIFFUSE_SUB_LIGHT);
  glLightfv(GL_LIGHT1, GL_SPECULAR, SPECULAR_LIGHT);
  glLightfv(GL_LIGHT1, GL_POSITION, SUB_LIGHT_POSITION);

  glEnable(GL_COLOR_MATERIAL); 
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, MATERIAL_AMBIENT); 
  glMaterialfv(GL_FRONT, GL_SPECULAR, MATERIAL_SPECULAR);
  glMateriali(GL_FRONT, GL_SHININESS, 48);

  glColor3f(1.0f, 0.0f, 0.0f); // Red color
  
  vector<Face> faces = mesh->getFaces();
  vector<glm::vec3> vertices = mesh->getVertices();
  
  glBegin(GL_TRIANGLES);
  for (int i = 0; i < faces.size(); i++) {
    vector<int> indices = faces.at(i).getIndices();
    glm::vec3 v1 = vertices.at(indices.at(0));
    glm::vec3 v2 = vertices.at(indices.at(1));
    glm::vec3 v3 = vertices.at(indices.at(2));
    glm::vec3 normal = glm::cross(v2 - v1, v3 - v1);
    glNormal3f(normal.x, normal.y, normal.z);
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
  }
  glEnd();
  
  glutSwapBuffers();
}

void glutReshape(int w, int h) {
  if (w == 0 || h == 0) return;

  width = w;
  height = h;

  trackball->resize(w, h);
  camera->changeAspect((float) w / (float) h);
  glViewport(0, 0, w, h);
}

void glutKeyboard(unsigned char key, int x, int y) {
  switch (key) {
    case 27:
      delete mesh;
      delete data;
      delete trackball;
      delete camera;
      exit(0);
      break;
    case 'q':
      camera->dollyIn();
      break;
    case 'a':
      camera->dollyOut();
      break;
    case 'w':
      camera->zoomIn();
      break;
    case 's':
      camera->zoomOut();
      break;
    case 'e':
      camera->showAll();
      break;
    default:
      break;
  }
}

void glutSpecial(int key, int x, int y) {
  switch (key) {
    case GLUT_KEY_UP:
      camera->pan(DIRECTION_UP);
      break;
    case GLUT_KEY_DOWN:
      camera->pan(DIRECTION_DOWN);
      break;
    case GLUT_KEY_LEFT:
      camera->pan(DIRECTION_LEFT);
      break;
    case GLUT_KEY_RIGHT:
      camera->pan(DIRECTION_RIGHT);
      break;
  }
}

void glutMouse(int button, int state, int x, int y) {
  int modifiers;
  
  if (button == GLUT_LEFT_BUTTON) {
    switch (state) {
      case GLUT_DOWN:
        modifiers = glutGetModifiers();
        if (modifiers && GLUT_ACTIVE_SHIFT) {
          camera->seek(x, y);
        }
        else {
          if (!isLeftButtonPressed) {
            isLeftButtonPressed = true;
            trackball->start(x, y);
          }
        }
        break;
      case GLUT_UP:
        isLeftButtonPressed = false;
        break;
    }
  }
}

void glutMotion(int x, int y) {
  if (isLeftButtonPressed) {
    trackball->stop(x, y);
  }
}

void glutTimer(int unused) {
  glutPostRedisplay();
  glutTimerFunc(30.0f, glutTimer, 0);
}
      
int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  
  glutInitWindowSize(550, 550);
  glutInitWindowPosition(0, 0);
  glutCreateWindow("Swept Surface 3D Viewer");

  data = new Data();
  data->parse(argv[1]);
  
  mesh = new Mesh();
  generatePolygonalMesh();

  trackball = new Trackball(550, 550);
  camera = new Camera(550, 550);
  trackball->setCamera(camera);

  glutReshapeFunc(glutReshape);
  glutDisplayFunc(glutDisplay);
  glutTimerFunc(30.0f, glutTimer, 0);
  glutKeyboardFunc(glutKeyboard);
  glutSpecialFunc(glutSpecial);
  glutMouseFunc(glutMouse);
  glutMotionFunc(glutMotion);

  glutMainLoop();

  return 0;
}
