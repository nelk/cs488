
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "controller.hpp"
#include "sound.hpp"

Controller::Controller(Viewer* viewer, Settings* settings)
  : viewer(viewer), settings(settings), lastTime(0), position(0, 0, 0), velocity(0, 0, 0), horizontalAngle(0), verticalAngle(0), skipMovements(2), flashlight(false), hasFlashlight(false), selecting(false), shooting(false), jumping(false) {

  flashlightSound = Sound::load("sound/click.wav");
  gunSound = Sound::load("sound/gun.wav");
  stepSound = Sound::load("sound/step.wav");
  stepSound->setGain(0.4);
}

Controller::~Controller() {
  delete flashlightSound;
  delete stepSound;
}

void Controller::reset() {
  lastTime = glfwGetTime();
  glfwSetCursorPos(viewer->getWindow(), viewer->getWidth()/2, viewer->getHeight()/2);
  skipMovements = 2;
}

void Controller::setPosition(glm::vec3& p) {
  position = p;
}

glm::vec3 Controller::getPosition() {
  return position;
}

void Controller::setHorizontalAngle(float a) {
  horizontalAngle = a;
}
void Controller::setVerticalAngle(float a) {
  verticalAngle = a;
}

glm::mat4 Controller::getViewMatrix(){
  return viewMatrix;
}
glm::mat4 Controller::getProjectionMatrix(){
  return projectionMatrix;
}

glm::mat4 Controller::getMirroredViewMatrix(const glm::vec3& mirrorPosition, const glm::vec3& mirrorNormal) {

  glm::vec3 p_mirrored = position - 2 * glm::dot(mirrorNormal, position - mirrorPosition) * mirrorNormal;
  glm::vec3 p2 = position + direction;
  glm::vec3 p2_mirrored = p2 - 2 * glm::dot(mirrorNormal, p2 - mirrorPosition) * mirrorNormal;

  return glm::lookAt(
    p_mirrored,   // Camera position.
    p2_mirrored,  // Lookat position.
    glm::vec3(0, 1, 0)   // Up.
  );
}

bool Controller::checkKeyJustPressed(int k) {
  bool wasPressed;
  if (keysPressed.find(k) == keysPressed.end()) {
    wasPressed = false;
  } else {
    wasPressed = keysPressed[k];
  }

  bool isPressed = glfwGetKey(viewer->getWindow(), k) == GLFW_PRESS;

  keysPressed[k] = isPressed;
  return !wasPressed && isPressed;
}

bool Controller::checkMouseJustPressed(int i) {
  bool wasPressed;
  if (mousePressed.find(i) == mousePressed.end()) {
    wasPressed = false;
  } else {
    wasPressed = mousePressed[i];
  }

  bool isPressed = glfwGetMouseButton(viewer->getWindow(), i) == GLFW_PRESS;

  mousePressed[i] = isPressed;
  return !wasPressed && isPressed;
}

void Controller::update() {
  double currentTime = glfwGetTime();
  float deltaTime = currentTime - lastTime;

  GLFWwindow* window = viewer->getWindow();
  int width = viewer->getWidth();
  int height = viewer->getHeight();

  // Get mouse position.
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  // Reset mouse position for next frame.
  glfwSetCursorPos(window, width/2, height/2);

  if (skipMovements > 0) {
    skipMovements--;
    return;
  }

  // Compute new orientation
  float delta_x = width/2 - xpos;
  float delta_y = height/2 - ypos;
  horizontalAngle += MOUSE_SPEED * delta_x;
  verticalAngle   += MOUSE_SPEED * delta_y;

  const float epsilon = 0.01; // To prevent direction flipping at extremes.
  if (verticalAngle > M_PI/2 - epsilon) {
    verticalAngle = M_PI/2 - epsilon;
  } else if (verticalAngle < -M_PI/2 + epsilon) {
    verticalAngle = -M_PI/2 + epsilon;
  }

  // Compute 3 direction vectors from spherical coordinates.
  direction = glm::vec3(
    cos(verticalAngle) * sin(horizontalAngle),
    sin(verticalAngle),
    cos(verticalAngle) * cos(horizontalAngle)
  );

  // Direction but without vertical.
  glm::vec3 flatDirection = glm::normalize(glm::vec3(1, 0, 1) * direction);

  glm::vec3 right = glm::vec3(
    sin(horizontalAngle - M_PI/2.0),
    0,
    cos(horizontalAngle - M_PI/2.0)
  );

  glm::vec3 up = glm::cross(right, direction);

  // TODO: Jumping.
  jumping = checkKeyJustPressed(GLFW_KEY_SPACE);

  // Keyboard movement.
  bool isWalking = false;
  // Forwards.
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS
      || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
    position += flatDirection * deltaTime * SPEED;
    isWalking = true;
  }
  // Backwards.
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS
      || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
    position -= flatDirection * deltaTime * SPEED;
    isWalking = true;
  }
  // Right.
  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS
      || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
    position += right * deltaTime * SPEED;
    isWalking = true;
  }
  // Left.
  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS
      || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
    position -= right * deltaTime * SPEED;
    isWalking = true;
  }
  // Up.
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    position += glm::vec3(0, 1, 0) * deltaTime * SPEED;
  }
  // Down.
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    position -= glm::vec3(0, 1, 0) * deltaTime * SPEED;
  }

  if (isWalking && currentTime - lastStepSoundTime >= 0.25) {
    lastStepSoundTime = currentTime;
    stepSound->play();
  }

  // Flashlight (F).
  if (hasFlashlight && checkKeyJustPressed(GLFW_KEY_F)) {
    flashlight = !flashlight;
    flashlightSound->play();
  }

  // Settings toggled by key press.
  for (int i = 0; i < 10; i++) {
    if (checkKeyJustPressed(GLFW_KEY_0 + i) || checkKeyJustPressed(GLFW_KEY_KP_0 + i)) {
      if (i < Settings::NUM_SETTINGS) {
        std::cerr << "Toggling " << Settings::settingNames[i] << std::endl;
        settings->toggle((Settings::SettingsEnum) i);
      }
    }
  }

  // Projection matrix: 45 degree Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units.
  projectionMatrix = glm::perspective(45.0f, width/(float)height, 0.1f, 100.0f);
  // Camera matrix
  viewMatrix = glm::lookAt(
    position,             // Camera position.
    position + direction, // Lookat position.
    up                    // Up.
  );


  // Mouse clicking.
  shooting = checkMouseJustPressed(0);
  selecting = checkMouseJustPressed(1);

  // Sound: update listener state.
  Sound::setListenerPosition(position);
  Sound::setListenerVelocity(glm::vec3(0, 0, 0));
  Sound::setListenerOrientation(direction, up);

  // Sound: update attaches source states.
  flashlightSound->setPosition(position + glm::vec3(0, -0.5, 0));
  stepSound->setPosition(position + glm::vec3(0, -1, 0));

  if (isShooting()) {
    gunSound->play();
  }

  if (checkKeyJustPressed(GLFW_KEY_P)) {
    viewer->takeScreenshot();
  }

  lastTime = currentTime;
}

