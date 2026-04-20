#include "camera.h"
#include "cone.h"
#include "cylinder.h"
#include "pointLight.h"
#include "shader.h"
#include "sphere.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb_image for texture loading

#include "stb_image.h"

#include <functional>
#include <iostream>
using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  Forward declarations
// ─────────────────────────────────────────────────────────────────────────────
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void drawCube(unsigned int &VAO, Shader &shader, glm::mat4 model,
              glm::vec3 color);

// ─────────────────────────────────────────────────────────────────────────────
//  Texture loader helper
// ─────────────────────────────────────────────────────────────────────────────
static unsigned int loadTexture(const char *path) {
  unsigned int texID;
  glGenTextures(1, &texID);

  int w, h, nChannels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path, &w, &h, &nChannels, 0);
  if (data) {
    GLenum fmt = (nChannels == 4) ? GL_RGBA : GL_RGB;
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    cout << "  Texture loaded OK: " << path << "\n";
  } else {
    // Use a 1×1 white fallback so missing images do not crash
    unsigned char white[3] = {255, 255, 255};
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 white);
    cout << "  WARNING: could not load texture: " << path
         << " – using white fallback\n";
    stbi_image_free(data);
  }
  return texID;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────────────────────────────────────────
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// ─────────────────────────────────────────────────────────────────────────────
//  Global state
// ─────────────────────────────────────────────────────────────────────────────
Camera camera(glm::vec3(-15.0f, 5.0f, 15.0f));
float deltaTime = 0.0f, lastFrame = 0.0f;

// Lighting toggles
bool directionLightOn = true;
bool spotLightOn = true;
bool point1On = true;
bool point2On = true;
bool ambientOn = true;
bool diffuseOn = true;
bool specularOn = true;

// Interactions
bool fanOn = false;
float fanRot = 0.0f;
bool openDoor = true;
float doorAngle = 90.0f;

// Lift state
bool liftUp = false;
float liftY = 0.0f;
float liftDoorAngle = 0.0f;
bool liftMoving = false;

// ── B2: texture mode
int textureMode = 1;
bool splitViewport = false;

// ── Bird animation state
float birdTime = 0.f;

// ── Interactive car state
float carX = -25.f;
float carZ = -6.f;
float carAngle = 90.f;
float carSpeed = 0.f;
bool carAuto = true;
bool openFridge = false;
float fridgeDoorAngle = 0.f;

// Interactive tap (top floor bathroom)
bool tapOn = false;

// Interactive AC (ground floor bedroom)
bool acOn = false;
float acPanelAngle = 0.f;
float acLouverPhase = 0.f;

// Interactive table lamp (top floor nightstand)
bool tableLampOn = false;

// Animated clouds
float cloudDriftTime = 0.f;

// ─────────────────────────────────────────────────────────────────────────────
//  Custom projection (no glm::perspective)
// ─────────────────────────────────────────────────────────────────────────────
static glm::mat4 myProjection(float fovRad, float aspect, float n, float f) {
  float t = tan(fovRad / 2.0f);
  glm::mat4 m(0.0f);
  m[0][0] = 1.0f / (aspect * t);
  m[1][1] = 1.0f / t;
  m[2][2] = -(f + n) / (f - n);
  m[2][3] = -1.0f;
  m[3][2] = -(2.0f * f * n) / (f - n);
  return m;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Point lights (same positions as original)
// ─────────────────────────────────────────────────────────────────────────────
glm::vec3 ptPos[] = {
    glm::vec3(-4.1f, 3.5f, -12.0f),
    glm::vec3(-4.1f, 3.5f, 9.7f),
    glm::vec3(-1.0f, 8.5f, -11.0f),
    glm::vec3(-1.0f, 8.5f, 0.0f),
};

PointLight pointlight1(ptPos[0].x, ptPos[0].y, ptPos[0].z, 0.30f, 0.10f, 0.10f,
                       1.0f, 0.5f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f, 0.09f, 0.032f,
                       1);
PointLight pointlight2(ptPos[1].x, ptPos[1].y, ptPos[1].z, 0.10f, 0.10f, 0.30f,
                       0.5f, 0.5f, 1.0f, 0.8f, 0.8f, 1.0f, 1.0f, 0.09f, 0.032f,
                       2);

// ═══════════════════════════════════════════════════════════════════════════════
//  BEZIER SURFACE OF REVOLUTION – decorative pillars, domes, arches, vases
// ═══════════════════════════════════════════════════════════════════════════════
struct BezierRevolution {
  unsigned int vao = 0;
  int idxCount = 0;
  static long long nCr(int n, int r) {
    if (r > n / 2)
      r = n - r;
    long long a = 1;
    for (int i = 1; i <= r; i++) {
      a *= n - r + i;
      a /= i;
    }
    return a;
  }
  static glm::vec2 evalBez(float t, float *cp, int deg) {
    float x = 0, y = 0;
    if (t > 1.f)
      t = 1.f;
    for (int i = 0; i <= deg; i++) {
      double c = (double)nCr(deg, i) * pow(1.0 - (double)t, deg - i) *
                 pow((double)t, i);
      x += (float)(c * cp[i * 2]);
      y += (float)(c * cp[i * 2 + 1]);
    }
    return glm::vec2(x, y);
  }
  void setup(float *cp, int deg, int uN = 30, int vN = 24) {
    vector<float> vts;
    vector<unsigned int> idx;
    const float PI2 = 6.28318f;
    for (int i = 0; i <= uN; i++) {
      glm::vec2 p = evalBez((float)i / uN, cp, deg);
      float r = p.x, y = p.y, inv = (r > 1e-6f) ? 1.f / r : 0.f;
      for (int j = 0; j <= vN; j++) {
        float th = j * PI2 / vN;
        float px = r * cosf(th), pz = -r * sinf(th);
        vts.push_back(px);
        vts.push_back(y);
        vts.push_back(pz);
        vts.push_back(px * inv);
        vts.push_back(0.f);
        vts.push_back(pz * inv);
      }
    }
    for (int i = 0; i < uN; i++) {
      int k1 = i * (vN + 1), k2 = k1 + vN + 1;
      for (int j = 0; j < vN; j++, k1++, k2++) {
        idx.push_back(k1);
        idx.push_back(k2);
        idx.push_back(k1 + 1);
        idx.push_back(k1 + 1);
        idx.push_back(k2);
        idx.push_back(k2 + 1);
      }
    }
    unsigned int vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vts.size() * sizeof(float), vts.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int),
                 idx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    idxCount = (int)idx.size();
  }
  void draw(Shader &sh, glm::mat4 model, glm::vec3 col) const {
    if (!vao)
      return;
    sh.use();
    sh.setVec3("material.ambient", col * 0.4f);
    sh.setVec3("material.diffuse", col);
    sh.setVec3("material.specular", glm::vec3(0.3f));
    sh.setFloat("material.shininess", 32.f);
    sh.setMat4("model", model);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, idxCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }
};

// ═══════════════════════════════════════════════════════════════════════════════
//  FRACTAL TREE – recursive branching tree using cylinder + cone
// ═══════════════════════════════════════════════════════════════════════════════
static void drawFractalBranch(Shader &sh, const Cylinder &bark, const Cone &leaf,
                       glm::mat4 xf, float len, float rad, int depth) {
  if (depth < 0)
    return;
  // Draw branch segment (cylinder)
  glm::mat4 m = glm::scale(xf, glm::vec3(rad * 2.f, len, rad * 2.f));
  bark.drawSphere(sh, m);
  if (depth == 0) {
    // Leaf tuft (cone at tip)
    glm::mat4 lm = glm::translate(xf, glm::vec3(0.f, len, 0.f));
    lm = glm::scale(lm, glm::vec3(rad * 4.5f, rad * 8.f, rad * 4.5f));
    leaf.drawCone(sh, lm);
    return;
  }
  glm::mat4 tip = glm::translate(xf, glm::vec3(0.f, len, 0.f));
  float cL = len * 0.68f, cR = rad * 0.62f;
  // 5 forks spread in all directions (full 3D)
  float forks[][3] = {
      {0.f, 25.f, 0.f},     // forward tilt
      {72.f, 30.f, 10.f},   // right-front
      {144.f, 28.f, -8.f},  // right-back
      {-72.f, 30.f, -10.f}, // left-front
      {-144.f, 28.f, 8.f}   // left-back
  };
  for (int i = 0; i < 5; i++) {
    glm::mat4 t =
        glm::rotate(tip, glm::radians(forks[i][0]), glm::vec3(0, 1, 0));
    t = glm::rotate(t, glm::radians(forks[i][1]), glm::vec3(1, 0, 0));
    t = glm::rotate(t, glm::radians(forks[i][2]), glm::vec3(0, 0, 1));
    drawFractalBranch(sh, bark, leaf, t, cL, cR, depth - 1);
  }
}

static void drawFractalTree(Shader &sh, const Cylinder &bark, const Cone &leaf,
                     float wx, float wy, float wz, float len, float rad,
                     int depth) {
  drawFractalBranch(sh, bark, leaf,
                    glm::translate(glm::mat4(1.f), glm::vec3(wx, wy, wz)), len,
                    rad, depth);
}

// ─────────────────────────────────────────────────────────────────────────────
//  main()
// ─────────────────────────────────────────────────────────────────────────────
int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                       "3D Apartment – Assignment B2 (Textures)", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetScrollCallback(window, scroll_callback);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return -1;
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // ── Print controls ────────────────────────────────────────────────────────
  cout << "\n=============================================\n";
  cout << "  3D Apartment - Assignment B2 (Textures)\n";
  cout << "=============================================\n";
  cout << "  CAMERA:\n";
  cout << "    W/S/A/D/Q/E         Move camera\n";
  cout << "    I/K                 Pitch up/down\n";
  cout << "    J/L                 Yaw left/right\n";
  cout << "    U/O                 Roll left/right\n";
  cout << "    LEFT SHIFT + above  4x FAST movement\n";
  cout << "  LIGHTING:\n";
  cout << "    1             Toggle Directional Light\n";
  cout << "    2             Toggle Point Lights (all)\n";
  cout << "    3             Toggle Spot Light\n";
  cout << "    5             Toggle Ambient component\n";
  cout << "    6             Toggle Diffuse component\n";
  cout << "    7             Toggle Specular component\n";
  cout << "  TEXTURE MODES (B2 Assignment):\n";
  cout << "    T             Cycle texture mode:\n";
  cout << "                  0=No Texture | 1=Simple Texture\n";
  cout << "                  2=Vertex-Blended | 3=Fragment-Blended\n";
  cout << "  INTERACTIONS:\n";
  cout << "    G             Toggle Ceiling Fan\n";
  cout << "    M / N         Open / Close Doors\n";
  cout << "    X / P         Open / Close Fridge Door\n";
  cout << "    4             Toggle Bathroom Tap Water\n";
  cout << "    8             Toggle AC (Ground Floor)\n";
  cout << "    0             Toggle Table Lamp (Top Floor)\n";
  cout << "  VIEWPORT:\n";
  cout << "    9             Toggle 4-Split Viewport\n";
  cout << "  OTHER:\n";
  cout << "    ESC           Quit\n";
  cout << "=============================================\n\n";
  cout << "Texture mode: 0 = No Texture (press T to cycle)\n\n";

  // ── Shaders ───────────────────────────────────────────────────────────────
  // Original Phong shader (no texture) – used for non-textured objects
  Shader lightingShader("vertexShaderForPhongShading.vs",
                        "fragmentShaderForPhongShading.fs");
  // New texture-capable shader
  Shader texShader("vertexShaderForPhongShadingWithTexture.vs",
                   "fragmentShaderForPhongShadingWithTexture.fs");
  // Light-cube (emissive bulb markers)
  Shader ourShader("vertexShader.vs", "fragmentShader.fs");

  // ── Textures ──────────────────────────────────────────────────────────────
  cout << "Loading textures...\n";
  unsigned int texFloor = loadTexture("floor.jpg");
  unsigned int texWall = loadTexture("wood.jpg"); // Replaced dark wall.jpg with wood.jpg
  unsigned int texMosaic = loadTexture("mosaic.jpg");
  unsigned int texCeiling = loadTexture("ceiling.jpg");
  unsigned int texGrass = loadTexture("grass.jpg");
  unsigned int texDoor = loadTexture("door.jpg");
  unsigned int texRoof = loadTexture("roof.jpg");
  
  // Optional textures (will fall back to white if file absent)
  unsigned int texTV = loadTexture("tv.jpg");
  unsigned int texRoad = loadTexture("road.jpg");
  unsigned int texStairs = loadTexture("stairs.jpg");
  unsigned int texSofa = loadTexture("sofa.jpg");
  // Textures for curvy objects (will fall back to white if missing)
  unsigned int texDecorSphere = loadTexture("decor_sphere.jpg");
  unsigned int texTreeLeaves = loadTexture("tree_leaves.jpg");
  unsigned int texDecorCone = loadTexture("decor_cone.jpg");
  unsigned int texBollard = loadTexture("bollard.jpg");
  // Portrait image textures
  unsigned int texPortrait1 = loadTexture("portrait1.jpg");
  unsigned int texPortrait2 = loadTexture("portrait2.jpg");
  unsigned int texLift = loadTexture("lift.jpg");
  unsigned int texCabinet = loadTexture("cabinet.jpg");
  unsigned int texWood = loadTexture("wood.jpg");
  unsigned int texBooks = loadTexture("books.jpg");
  unsigned int texTV2 = loadTexture("tv2.jpg");
  unsigned int texTV3 = loadTexture("tv3.jpg");
  cout << "Textures loaded.\n\n";

  // --- SKYBOX setup -------------------------------------------------
  // Create skybox shader and load cubemap
  Shader skyboxShader("skybox.vs", "skybox.fs");
  vector<string> faces{
      "skybox/right.png",  "skybox/left.png",  "skybox/top.png",
      "skybox/bottom.png", "skybox/front.png", "skybox/back.png",
  };

  auto loadCubemap = [&](const vector<string> &faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    for (unsigned int i = 0; i < faces.size(); i++) {
      int width, height, nrChannels;
      stbi_set_flip_vertically_on_load(false);
      unsigned char *data =
          stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
      if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width,
                     height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        cout << "  Cubemap face loaded: " << faces[i] << "\n";
      } else {
        // fallback: white pixel
        unsigned char white[3] = {255, 255, 255};
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, white);
        cout << "  WARNING: could not load cubemap face: " << faces[i] << "\n";
      }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
  };

  unsigned int cubemapTexture = loadCubemap(faces);

  float skyboxVertices[] = {
      // positions
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

      1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

      -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

  unsigned int skyboxVAO, skyboxVBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glBindVertexArray(0);

  // ── Cube VAO (no texture coords) – same as original ───────────────────────
  float cube_vertices[] = {
      0, 0, 0,  0,  0,  -1, 1, 0, 0, 0,  0,  -1, 1,  1, 0, 0, 0,  -1, 0,  1,  0,
      0, 0, -1, 1,  0,  0,  1, 0, 0, 1,  1,  0,  1,  0, 0, 1, 0,  1,  1,  0,  0,
      1, 1, 1,  1,  0,  0,  0, 0, 1, 0,  0,  1,  1,  0, 1, 0, 0,  1,  1,  1,  1,
      0, 0, 1,  0,  1,  1,  0, 0, 1, 0,  0,  1,  -1, 0, 0, 0, 1,  1,  -1, 0,  0,
      0, 1, 0,  -1, 0,  0,  0, 0, 0, -1, 0,  0,  1,  1, 1, 0, 1,  0,  1,  1,  0,
      0, 1, 0,  0,  1,  0,  0, 1, 0, 0,  1,  1,  0,  1, 0, 0, 0,  0,  0,  -1, 0,
      1, 0, 0,  0,  -1, 0,  1, 0, 1, 0,  -1, 0,  0,  0, 1, 0, -1, 0};
  unsigned int cube_indices[] = {
      0,  3,  2,  2,  1,  0,  4,  5,  7,  7,  6,  4,  8,  9,  10, 10, 11, 8,
      12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

  unsigned int VBO, VAO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)12);
  glEnableVertexAttribArray(1);
  // (no tex coord attrib for this VAO)

  // Light-cube VAO
  unsigned int lightCubeVAO;
  glGenVertexArrays(1, &lightCubeVAO);
  glBindVertexArray(lightCubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // ── Textured cube VAO (pos + normal + texcoord) ───────────────────────────
  // Uses [0,1] cube with UV coords mapped per face
  float tcube_vertices[] = {
      // pos(3)  normal(3)   uv(2)
      // BACK face  (z=0)
      0,
      0,
      0,
      0,
      0,
      -1,
      1,
      0,
      1,
      0,
      0,
      0,
      0,
      -1,
      0,
      0,
      1,
      1,
      0,
      0,
      0,
      -1,
      0,
      1,
      0,
      1,
      0,
      0,
      0,
      -1,
      1,
      1,
      // RIGHT face (x=1)
      1,
      0,
      0,
      1,
      0,
      0,
      0,
      0,
      1,
      1,
      0,
      1,
      0,
      0,
      0,
      1,
      1,
      0,
      1,
      1,
      0,
      0,
      1,
      0,
      1,
      1,
      1,
      1,
      0,
      0,
      1,
      1,
      // FRONT face (z=1)
      0,
      0,
      1,
      0,
      0,
      1,
      0,
      0,
      1,
      0,
      1,
      0,
      0,
      1,
      1,
      0,
      1,
      1,
      1,
      0,
      0,
      1,
      1,
      1,
      0,
      1,
      1,
      0,
      0,
      1,
      0,
      1,
      // LEFT face  (x=0)
      0,
      0,
      1,
      -1,
      0,
      0,
      1,
      0,
      0,
      1,
      1,
      -1,
      0,
      0,
      1,
      1,
      0,
      1,
      0,
      -1,
      0,
      0,
      0,
      1,
      0,
      0,
      0,
      -1,
      0,
      0,
      0,
      0,
      // TOP face   (y=1)
      1,
      1,
      1,
      0,
      1,
      0,
      1,
      0,
      1,
      1,
      0,
      0,
      1,
      0,
      1,
      1,
      0,
      1,
      0,
      0,
      1,
      0,
      0,
      1,
      0,
      1,
      1,
      0,
      1,
      0,
      0,
      0,
      // BOTTOM face (y=0)
      0,
      0,
      0,
      0,
      -1,
      0,
      0,
      0,
      1,
      0,
      0,
      0,
      -1,
      0,
      1,
      0,
      1,
      0,
      1,
      0,
      -1,
      0,
      1,
      1,
      0,
      0,
      1,
      0,
      -1,
      0,
      0,
      1,
  };

  unsigned int tVBO, tVAO, tEBO;
  glGenVertexArrays(1, &tVAO);
  glGenBuffers(1, &tVBO);
  glGenBuffers(1, &tEBO);
  glBindVertexArray(tVAO);
  glBindBuffer(GL_ARRAY_BUFFER, tVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(tcube_vertices), tcube_vertices,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices,
               GL_STATIC_DRAW);
  // pos
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  // normal
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  // uv
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // ── Curvy objects: Sphere & Cone ──────────────────────────────────────────
  // Sphere: placed as a large decorative globe lamp in the living room
  Sphere decorSphere(0.5f, 36, 18, glm::vec3(0.9f, 0.85f, 0.7f),
                     glm::vec3(0.9f, 0.85f, 0.7f), glm::vec3(0.5f, 0.5f, 0.5f),
                     32.0f);

  // Cone: placed as a conical lampshade / roof ornament
  Cone decorCone(0.6f, 1.2f, 36, glm::vec3(0.8f, 0.5f, 0.2f),
                 glm::vec3(0.8f, 0.5f, 0.2f), glm::vec3(0.5f, 0.5f, 0.5f),
                 32.0f);

  // Sphere for trees (outdoor)
  Sphere treeTopSphere(1.0f, 20, 12, glm::vec3(0.18f, 0.68f, 0.22f),
                       glm::vec3(0.18f, 0.68f, 0.22f),
                       glm::vec3(0.1f, 0.3f, 0.1f), 16.0f);

  // Cone for outdoor decorative bollards
  Cone bollardCone(0.2f, 0.8f, 20, glm::vec3(0.9f, 0.2f, 0.1f),
                   glm::vec3(0.9f, 0.2f, 0.1f), glm::vec3(0.4f, 0.4f, 0.4f),
                   16.0f);

// ─────────────────────────────────────────────────────────────────────────────
// RULED SURFACE FROM SPLINE CURVES (Decorative Pot)
// ─────────────────────────────────────────────────────────────────────────────
struct RuledSurface {
  unsigned int VAO, VBO, EBO;
  int indexCount;

  RuledSurface() {
    // Generate a ruled surface connecting a wavy spline curve (top rim) to a flat circle (base)
    vector<glm::vec3> pts1, pts2;
    int segments = 40;
    for (int i = 0; i <= segments; i++) {
      float t = (float)i / segments;
      float angle = t * 2.0f * 3.14159f;

      // Top profile: wavy circle using sine spline (flowers pot rim)
      float rTop = 0.6f + 0.08f * sinf(angle * 8.0f);
      pts1.push_back(glm::vec3(rTop * cosf(angle), 1.2f, rTop * sinf(angle)));

      // Bottom profile: smaller stable circle
      float rBot = 0.35f;
      pts2.push_back(glm::vec3(rBot * cosf(angle), 0.0f, rBot * sinf(angle)));
    }

    vector<float> vertices;
    vector<unsigned int> indices;

    for (int i = 0; i <= segments; i++) {
      int prev = (i == 0) ? segments - 1 : i - 1;
      int next = (i == segments) ? 1 : i + 1;

      glm::vec3 dp1 = pts1[next] - pts1[prev];
      glm::vec3 dp2 = pts2[next] - pts2[prev];

      glm::vec3 dir = pts2[i] - pts1[i];
      glm::vec3 n1 = glm::normalize(glm::cross(dir, dp1));
      glm::vec3 n2 = glm::normalize(glm::cross(dir, dp2));
      if (n1.y < 0) n1 = -n1;
      if (n2.y < 0) n2 = -n2;

      vertices.push_back(pts1[i].x); vertices.push_back(pts1[i].y); vertices.push_back(pts1[i].z);
      vertices.push_back(n1.x); vertices.push_back(n1.y); vertices.push_back(n1.z);
      vertices.push_back((float)i / segments); vertices.push_back(1.0f); // uv

      vertices.push_back(pts2[i].x); vertices.push_back(pts2[i].y); vertices.push_back(pts2[i].z);
      vertices.push_back(n2.x); vertices.push_back(n2.y); vertices.push_back(n2.z);
      vertices.push_back((float)i / segments); vertices.push_back(0.0f); // uv
    }

    for (int i = 0; i < segments; i++) {
      int r0 = i * 2;
      int r1 = r0 + 1;
      int r2 = r0 + 2;
      int r3 = r0 + 3;
      indices.push_back(r0); indices.push_back(r1); indices.push_back(r2);
      indices.push_back(r1); indices.push_back(r3); indices.push_back(r2);
    }
    indexCount = indices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
  }

  void draw(Shader &shader, glm::mat4 model, glm::vec3 color) {
    shader.use();
    shader.setMat4("model", model);
    shader.setVec3("material.ambient", color * 0.4f);
    shader.setVec3("material.diffuse", color);
    shader.setVec3("material.specular", glm::vec3(0.5f));
    shader.setFloat("material.shininess", 32.0f);
    glBindVertexArray(VAO);
    glDisable(GL_CULL_FACE);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glEnable(GL_CULL_FACE);
  }
};

// ── Bezier facade objects (dome, arch, pillar, railing, vase) ─────────────
BezierRevolution bezDome, bezArch, bezPillar, bezRailing, bezVase;
RuledSurface flowerPotSurface;
  {
    float cp[] = {0.f, 1.f, 0.2f, 0.9f, 0.9f, 0.4f, 1.f, 0.f};
    bezDome.setup(cp, 3, 30, 36);
  }
  {
    float cp[] = {0.55f, 0.f, 0.55f, 0.3f, 0.75f, 0.6f, 0.75f, 1.f, 0.6f, 1.f};
    bezArch.setup(cp, 4, 20, 36);
  }
  {
    float cp[] = {0.2f,  0.f,   0.28f, 0.1f,  0.45f, 0.35f, 0.5f,  0.5f, 0.42f,
                  0.65f, 0.22f, 0.78f, 0.22f, 0.88f, 0.38f, 0.95f, 0.4f, 1.f};
    bezVase.setup(cp, 8, 40, 32);
  }
  {
    float cp[] = {0.5f, 0.f, 0.52f, 0.2f, 0.52f, 0.4f, 0.46f, 0.7f, 0.38f, 1.f};
    bezPillar.setup(cp, 4, 28, 28);
  }
  {
    float cp[] = {0.12f, 0.f,  0.14f, 0.08f, 0.08f, 0.2f,  0.06f, 0.45f, 0.06f,
                  0.55f, 0.1f, 0.7f,  0.14f, 0.78f, 0.12f, 0.88f, 0.12f, 1.f};
    bezRailing.setup(cp, 8, 30, 20);
  }

  // ── Fractal tree parts + gate sphere finials ──────────────────────────────
  Cylinder treeBark(0.5f, 12, 8, glm::vec3(0.36f, 0.22f, 0.10f),
                    glm::vec3(0.36f, 0.22f, 0.10f), glm::vec3(0.1f, 0.1f, 0.1f),
                    8.0f);
  Cone treeLeaf(0.5f, 1.0f, 12, glm::vec3(0.18f, 0.55f, 0.15f),
                glm::vec3(0.18f, 0.55f, 0.15f), glm::vec3(0.1f, 0.2f, 0.1f),
                8.0f);
  Sphere gateFinial(0.5f, 24, 14, glm::vec3(0.85f, 0.82f, 0.78f),
                    glm::vec3(0.85f, 0.82f, 0.78f), glm::vec3(0.5f, 0.5f, 0.5f),
                    32.0f);

  // ─────────────────────────────────────────────────────────────────────────
  //  Main render loop
  // ─────────────────────────────────────────────────────────────────────────
  while (!glfwWindowShouldClose(window)) {
    float cur = glfwGetTime();
    deltaTime = cur - lastFrame;
    lastFrame = cur;
    processInput(window);

    if (fanOn)
      fanRot += 150.0f * deltaTime;
    if (openDoor && doorAngle < 90.0f)
      doorAngle += 80.0f * deltaTime;
    if (!openDoor && doorAngle > 0.0f)
      doorAngle -= 80.0f * deltaTime;

    float fridgeTarget = openFridge ? 108.f : 0.f;
    fridgeDoorAngle += (fridgeTarget - fridgeDoorAngle) * 7.5f * deltaTime;

    // AC panel animation
    float acPanelTarget = acOn ? 35.f : 0.f;
    acPanelAngle += (acPanelTarget - acPanelAngle) * 5.0f * deltaTime;
    if (acOn) acLouverPhase += deltaTime * 2.5f;

    // Cloud drift
    cloudDriftTime += deltaTime;

    int winW, winH;
    glfwGetFramebufferSize(window, &winW, &winH);

    float fullAspect = (float)winW / (float)winH;
    glm::mat4 viewInside = camera.GetViewMatrix();
    glm::mat4 I = glm::mat4(1.0f);

    // ────────────────────────────────────────────────────────────────────
    //  setupAndRender lambda – draws the entire scene
    // ────────────────────────────────────────────────────────────────────
    auto setupAndRender = [&](glm::mat4 view_, glm::mat4 proj_,
                              glm::vec3 viewPos_, bool dir_, bool point_,
                              bool spot_, bool ambOnly_, bool diffOnly_,
                              bool dirOnly_) {
      glm::mat4 I = glm::mat4(1.0f);

      // ── Helper: bind texShader and set shared uniforms ────────────────
      auto setupTexShader = [&]() {
        texShader.use();
        texShader.setVec3("viewPos", viewPos_);
        texShader.setMat4("projection", proj_);
        texShader.setMat4("view", view_);
        texShader.setInt("textureMode", textureMode);
        texShader.setInt("texture1", 0);

        // Component toggles
        if (ambOnly_) {
          texShader.setBool("ambientLight", true);
          texShader.setBool("diffuseLight", false);
          texShader.setBool("specularLight", false);
        } else if (diffOnly_) {
          texShader.setBool("ambientLight", false);
          texShader.setBool("diffuseLight", true);
          texShader.setBool("specularLight", false);
        } else {
          texShader.setBool("ambientLight", ambientOn);
          texShader.setBool("diffuseLight", diffuseOn);
          texShader.setBool("specularLight", specularOn);
        }

        // Directional
        texShader.setVec3("directionalLight.direction", -0.5f, -1.0f, -0.3f);
        texShader.setVec3("directionalLight.ambient", 0.56f, 0.54f, 0.48f);
        texShader.setVec3("directionalLight.diffuse", 1.00f, 0.97f, 0.86f);
        texShader.setVec3("directionalLight.specular", 0.50f, 0.48f, 0.35f);
        texShader.setBool("directionLightOn", dir_);

        // Point lights
        if (ambOnly_ || diffOnly_ || dirOnly_) {
          texShader.setVec3("pointLights[0].ambient", glm::vec3(0));
          texShader.setVec3("pointLights[0].diffuse", glm::vec3(0));
          texShader.setVec3("pointLights[0].specular", glm::vec3(0));
          texShader.setVec3("pointLights[1].ambient", glm::vec3(0));
          texShader.setVec3("pointLights[1].diffuse", glm::vec3(0));
          texShader.setVec3("pointLights[1].specular", glm::vec3(0));
          texShader.setBool("spotLightOn", false);
        } else {
          // pointlight1
          texShader.setVec3("pointLights[0].position", ptPos[0]);
          texShader.setVec3("pointLights[0].ambient",
                            glm::vec3(0.42f, 0.22f, 0.18f));
          texShader.setVec3("pointLights[0].diffuse",
                            glm::vec3(1.0f, 0.5f, 0.5f));
          texShader.setVec3("pointLights[0].specular",
                            glm::vec3(1.0f, 0.8f, 0.8f));
          texShader.setFloat("pointLights[0].k_c", 1.0f);
          texShader.setFloat("pointLights[0].k_l", 0.09f);
          texShader.setFloat("pointLights[0].k_q", 0.032f);
          // pointlight2
          texShader.setVec3("pointLights[1].position", ptPos[1]);
          texShader.setVec3("pointLights[1].ambient",
                            glm::vec3(0.18f, 0.18f, 0.40f));
          texShader.setVec3("pointLights[1].diffuse",
                            glm::vec3(0.5f, 0.5f, 1.0f));
          texShader.setVec3("pointLights[1].specular",
                            glm::vec3(0.8f, 0.8f, 1.0f));
          texShader.setFloat("pointLights[1].k_c", 1.0f);
          texShader.setFloat("pointLights[1].k_l", 0.09f);
          texShader.setFloat("pointLights[1].k_q", 0.032f);
          texShader.setBool("spotLightOn", spot_);
        }

        // Spot light (camera flashlight style so key 3 is always noticeable)
        texShader.setVec3("spotLight.position", viewPos_);
        texShader.setVec3("spotLight.direction", camera.Front);
        texShader.setVec3("spotLight.ambient", 0.15f, 0.05f, 0.15f);
        texShader.setVec3("spotLight.diffuse", 1.0f, 0.20f, 1.0f);
        texShader.setVec3("spotLight.specular", 1.0f, 0.50f, 1.0f);
        texShader.setFloat("spotLight.k_c", 1.0f);
        texShader.setFloat("spotLight.k_l", 0.09f);
        texShader.setFloat("spotLight.k_q", 0.032f);
        texShader.setFloat("spotLight.inner_circle", cosf(glm::radians(18.0f)));
        texShader.setFloat("spotLight.outer_circle", cosf(glm::radians(26.0f)));

        texShader.setVec3("material.emissive", glm::vec3(0.0f));
      };

      // ── Original lightingShader setup (no texture) ────────────────────
      lightingShader.use();
      lightingShader.setVec3("viewPos", viewPos_);
      lightingShader.setMat4("projection", proj_);
      lightingShader.setMat4("view", view_);

      if (ambOnly_) {
        lightingShader.setBool("ambientLight", true);
        lightingShader.setBool("diffuseLight", false);
        lightingShader.setBool("specularLight", false);
      } else if (diffOnly_) {
        lightingShader.setBool("ambientLight", false);
        lightingShader.setBool("diffuseLight", true);
        lightingShader.setBool("specularLight", false);
      } else {
        lightingShader.setBool("ambientLight", ambientOn);
        lightingShader.setBool("diffuseLight", diffuseOn);
        lightingShader.setBool("specularLight", specularOn);
      }

      pointlight1.setUpPointLight(lightingShader);
      pointlight2.setUpPointLight(lightingShader);

      lightingShader.setVec3("directionalLight.direction", -0.5f, -1.0f, -0.3f);
      lightingShader.setVec3("directionalLight.ambient", 0.56f, 0.54f, 0.48f);
      lightingShader.setVec3("directionalLight.diffuse", 1.00f, 0.97f, 0.86f);
      lightingShader.setVec3("directionalLight.specular", 0.50f, 0.48f, 0.35f);
      lightingShader.setBool("directionLightOn", dir_);

      if (ambOnly_ || diffOnly_ || dirOnly_) {
        lightingShader.setVec3("pointLights[0].ambient", glm::vec3(0));
        lightingShader.setVec3("pointLights[0].diffuse", glm::vec3(0));
        lightingShader.setVec3("pointLights[0].specular", glm::vec3(0));
        lightingShader.setVec3("pointLights[1].ambient", glm::vec3(0));
        lightingShader.setVec3("pointLights[1].diffuse", glm::vec3(0));
        lightingShader.setVec3("pointLights[1].specular", glm::vec3(0));
        lightingShader.setBool("spotLightOn", false);
      }

      lightingShader.setVec3("spotLight.position", viewPos_);
      lightingShader.setVec3("spotLight.direction", camera.Front);
      lightingShader.setVec3("spotLight.ambient", 0.15f, 0.05f, 0.15f);
      lightingShader.setVec3("spotLight.diffuse", 1.0f, 0.20f, 1.0f);
      lightingShader.setVec3("spotLight.specular", 1.0f, 0.50f, 1.0f);
      lightingShader.setFloat("spotLight.k_c", 1.0f);
      lightingShader.setFloat("spotLight.k_l", 0.09f);
      lightingShader.setFloat("spotLight.k_q", 0.032f);
      lightingShader.setFloat("spotLight.inner_circle",
                              cosf(glm::radians(18.0f)));
      lightingShader.setFloat("spotLight.outer_circle",
                              cosf(glm::radians(26.0f)));
      if (!dirOnly_ && !ambOnly_ && !diffOnly_)
        lightingShader.setBool("spotLightOn", spot_);
      else
        lightingShader.setBool("spotLightOn", false);

      lightingShader.setVec3("material.emissive", glm::vec3(0.0f));

      // ── Colour palette (unchanged from B1) ────────────────────────────
      // Changed exterior to bright base so the vivid orangey-wood texture shows fully
      glm::vec3 wallIn(0.96f, 0.95f, 0.93f), wallOut(1.0f, 0.9f, 0.85f);
      glm::vec3 floorCol(0.80f, 0.76f, 0.70f), ceilCol(0.99f, 0.99f, 0.99f);
      glm::vec3 roofCol(0.54f, 0.48f, 0.40f), roadCol(0.40f, 0.40f, 0.42f);
      glm::vec3 grassCol(0.28f, 0.72f, 0.32f), concreteC(0.70f, 0.70f, 0.68f);
      glm::vec3 sofaMain(0.22f, 0.32f, 0.52f), sofaArm(0.15f, 0.22f, 0.38f);
      glm::vec3 cushion(0.95f, 0.92f, 0.85f), tableTop(0.18f, 0.18f, 0.20f);
      glm::vec3 tableLeg(0.55f, 0.45f, 0.30f), bedFrame(0.25f, 0.25f, 0.28f);
      glm::vec3 bedSheet(0.96f, 0.96f, 0.98f), bedDuvet1(0.30f, 0.55f, 0.75f);
      glm::vec3 bedDuvet2(0.58f, 0.30f, 0.55f), pillow(0.96f, 0.96f, 0.98f);
      glm::vec3 kitchenC(0.30f, 0.26f, 0.22f), kitchenT(0.84f, 0.80f, 0.72f);
      glm::vec3 bathC(0.72f, 0.88f, 0.95f), bathInner(0.85f, 0.95f, 0.98f);
      glm::vec3 tvFrame(0.08f, 0.08f, 0.10f), tvScreen(0.05f, 0.15f, 0.30f);
      glm::vec3 woodDark(0.38f, 0.22f, 0.10f), woodMid(0.58f, 0.38f, 0.18f);
      glm::vec3 doorCol(0.18f, 0.18f, 0.20f), fanHub(0.20f, 0.20f, 0.22f);
      glm::vec3 fanBlade(0.58f, 0.48f, 0.35f), stairCol(0.65f, 0.55f, 0.40f);
      glm::vec3 liftCol(0.55f, 0.65f, 0.75f), shelfCol(0.58f, 0.38f, 0.18f);
      glm::vec3 swingCol(0.18f, 0.18f, 0.20f), swingRope(0.65f, 0.55f, 0.38f);
      glm::vec3 swingSeat(0.40f, 0.62f, 0.82f), lampGlow2(1.0f, 0.95f, 0.70f);

      auto box = [&](float x, float y, float z, float sx, float sy, float sz,
                     glm::vec3 col) {
        float px = sx < 0 ? x + sx : x;
        float py = sy < 0 ? y + sy : y;
        float pz = sz < 0 ? z + sz : z;
        glm::mat4 m = glm::translate(I, glm::vec3(px, py, pz));
        m = glm::scale(m, glm::vec3(fabs(sx), fabs(sy), fabs(sz)));
        drawCube(VAO, lightingShader, m, col);
      };

      auto mirrorBox = [&](float x, float y, float z, float sx, float sy,
                           float sz, glm::vec3 col) {
        float px = sx < 0 ? x + sx : x;
        float py = sy < 0 ? y + sy : y;
        float pz = sz < 0 ? z + sz : z;
        glm::mat4 m = glm::translate(I, glm::vec3(px, py, pz));
        m = glm::scale(m, glm::vec3(fabs(sx), fabs(sy), fabs(sz)));
        lightingShader.use();
        lightingShader.setVec3("material.ambient", col * 0.7f);
        lightingShader.setVec3("material.diffuse", col);
        lightingShader.setVec3("material.specular", glm::vec3(1.0f));
        lightingShader.setFloat("material.shininess", 128.0f);
        lightingShader.setVec3("material.emissive", col * 0.15f);
        lightingShader.setMat4("model", m);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        lightingShader.setVec3("material.emissive", glm::vec3(0.0f));
      };

      // ── Textured draw helper ──────────────────────────────────────────
      // Draws a [0,1] cube with given texture and material colour
      auto texBox = [&](float x, float y, float z, float sx, float sy, float sz,
                        glm::vec3 matColor, unsigned int tex) {
        float px = sx < 0 ? x + sx : x;
        float py = sy < 0 ? y + sy : y;
        float pz = sz < 0 ? z + sz : z;

        setupTexShader();
        texShader.setVec3("objectColor", matColor); // vertex colour
        texShader.setVec3("material.ambient", matColor * 0.4f);
        texShader.setVec3("material.diffuse", matColor);
        texShader.setVec3("material.specular", glm::vec3(0.3f));
        texShader.setFloat("material.shininess", 32.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glm::mat4 m = glm::translate(I, glm::vec3(px, py, pz));
        m = glm::scale(m, glm::vec3(fabs(sx), fabs(sy), fabs(sz)));
        texShader.setMat4("model", m);

        glBindVertexArray(tVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Restore lightingShader as active (box() uses it)
        lightingShader.use();
      };

      // ── LIFT ANIMATION LOGIC ──────────────────────────────────────────
      {
        float liftTargetY = liftUp ? 5.0f : 0.0f;
        float liftSpeed = 2.5f * deltaTime;
        float doorTargetAngle = 0.0f;

        if (std::abs(liftY - liftTargetY) > 0.01f) {
          liftMoving = true;
          doorTargetAngle = 0.0f; // close doors before moving
          if (liftDoorAngle <= 0.1f) {
            if (liftY < liftTargetY)
              liftY += liftSpeed;
            else
              liftY -= liftSpeed;
            if (std::abs(liftY - liftTargetY) < liftSpeed)
              liftY = liftTargetY;
          }
        } else {
          liftMoving = false;
          doorTargetAngle = 90.0f; // open doors when arrived
        }
        // smooth doors
        float ds = 150.0f * deltaTime;
        if (liftDoorAngle < doorTargetAngle)
          liftDoorAngle += ds;
        if (liftDoorAngle > doorTargetAngle)
          liftDoorAngle -= ds;
        liftDoorAngle = glm::clamp(liftDoorAngle, 0.0f, 90.0f);
      }

      // ═════════════════════════════════════════════════════════════════
      //  TEXTURED SURFACES
      //  (Assignment B2: textured floor, wall, ceiling, grass, roof, door)
      // ═════════════════════════════════════════════════════════════════

      // ── FLOOR (textured) ──────────────────────────────────────────────
      texBox(-10, -0.99f, -12, 20, -0.1f, 22, floorCol, texFloor);
      texBox(-10, -0.99f, -20, 14, -0.1f, 8, concreteC, texFloor);
      texBox(4, -0.99f, -20, 6, -0.1f, 8, bathC, texFloor);

      // ── GRASS (textured) ──────────────────────────────────────────────
      // Massive expanded grass terrain, lowered significantly to stop flickering with road
      texBox(-150, -1.20f, -150, 300, -0.05f, 300, grassCol, texGrass);

      // ── WALLS – exterior (textured) ───────────────────────────────────
      texBox(-10.1f, -1, -20.1f, 20.2f, 7, 0.1f, wallOut, texWall);
      texBox(-10.1f, -1, 10.1f, 20.2f, 7, -0.1f, glm::vec3(1.05f, 1.02f, 0.96f),
             texWall);
      texBox(10.1f, -1, -20.0f, -0.1f, 7, 30.0f, wallOut, texWall);
      texBox(-10.1f, -1, -20.0f, 0.1f, 7, 1.0f, wallOut, texWall);
      texBox(-10.1f, -1, -13.0f, 0.1f, 7, 1.0f, wallOut, texWall);
      texBox(-10.1f, -1, -12.0f, 0.1f, 7, 4.0f, wallOut, texWall);
      texBox(-10.1f, -1, -4.0f, 0.1f, 7, 14.0f, wallOut, texWall);

      // ── WALLS – interior (solid colour, mode-dependent) ───────────────
      box(-10, -1, -20, 20, 4.98f, 0.1f, wallIn);
      // South/front wall remains plain; mosaic moved to TV room-facing walls.
      box(-10, -1, 10, 12, 4.98f, -0.1f, wallIn);
      box(2, -1, 10, 8, 4.98f, -0.1f, wallIn);
      
      box(10, -1, -20, -0.1f, 4.98f, 30, wallIn);
      box(-10, -1, -20, 0.1f, 4.98f, 1, wallIn);
      // Removed garage lintel to fully open the space upwards.
      box(-10, -1, -13, 0.1f, 4.98f, 1, wallIn);
      box(-10, -1, -12, 0.1f, 4.98f, 4, wallIn);
      // Split west wall to apply mosaic ONLY to TV Room area (Z = 0 to 10)
      box(-10, -1, -4, 0.1f, 4.98f, 4, wallIn); // Solid outside TV room (Z = -4 to 0)
      texBox(-10, -1, 0, 0.1f, 4.98f, 10, wallIn, texMosaic); // Mosaic inside TV room (Z = 0 to 10)
      
      // inner partitions
      box(4, -1, -20, -0.1f, 4.98f, 8, wallIn);
      // TV wall gets mosaic so dining room no longer carries it.
      texBox(-10, -1, -12, 14, 4.98f, -0.1f, wallIn, texMosaic);
      box(4, 3, -12, 2, 0.98f, -0.1f, wallIn);
      box(6, -1, -12, 4, 4.98f, -0.1f, wallIn);
      
      // TV Room inner partitions (Textured with Mosaic)
      texBox(2, -1, 0, -0.1f, 4.98f, 8, wallIn, texMosaic);
      texBox(2, 3, 8, -0.1f, 0.98f, 2, wallIn, texMosaic);
      texBox(-10, -1, 0, 10, 4.98f, -0.1f, wallIn, texMosaic);
      texBox(0, 3, 0, 2, 0.98f, -0.1f, wallIn, texMosaic);
      texBox(2, 3, 0, 2, 0.98f, -0.1f, wallIn, texMosaic);
      
      box(4, -1, 0, 6, 4.98f, -0.1f, wallIn);
      // lift shaft
      box(6, -1, -4, 0.1f, 10, 4, liftCol);
      box(6, -1, -4, 0.5f, 10, 0.1f, liftCol);
      box(9.5f, -1, -4, 0.5f, 10, 0.1f, liftCol);
      box(6, 4, 0, 4, 4.98f, -0.1f, liftCol);
      // 2F walls
      box(-4, 4, -15, 7, 5, 0.1f, wallIn);
      texBox(-4.1f, 4, -15.1f, 7.1f, 5, 0.1f, wallOut, texWall);
      box(6, 8, -15, 2, 1, 0.1f, wallOut);
      box(8, 4, -15, 2, 5, 0.1f, wallIn);
      texBox(8, 4, -15.1f, 2, 5, 0.1f, wallOut, texWall);
      box(-4, 4, 5, 7, 5, -0.1f, wallIn);
      texBox(-4.1f, 4, 5.1f, 7.1f, 5, -0.1f, wallOut, texWall);
      box(6, 8, 5, 2, 1, -0.1f, wallOut);
      box(8, 4, 5, 2, 5, -0.1f, wallIn);
      texBox(8, 4, 5.1f, 2, 5, -0.1f, wallOut, texWall);
      box(10, 4, -15, -0.1f, 5, 20, wallIn);
      texBox(10.1f, 6, -15.1f, -0.1f, 3, 20.2f, wallOut, texWall);
      // Uncommented wallIn statements replaced cleanly by the localized top floor rendering
      // box(-4, 4, -15, 0.1f, 5, 8.5f, wallIn);
      texBox(-4.1f, 4, -15, 0.1f, 5, 8.5f, wallOut, texWall);
      // box(-4, 8, -6.5f, 0.1f, 1, 3, wallIn);
      texBox(-4.1f, 8, -6.5f, 0.1f, 1, 3, wallOut, texWall);
      // box(-4, 4, -3.5f, 0.1f, 5, 8.5f, wallIn);
      texBox(-4.1f, 4, -3.5f, 0.1f, 5, 8.5f, wallOut, texWall);
      // box(3, 4, -15, 0.1f, 5, 6, wallIn);
      // box(3, 8, -9, 0.1f, 1, 2, wallIn);
      // box(3, 8, -3, 0.1f, 1, 2, wallIn);
      // box(3, 4, -1, 0.1f, 5, 6, wallIn);
      // (Redundant cross-room partitions at Z=-7 and Z=-3 removed to unify the top floor)

      // ── 2nd Floor Terrace Boundary (filling gaps) ─────────────────────
      texBox(-10.1f, 4, -19.1f, 0.1f, 2, 6.2f, wallOut,
             texWall); // above garage/window
      texBox(-10.1f, 4, -8.1f, 0.1f, 2, 4.2f, wallOut,
             texWall); // above door area
      texBox(-10.1f, 4, -13.1f, 0.1f, 2, 1.2f, wallOut,
             texWall); // small connector segment

      // Top cap for boundary to look more like a railing
      box(-10.15f, 6.0f, -20.1f, 0.2f, 0.15f, 30.2f, glm::vec3(0.35f));
      box(10.05f, 6.0f, -20.1f, 0.2f, 0.15f, 30.2f, glm::vec3(0.35f));
      box(-10.15f, 6.0f, -20.1f, 20.3f, 0.15f, 0.2f, glm::vec3(0.35f));
      box(-10.15f, 6.0f, 10.0f, 20.3f, 0.15f, -0.2f, glm::vec3(0.35f));

      // ── CEILING (textured) ────────────────────────────────────────────
      texBox(-10, 4.1f, -12, 16, -0.1f, 22, ceilCol, texCeiling);
      texBox(-10, 4.1f, -20, 14, -0.1f, 8, ceilCol, texCeiling);
      texBox(6, 4.1f, 0, 4, -0.1f, 10, ceilCol, texCeiling);
      texBox(6, 4.1f, -7, 4, -0.1f, 3, ceilCol, texCeiling);
      texBox(6, 4.1f, -12, 4, -0.1f, 12, ceilCol, texCeiling); // Complete right terrace floor
      // 2F ceiling
      texBox(-4, 9, -15, 14, 0.1f, 20, ceilCol, texCeiling);

      // ── ROOF (textured) ───────────────────────────────────────────────
      texBox(-11, 4.25f, -21, 22, -0.1f, 9, roofCol, texRoof);
      // Removed the large central intersecting roof slab so it doesn't clip through the top floor at Y=4.25f
      // Left side roof (beside the top floor)
      texBox(-11, 4.25f, -12, 7, -0.1f, 23, roofCol, texRoof); 
      // Front side roof (in front of the top floor)
      texBox(-4, 4.25f, 5, 10, -0.1f, 6, roofCol, texRoof); 
      
      texBox(6, 4.25f, -7, 4, -0.1f, 3, roofCol, texRoof);
      texBox(10, 4.25f, -12, 1, -0.1f, 12, roofCol, texRoof);
      texBox(6, 4.25f, 0, 5, -0.1f, 11, roofCol, texRoof);
      texBox(-5, 9.25f, -16, 11, 0.1f, 22, wallOut, texRoof);
      texBox(6, 9.25f, -7, 5, 0.1f, 13, roofCol, texRoof);
      texBox(6, 9.25f, -16, 5, 0.1f, 4, roofCol, texRoof);
      texBox(10, 9.25f, -12, 1, 0.1f, 5, roofCol, texRoof);

      // ── ROAD (solid) ──────────────────────────────────────────────────
      // Positioned exactly outside the boundaries
      box(-20.37f, -1, -40.37f, -10, -0.1f, 70.74f, roadCol); // Left road
      box(20.37f, -1, -40.37f, 10, -0.1f, 70.74f, roadCol);   // Right road
      box(-30.37f, -1, -30.37f, 60.74f, -0.1f, -10, roadCol); // Back road
      box(-30.37f, -1, 20.37f, 60.74f, -0.1f, 10, roadCol);   // Front road

      // Change this lambda parameter name from 'texDoor' to avoid shadowing the
      // outer variable
      auto texDoorFunc = [&](float px, float py, float pz, float sx, float sy3,
                             float sz3, float angle) {
        setupTexShader();
        texShader.setVec3("objectColor", doorCol);
        texShader.setVec3("material.ambient", doorCol * 0.3f);
        texShader.setVec3("material.diffuse", doorCol);
        texShader.setVec3("material.specular", glm::vec3(0.5f));
        texShader.setFloat("material.shininess", 64.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texDoor);
        glm::mat4 T = glm::translate(I, glm::vec3(px, py, pz));
        glm::mat4 R = glm::rotate(I, glm::radians(angle), glm::vec3(0, 1, 0));
        glm::mat4 S = glm::scale(I, glm::vec3(sx, sy3, sz3));
        texShader.setMat4("model", T * R * S);
        glBindVertexArray(tVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        lightingShader.use();
      };
      // Use texDoorFunc instead of texDoor for the lambda calls below
      texDoorFunc(-10, -1, -8, 0.1f, 4, 2, doorAngle);
      texDoorFunc(-10, -1, -4, 0.1f, 4, -2, -doorAngle);
      texDoorFunc(6, -1, -12, -2, 4, -0.1f, doorAngle);
      texDoorFunc(2, -1, 8, -0.1f, 4, 2, -doorAngle);
      texDoorFunc(0, -1, 0, 2, 4, -0.1f, doorAngle);
      texDoorFunc(4, -1, 0, -2, 4, -0.1f, -doorAngle);
      texDoorFunc(8, 4, -15, -2, 4, 0.1f, -doorAngle);
      // texDoorFunc(8, 4, 5, -2, 4, -0.1f, doorAngle);
      // texDoorFunc(3, 4, -9, 0.1f, 4, 2, doorAngle);
      // texDoorFunc(3, 4, -1, 0.1f, 4, -2, -doorAngle);
      // Top floor LEFT-WALL entry door (same side as ground-floor main entrance)
      texDoorFunc(-4, 4, -6.5f, 0.1f, 4, 3.0f, doorAngle);
      // Bedroom <-> Bathroom dividing-wall door
      // texDoorFunc(0, 4, -5, 0.1f, 4, 2.0f, doorAngle);

      // ── STAIRS ────────────────────────────────────────────────────────
      // Added solid base behind/under the stairs to remove floating look.
      float sy = 4.0f, sz = -12.0f;
      for (int i = 0; i < 10; i++) {
        // base block under the stair down to floor
        box(8, -1.0f, sz, 2, sy + 1.0f, 0.5f, stairCol);
        // tread (textured)
        texBox(8, sy, sz, 2, -0.1f, 0.5f, stairCol, texStairs);
        sz += 0.5f;
        // riser (textured)
        texBox(8, sy, sz + 0.1f, 2, -0.5f, -0.1f, stairCol, texStairs);
        sy -= 0.5f;
      }
      float sy2 = 9.0f, sz2 = -7.0f;
      for (int i = 0; i < 10; i++) {
        // base block under the stair down to floor (Top floor is y=4.0)
        box(6, 4.0f, sz2 - 0.4f, 2, sy2 - 4.0f, 0.5f, stairCol);
        // tread (textured)
        texBox(6, sy2, sz2 - 0.4f, 2, -0.1f, 0.5f, stairCol, texStairs);
        sz2 -= 0.5f;
        // riser (textured)
        texBox(6, sy2, sz2 + 0.1f, 2, -0.5f, -0.1f, stairCol, texStairs);
        sy2 -= 0.5f;
      }
      // Continuous back support so stairs do not look floating from the side.
     

      // ── SOFA (textured with sofa.jpg) – TV room (1st room)
      // Sofa placed firmly inside room 1 (z=-3 to z=-1), well away from the z=0
      // partition.
      // ── SOFA (textured with sofa.jpg) – TV room (1st room)
      // ── SOFA – faces TV (open side toward -z, backrest at z=-1)
      // ── SOFA – centered in room away from door
      texBox(-6.0f, -0.5f, -3.0f, 5.0f, 1.0f, 2.0f, sofaMain,
             texSofa); // seat base
      texBox(-6.0f, 0.5f, -1.4f, 5.0f, 1.5f, -0.4f, sofaArm,
             texSofa); // backrest
      texBox(-6.4f, -0.5f, -3.0f, 0.4f, 1.8f, 2.0f, sofaArm,
             texSofa); // left arm
      texBox(-1.0f, -0.5f, -3.0f, 0.4f, 1.8f, 2.0f, sofaArm,
             texSofa); // right arm
      texBox(-5.9f, 0.0f, -2.9f, 2.3f, 0.5f, 1.3f, cushion,
             texSofa); // left cushion
      texBox(-3.5f, 0.0f, -2.9f, 2.3f, 0.5f, 1.3f, cushion,
             texSofa); // right cushion
      // ── TEA TABLE removed (user requested) ─────────────────────────
      // (table behind/near sofa removed)

      // ── DINING TABLE + CHAIRS (2nd room, dining room) ─────────────────
      // Table top
      box(-7.5f, 1.0f, 5.0f, 7.0f, 0.1f, 3.0f, tableTop);
      // Table legs
      box(-7.5f, 1.0f, 5.0f, 0.1f, -2.0f, 0.1f, tableLeg);
      box(-7.5f, 1.0f, 8.0f, 0.1f, -2.0f, -0.1f, tableLeg);
      box(-0.5f, 1.0f, 5.0f, -0.1f, -2.0f, 0.1f, tableLeg);
      box(-0.5f, 1.0f, 8.0f, -0.1f, -2.0f, -0.1f, tableLeg);

      {
        glm::vec3 cSeat(0.455f, 0.235f, 0.102f);
        glm::vec3 cBack(0.329f, 0.173f, 0.110f);

        // Far side chairs (z~8): backrest faces +z (away from table)
        // Chair 1 far
        box(-6.0f, 0.0f, 8.0f, 1.0f, 0.1f, 1.0f, cSeat);
        box(-6.0f, 0.0f, 8.0f, 0.1f, -1.0f, 0.1f, cBack);
        box(-6.0f, 0.0f, 9.0f, 0.1f, -1.0f, -0.1f, cBack);
        box(-5.0f, 0.0f, 8.0f, -0.1f, -1.0f, 0.1f, cBack);
        box(-5.0f, 0.0f, 9.0f, -0.1f, -1.0f, -0.1f, cBack);
        box(-6.0f, 0.0f, 9.0f, 1.0f, 1.5f, -0.1f,
            cBack); // backrest vertical slab

        // Chair 2 far
        box(-3.0f, 0.0f, 8.0f, 1.0f, 0.1f, 1.0f, cSeat);
        box(-3.0f, 0.0f, 8.0f, 0.1f, -1.0f, 0.1f, cBack);
        box(-3.0f, 0.0f, 9.0f, 0.1f, -1.0f, -0.1f, cBack);
        box(-2.0f, 0.0f, 8.0f, -0.1f, -1.0f, 0.1f, cBack);
        box(-2.0f, 0.0f, 9.0f, -0.1f, -1.0f, -0.1f, cBack);
        box(-3.0f, 0.0f, 9.0f, 1.0f, 1.5f, -0.1f, cBack);

        // Near side chairs (z~4): backrest faces -z (away from table)
        // Chair 1 near
        box(-6.0f, 0.0f, 4.0f, 1.0f, 0.1f, 1.0f, cSeat);
        box(-6.0f, 0.0f, 4.0f, 0.1f, -1.0f, 0.1f, cBack);
        box(-6.0f, 0.0f, 5.0f, 0.1f, -1.0f, -0.1f, cBack);
        box(-5.0f, 0.0f, 4.0f, -0.1f, -1.0f, 0.1f, cBack);
        box(-5.0f, 0.0f, 5.0f, -0.1f, -1.0f, -0.1f, cBack);
        box(-6.0f, 0.0f, 4.0f, 1.0f, 1.5f, 0.1f, cBack); // backrest faces -z

        // Chair 2 near
        box(-3.0f, 0.0f, 4.0f, 1.0f, 0.1f, 1.0f, cSeat);
        box(-3.0f, 0.0f, 4.0f, 0.1f, -1.0f, 0.1f, cBack);
        box(-3.0f, 0.0f, 5.0f, 0.1f, -1.0f, -0.1f, cBack);
        box(-2.0f, 0.0f, 4.0f, -0.1f, -1.0f, 0.1f, cBack);
        box(-2.0f, 0.0f, 5.0f, -0.1f, -1.0f, -0.1f, cBack);
        box(-3.0f, 0.0f, 4.0f, 1.0f, 1.5f, 0.1f, cBack);

        // Side chairs (one each side of table, backrest faces outward)
        // Left side chair (x~-8.5, z~6): backrest faces -x
        box(-8.5f, 0.0f, 6.0f, 1.0f, 0.1f, 1.0f, cSeat);
        box(-8.5f, 0.0f, 6.0f, 0.1f, -1.0f, 0.1f, cBack);
        box(-8.5f, 0.0f, 7.0f, 0.1f, -1.0f, -0.1f, cBack);
        box(-7.5f, 0.0f, 6.0f, -0.1f, -1.0f, 0.1f, cBack);
        box(-7.5f, 0.0f, 7.0f, -0.1f, -1.0f, -0.1f, cBack);
        box(-8.5f, 0.0f, 6.0f, 0.1f, 1.5f, 1.0f, cBack); // backrest on left

        // Right side chair (x~-0.5, z~6): backrest faces +x
        box(-0.5f, 0.0f, 6.0f, 1.0f, 0.1f, 1.0f, cSeat);
        box(-0.5f, 0.0f, 6.0f, 0.1f, -1.0f, 0.1f, cBack);
        box(-0.5f, 0.0f, 7.0f, 0.1f, -1.0f, -0.1f, cBack);
        box(0.5f, 0.0f, 6.0f, -0.1f, -1.0f, 0.1f, cBack);
        box(0.5f, 0.0f, 7.0f, -0.1f, -1.0f, -0.1f, cBack);
        box(0.5f, 0.0f, 6.0f, -0.1f, 1.5f, 1.0f, cBack); // backrest on right
      }
      // ── BOOKSHELF (2nd room, against x=2 partition wall) ─────────────
      // Unit: x=1.6–2.0 (30cm deep), z=2.0–5.0 (3m wide), y=-1 to 4 (5m tall)
      {
        glm::vec3 shelfWood(0.55f, 0.33f, 0.11f); // warm walnut
        glm::vec3 bkRed(0.72f, 0.08f, 0.08f);
        glm::vec3 bkBlue(0.12f, 0.22f, 0.68f);
        glm::vec3 bkGreen(0.10f, 0.50f, 0.20f);
        glm::vec3 bkYellow(0.82f, 0.74f, 0.08f);
        glm::vec3 bkCream(0.92f, 0.88f, 0.78f);
        glm::vec3 bkOrange(0.88f, 0.40f, 0.08f);
        glm::vec3 bkPurple(0.48f, 0.10f, 0.58f);
        glm::vec3 bkTeal(0.08f, 0.55f, 0.52f);
        glm::vec3 bkBrown(0.40f, 0.20f, 0.08f);

        // ── Frame ──────────────────────────────────────────────────
        // Back panel (thin, flush with wall)
        box(1.92f, -1.0f, 2.0f, 0.08f, 5.0f, 3.0f, shelfWood);
        // Left side panel
        box(1.60f, -1.0f, 1.95f, 0.35f, 5.0f, 0.08f, shelfWood);
        // Right side panel
        box(1.60f, -1.0f, 5.0f, 0.35f, 5.0f, -0.08f, shelfWood);
        // Top panel
        box(1.60f, 4.0f, 1.95f, 0.35f, 0.12f, 3.1f, shelfWood);
        // Bottom panel (sits on floor)
        box(1.60f, -1.0f, 1.95f, 0.35f, 0.12f, 3.1f, shelfWood);
        // 4 horizontal shelves (evenly spaced between y=-0.88 and y=4)
        // shelf 1 at y=0.2
        box(1.60f, 0.20f, 1.95f, 0.35f, 0.10f, 3.1f, shelfWood);
        // shelf 2 at y=1.4
        box(1.60f, 1.40f, 1.95f, 0.35f, 0.10f, 3.1f, shelfWood);
        // shelf 3 at y=2.6
        box(1.60f, 2.60f, 1.95f, 0.35f, 0.10f, 3.1f, shelfWood);
        // shelf 4 at y=3.8  (below top panel)
        box(1.60f, 3.80f, 1.95f, 0.35f, 0.10f, 3.1f, shelfWood);

        // ── Books ──────────────────────────────────────────────────
        // Books stand upright: depth in x (0.25f), height in y, spine width in
        // z They sit on each shelf, spines face -x (toward viewer in room)
        // Bottom compartment y=-0.88 to 0.2 → books height ~0.9
        // (sx=-0.25 so book goes from x=1.85 toward x=1.60, spine at x=1.60)
        float bx = 1.85f; // front face of book (spine at bx-0.25)
        // Row 0 (floor to shelf1): y=-0.88, height=1.0
        box(bx, -0.88f, 2.08f, -0.25f, 1.0f, 0.22f, bkRed);
        box(bx, -0.88f, 2.32f, -0.25f, 1.0f, 0.18f, bkBlue);
        box(bx, -0.88f, 2.52f, -0.25f, 1.0f, 0.24f, bkGreen);
        box(bx, -0.88f, 2.78f, -0.25f, 0.9f, 0.20f, bkYellow);
        box(bx, -0.88f, 3.00f, -0.25f, 1.0f, 0.22f, bkCream);
        box(bx, -0.88f, 3.24f, -0.25f, 0.9f, 0.18f, bkOrange);
        box(bx, -0.88f, 3.44f, -0.25f, 1.0f, 0.22f, bkPurple);
        box(bx, -0.88f, 3.68f, -0.25f, 0.9f, 0.20f, bkTeal);
        box(bx, -0.88f, 3.90f, -0.25f, 1.0f, 0.24f, bkBrown);
        box(bx, -0.88f, 4.16f, -0.25f, 0.9f, 0.18f, bkRed);
        box(bx, -0.88f, 4.36f, -0.25f, 1.0f, 0.22f, bkBlue);
        box(bx, -0.88f, 4.60f, -0.25f, 0.9f, 0.20f, bkGreen);
        // Row 1 (shelf1 to shelf2): y=0.30
        box(bx, 0.30f, 2.08f, -0.25f, 1.0f, 0.20f, bkPurple);
        box(bx, 0.30f, 2.30f, -0.25f, 1.0f, 0.24f, bkOrange);
        box(bx, 0.30f, 2.56f, -0.25f, 0.9f, 0.18f, bkCream);
        box(bx, 0.30f, 2.76f, -0.25f, 1.0f, 0.22f, bkTeal);
        box(bx, 0.30f, 3.00f, -0.25f, 1.0f, 0.20f, bkRed);
        box(bx, 0.30f, 3.22f, -0.25f, 0.9f, 0.24f, bkYellow);
        box(bx, 0.30f, 3.48f, -0.25f, 1.0f, 0.18f, bkBrown);
        box(bx, 0.30f, 3.68f, -0.25f, 0.9f, 0.22f, bkBlue);
        box(bx, 0.30f, 3.92f, -0.25f, 1.0f, 0.20f, bkGreen);
        box(bx, 0.30f, 4.14f, -0.25f, 1.0f, 0.24f, bkPurple);
        box(bx, 0.30f, 4.40f, -0.25f, 0.9f, 0.18f, bkOrange);
        box(bx, 0.30f, 4.60f, -0.25f, 1.0f, 0.22f, bkRed);
        // Row 2 (shelf2 to shelf3): y=1.50
        box(bx, 1.50f, 2.08f, -0.25f, 1.0f, 0.22f, bkTeal);
        box(bx, 1.50f, 2.32f, -0.25f, 0.9f, 0.18f, bkYellow);
        box(bx, 1.50f, 2.52f, -0.25f, 1.0f, 0.24f, bkRed);
        box(bx, 1.50f, 2.78f, -0.25f, 1.0f, 0.20f, bkBlue);
        box(bx, 1.50f, 3.00f, -0.25f, 0.9f, 0.22f, bkGreen);
        box(bx, 1.50f, 3.24f, -0.25f, 1.0f, 0.18f, bkCream);
        box(bx, 1.50f, 3.44f, -0.25f, 1.0f, 0.24f, bkOrange);
        box(bx, 1.50f, 3.70f, -0.25f, 0.9f, 0.20f, bkPurple);
        box(bx, 1.50f, 3.92f, -0.25f, 1.0f, 0.22f, bkBrown);
        box(bx, 1.50f, 4.14f, -0.25f, 1.0f, 0.18f, bkTeal);
        box(bx, 1.50f, 4.34f, -0.25f, 0.9f, 0.24f, bkRed);
        box(bx, 1.50f, 4.58f, -0.25f, 1.0f, 0.20f, bkBlue);
        // Row 3 (shelf3 to shelf4): y=2.70
        box(bx, 2.70f, 2.08f, -0.25f, 1.0f, 0.20f, bkGreen);
        box(bx, 2.70f, 2.30f, -0.25f, 0.9f, 0.22f, bkYellow);
        box(bx, 2.70f, 2.54f, -0.25f, 1.0f, 0.18f, bkPurple);
        box(bx, 2.70f, 2.74f, -0.25f, 1.0f, 0.24f, bkCream);
        box(bx, 2.70f, 3.00f, -0.25f, 0.9f, 0.20f, bkOrange);
        box(bx, 2.70f, 3.22f, -0.25f, 1.0f, 0.22f, bkBrown);
        box(bx, 2.70f, 3.46f, -0.25f, 1.0f, 0.18f, bkTeal);
        box(bx, 2.70f, 3.66f, -0.25f, 0.9f, 0.24f, bkRed);
        box(bx, 2.70f, 3.92f, -0.25f, 1.0f, 0.20f, bkBlue);
        box(bx, 2.70f, 4.14f, -0.25f, 1.0f, 0.22f, bkGreen);
        box(bx, 2.70f, 4.38f, -0.25f, 0.9f, 0.18f, bkYellow);
        box(bx, 2.70f, 4.58f, -0.25f, 1.0f, 0.24f, bkPurple);
        // Row 4 (shelf4 to top): y=3.90
        box(bx, 3.90f, 2.08f, -0.25f, 0.8f, 0.22f, bkBrown);
        box(bx, 3.90f, 2.32f, -0.25f, 0.8f, 0.18f, bkOrange);
        box(bx, 3.90f, 2.52f, -0.25f, 0.9f, 0.24f, bkCream);
        box(bx, 3.90f, 2.78f, -0.25f, 0.8f, 0.20f, bkTeal);
        box(bx, 3.90f, 3.00f, -0.25f, 0.9f, 0.22f, bkRed);
        box(bx, 3.90f, 3.24f, -0.25f, 0.8f, 0.18f, bkBlue);
        box(bx, 3.90f, 3.44f, -0.25f, 0.9f, 0.24f, bkGreen);
        box(bx, 3.90f, 3.70f, -0.25f, 0.8f, 0.20f, bkYellow);
        box(bx, 3.90f, 3.92f, -0.25f, 0.9f, 0.22f, bkPurple);
        box(bx, 3.90f, 4.14f, -0.25f, 0.8f, 0.18f, bkOrange);
        // Per-book textured front faces (books.jpg), slight epsilon prevents flicker.
        float rowYTex[5] = {-0.88f, 0.30f, 1.50f, 2.70f, 3.90f};
        float rowHTex[5] = {1.0f, 1.0f, 1.0f, 1.0f, 0.8f};
        for (int rr = 0; rr < 5; rr++) {
          for (int bi = 0; bi < 12; bi++) {
            float bzTex = 2.08f + 0.22f * bi;
            float hTex = rowHTex[rr] - 0.08f + 0.08f * ((bi + rr) % 3);
            float wTex = 0.16f + 0.04f * ((bi * 2 + rr) % 3);
            texBox(1.602f + rr * 0.001f, rowYTex[rr], bzTex, 0.01f, hTex, wTex,
                   glm::vec3(1.0f), texBooks);
          }
        }
        // ═══════════════════════════════════════════════════════════
        //  TOP FLOOR — Bedroom + Bathroom  (x=-4..3, z=-15..5, y=4)
        // ═══════════════════════════════════════════════════════════
        // Palette
        glm::vec3 flrTop(0.92f, 0.90f, 0.88f); // Soft continuous floor
        glm::vec3 wallColor(0.95f, 0.93f, 0.88f); // Warm unified walls
        glm::vec3 ceilCol2(0.94f, 0.93f, 0.91f);
        glm::vec3 skirtDk(0.28f, 0.16f, 0.06f);
        glm::vec3 crownW(0.92f, 0.91f, 0.89f);
        glm::vec3 windowSky(0.70f, 0.87f, 0.98f); // sky-blue glass
        glm::vec3 frameW(0.93f, 0.93f, 0.92f);
        glm::vec3 curtPink(0.76f, 0.52f, 0.58f);
        glm::vec3 curtBlue(0.52f, 0.62f, 0.82f);
        
        glm::vec3 fixtureC(0.85f, 0.86f, 0.88f); // chrome
        glm::vec3 commodeC(0.90f, 0.85f, 0.80f); // Warm beige/sand color
        glm::vec3 bathtubC(0.90f, 0.94f, 0.98f);
        glm::vec3 bathtubInn(0.96f, 0.98f, 1.00f);
        glm::vec3 basinC(0.94f, 0.95f, 0.96f);
        glm::vec3 doorWood(0.42f, 0.28f, 0.12f);

        float Y = 4.0f;  // floor y
        float RH = 5.0f; // room height

        // ── TOP FLOOR UNIFIED (BEDROOM + BATHROOM) ──────────────


        // ── FLOOR (Offset slightly up to 4.01 to stop ground-floor ceiling flickering) ──────
        box(-4.0f, Y + 0.01f, -15.f, 7.0f, 0.04f, 20.f, flrTop);

        // ── CEILING ───────────────────────
        // (Removed duplicate ceiling box; the global texBox texCeiling takes over to stop flickering!)

        // ── INNER WALLS (Offset slightly inward to stop Outside-View flickering) ──────
        // Back wall
        box(-3.95f, Y, -14.95f, 6.9f, RH, 0.1f, wallColor); 
        // Right wall (Solid entirely)
        box(2.95f, Y, -14.95f, 0.1f, RH, 19.9f, wallColor); 
        // Front wall
        box(-3.95f, Y, 4.95f, 7.0f, RH, 0.1f, wallColor);
        // Left wall (Split perfectly to hold the real 3-wide door opening at Z = -6.5)
        box(-3.95f, Y, -14.95f, 0.1f, RH, 8.45f, wallColor); 
        box(-3.95f, Y + 4.0f, -6.5f, 0.1f, RH - 4.0f, 3.0f, wallColor); 
        box(-3.95f, Y, -3.5f, 0.1f, RH, 8.5f, wallColor); 

        // ── DIVIDER WALL (As requested: "just akta deyal dao") ───
        // Positioned at Z = -5 separating the bedroom and bathroom, extending partially (X = -4 to 1)
        box(-4.0f, Y, -5.0f, 5.0f, RH, 0.12f, wallColor);
        // Small archway drop from the ceiling in the gap
        box(1.0f, Y + 3.8f, -5.0f, 2.0f, RH - 3.8f, 0.12f, wallColor);

        // ── BATHROOM ACCENT WALL (just one mosaic panel near tub)
        texBox(-3.9f, Y, -5.0f, 0.05f, RH, 10.0f, glm::vec3(0.60f, 0.73f, 0.68f), texMosaic);
        // box(-3.95f, Y, -15.0f, 0.08f, 0.30f, 20.0f, skirtDk); // left
        // box(2.95f, Y, -15.0f, 0.08f, 0.30f, 20.0f, skirtDk);  // right
        // box(-4.0f, Y, 4.95f, 7.0f, 0.30f, -0.08f, skirtDk);   // front

        // ── CROWN MOULDING ───────────────────────────────────────
        box(-4.0f, Y + RH - 0.2f, -14.95f, 7.0f, 0.20f, 0.10f, crownW);
        box(-3.95f, Y + RH - 0.2f, -15.0f, 0.10f, 0.20f, 20.0f, crownW);
        box(2.95f, Y + RH - 0.2f, -15.0f, 0.10f, 0.20f, 20.0f, crownW);

        // (Removed ROOF DOOR PANEL to leave wall continuous)
        // ── BEDROOM WINDOWS ──────────────────────────────────────
        // (a) Back wall high ventilator window (small; avoids ghost patch)
        {
          float wx = -1.55f, wy = Y + 3.85f, wz = -14.92f;
          box(wx, wy, wz, 1.40f, 0.55f, 0.10f, frameW);
          box(wx + 0.05f, wy + 0.04f, wz + 0.02f, 1.30f, 0.43f, 0.04f,
              glm::vec3(0.62f, 0.78f, 0.92f));
          for (int i = 0; i < 7; i++) {
            box(wx + 0.10f + i * 0.18f, wy + 0.04f, wz + 0.04f, 0.02f, 0.43f,
                0.03f, glm::vec3(0.30f, 0.20f, 0.10f));
          }
          box(wx, wy - 0.06f, wz + 0.10f, 1.60f, 0.05f, 0.05f, skirtDk);
        }
        // (b) Right wall bedroom window (x=3)
        {
          float wxr = 2.92f, wyr = Y + 1.8f, wzr = -10.0f;
          box(wxr + 0.02f, wyr, wzr, 0.04f, 1.9f, 2.2f, windowSky);
          box(wxr, wyr, wzr, 0.12f, 2.1f, 2.4f, frameW);
          box(wxr + 0.03f, wyr, wzr, 0.04f, 1.7f, 2.0f, windowSky);
          box(wxr + 0.05f, wyr, wzr, 0.05f, 0.06f, 2.0f, frameW);
          box(wxr + 0.05f, wyr, wzr, 0.05f, 1.7f, 0.06f, frameW);
          box(wxr + 0.18f, wyr + 1.2f, wzr, 0.08f, 0.07f, 3.0f, skirtDk);
          // box(wxr + 0.18f, wyr, wzr - 0.94f, 0.08f, 2.2f, 0.55f, curtPink);
          // box(wxr + 0.18f, wyr, wzr + 0.94f, 0.08f, 2.2f, 0.55f, curtPink);
        }

        // ── BATHROOM WINDOW (front wall z=5) ─────────────────────
        // {
        //   float wxb = 0.8f, wyb = Y + 1.8f, wzb = 4.92f;
        //   box(wxb, wyb, wzb + 0.02f, 2.2f, 1.9f, 0.04f, windowSky);
        //   box(wxb, wyb, wzb, 2.4f, 2.1f, -0.12f, frameW);
        //   box(wxb, wyb, wzb - 0.03f, 2.0f, 1.7f, -0.04f, windowSky);
        //   box(wxb, wyb, wzb - 0.05f, 0.06f, 1.7f, -0.05f, frameW);
        //   box(wxb, wyb, wzb - 0.05f, 2.0f, 0.06f, -0.05f, frameW);
        //   box(wxb, wyb + 1.2f, wzb - 0.18f, 3.0f, 0.07f, 0.07f, skirtDk);
        //   box(wxb - 0.94f, wyb, wzb - 0.18f, 0.55f, 2.2f, -0.08f, curtBlue);
        //   box(wxb + 0.94f, wyb, wzb - 0.18f, 0.55f, 2.2f, -0.08f, curtBlue);
        // }
        // // Right wall bathroom window (x=3)
        // {
        //   float wxr2 = 2.92f, wyr2 = Y + 1.8f, wzr2 = 1.5f;
        //   box(wxr2 + 0.02f, wyr2, wzr2, 0.04f, 1.8f, 2.0f, windowSky);
        //   box(wxr2, wyr2, wzr2, 0.12f, 2.0f, 2.2f, frameW);
        //   box(wxr2 + 0.03f, wyr2, wzr2, 0.04f, 1.6f, 1.8f, windowSky);
        // }

        // ════════════════════════════════════════════════════════
        //  BEDROOM FURNITURE  (minimal, clean, well-oriented)
        // ════════════════════════════════════════════════════════

        // ── BED – rebuilt cleanly (no detached planks) ────────────
        {
          glm::vec3 bedLeg(0.26f, 0.16f, 0.07f);
          glm::vec3 bedFrm(0.32f, 0.20f, 0.10f);
          glm::vec3 mattrs(0.94f, 0.92f, 0.90f);
          glm::vec3 duvetC(0.92f, 0.88f, 0.82f);
          glm::vec3 duvAcc(0.65f, 0.40f, 0.45f);
          glm::vec3 pillwC(0.97f, 0.96f, 0.95f);
          glm::vec3 hdBrd(0.18f, 0.22f, 0.38f);
          glm::vec3 hdBrdD(0.12f, 0.16f, 0.28f);
          glm::vec3 sheetC(0.68f, 0.80f, 0.90f);

          // Origin-anchored bed footprint
          float bx = -2.45f;   // left
          float bz = -14.75f;  // back (near headboard)
          float bw = 1.95f;    // width (x)
          float bd = 3.45f;    // length (z)

          // Legs
          box(bx, Y, bz, 0.14f, 0.30f, 0.14f, bedLeg);
          box(bx + bw - 0.14f, Y, bz, 0.14f, 0.30f, 0.14f, bedLeg);
          box(bx, Y, bz + bd - 0.14f, 0.14f, 0.30f, 0.14f, bedLeg);
          box(bx + bw - 0.14f, Y, bz + bd - 0.14f, 0.14f, 0.30f, 0.14f, bedLeg);

          // Side rails + foot rail
          box(bx, Y + 0.34f, bz, 0.14f, 0.42f, bd, bedFrm);
          box(bx + bw - 0.14f, Y + 0.34f, bz, 0.14f, 0.42f, bd, bedFrm);
          box(bx, Y + 0.38f, bz + bd - 0.12f, bw, 0.38f, 0.12f, bedLeg);

          // Slats completely inside frame
          for (int s = 0; s < 6; s++)
            box(bx + 0.14f, Y + 0.22f, bz + 0.26f + s * 0.50f, bw - 0.28f, 0.05f,
                0.09f, bedLeg);

          // Headboard at back wall side
          box(bx, Y + 0.40f, bz - 0.20f, bw, 1.88f, 0.20f, hdBrd);
          for (int r = 0; r < 3; r++)
            for (int c = 0; c < 3; c++)
              box(bx + 0.22f + c * 0.50f, Y + 0.52f + r * 0.48f, bz - 0.24f, 0.08f,
                  0.08f, 0.05f, hdBrdD);

          // Mattress + bedding (single-level look, same footprint)
          box(bx + 0.08f, Y + 0.60f, bz + 0.06f, bw - 0.16f, 0.30f, bd - 0.12f,
              mattrs);
          box(bx + 0.08f, Y + 0.88f, bz + 0.06f, bw - 0.16f, 0.05f, bd - 0.12f,
              duvetC);
          box(bx + 0.08f, Y + 0.92f, bz + 0.06f, bw - 0.16f, 0.02f, bd - 0.12f,
              sheetC);

          // box(bx + 0.36f, Y + 0.96f, bz + 0.10f, 0.20f, 0.06f, 0.22f, pillwC);
          // box(bx + 1.18f, Y + 0.96f, bz + 0.10f, 0.20f, 0.06f, 0.22f, pillwC);
        }

        // ── NIGHTSTAND – right of bed against right wall ──────────
        {
          glm::vec3 nW(0.32f, 0.22f, 0.11f);
          float nx2 = 1.2f, nz2 = -13.5f; // moved right with bed
          // Body with 2 small drawers
          box(nx2, Y + 0.0f, nz2, 0.65f, 1.05f, 0.60f, nW);
          box(nx2 + 0.05f, Y + 0.62f, nz2 + 0.55f, 0.55f, 0.36f, 0.05f,
              glm::vec3(0.38f, 0.26f, 0.13f)); // drawer 1
          box(nx2 + 0.05f, Y + 0.25f, nz2 + 0.55f, 0.55f, 0.32f, 0.05f,
              glm::vec3(0.38f, 0.26f, 0.13f)); // drawer 2
          box(nx2 + 0.24f, Y + 0.78f, nz2 + 0.58f, 0.14f, 0.06f, 0.04f,
              glm::vec3(0.75f, 0.60f, 0.32f)); // handle 1
          box(nx2 + 0.24f, Y + 0.41f, nz2 + 0.58f, 0.14f, 0.06f, 0.04f,
              glm::vec3(0.75f, 0.60f, 0.32f)); // handle 2
          // Table lamp on top: base, pole, shade (INTERACTIVE – key 0)
          box(nx2 + 0.22f, Y + 1.06f, nz2 + 0.22f, 0.20f, 0.06f, 0.20f,
              glm::vec3(0.55f, 0.45f, 0.30f)); // base
          box(nx2 + 0.30f, Y + 1.12f, nz2 + 0.30f, 0.05f, 0.28f, 0.05f,
              glm::vec3(0.55f, 0.45f, 0.30f)); // pole

          if (tableLampOn) {
            // Glowing lamp shade (warm emissive)
            lightingShader.use();
            lightingShader.setVec3("material.ambient", glm::vec3(1.0f, 0.92f, 0.60f));
            lightingShader.setVec3("material.diffuse", glm::vec3(1.0f, 0.95f, 0.70f));
            lightingShader.setVec3("material.specular", glm::vec3(0.8f));
            lightingShader.setFloat("material.shininess", 64.0f);
            lightingShader.setVec3("material.emissive", glm::vec3(0.95f, 0.85f, 0.45f));
            glm::mat4 shadeM = glm::translate(I, glm::vec3(nx2 + 0.12f, Y + 1.38f, nz2 + 0.12f));
            shadeM = glm::scale(shadeM, glm::vec3(0.40f, 0.26f, 0.40f));
            lightingShader.setMat4("model", shadeM);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            // Reset emissive
            lightingShader.setVec3("material.emissive", glm::vec3(0.0f));

            // Bright bulb inside (small glowing sphere)
            Sphere lampBulb2(0.5f, 12, 8,
                glm::vec3(1.0f, 0.97f, 0.82f),
                glm::vec3(1.0f, 0.97f, 0.82f),
                glm::vec3(1.0f), 128.0f);
            lightingShader.setVec3("material.emissive", glm::vec3(1.0f, 0.90f, 0.50f));
            glm::mat4 bulbM = glm::translate(I, glm::vec3(nx2 + 0.32f, Y + 1.42f, nz2 + 0.32f));
            bulbM = glm::scale(bulbM, glm::vec3(0.12f));
            lampBulb2.drawSphere(lightingShader, bulbM);
            lightingShader.setVec3("material.emissive", glm::vec3(0.0f));

            // Warm light pool on nightstand surface
            box(nx2 + 0.05f, Y + 1.055f, nz2 + 0.05f, 0.55f, 0.01f, 0.50f,
                glm::vec3(1.0f, 0.92f, 0.60f));
          } else {
            // Dim/off lamp shade
            box(nx2 + 0.12f, Y + 1.38f, nz2 + 0.12f, 0.40f, 0.26f, 0.40f,
                glm::vec3(0.75f, 0.68f, 0.45f));
          }
          box(nx2 + 0.22f, Y + 1.30f, nz2 + 0.22f, 0.20f, 0.10f, 0.20f,
              tableLampOn ? glm::vec3(1.f, 0.97f, 0.82f) : glm::vec3(0.60f, 0.55f, 0.42f));
        }

        // ── STUDY TABLE + CHAIR – facing the right wall ───────────────
        {
          glm::vec3 tblW(0.70f, 0.52f, 0.30f);
          glm::vec3 tblD(0.60f, 0.44f, 0.22f);
          glm::vec3 legW2(0.50f, 0.36f, 0.18f);
          glm::vec3 mirF(0.30f, 0.20f, 0.08f);
          glm::vec3 mirG(0.80f, 0.90f, 0.96f);
          
          // Table positioned flush against the right wall (X=3.0)
          float txW = 2.0f;     // Left edge of table
          float tzW = -11.5f;   // Back edge along Z
          float tDepth = 0.85f; // Along X
          float tWidth = 1.85f; // Along Z
          
          // Tabletop (facing right wall)
          box(txW, Y + 1.50f, tzW, tDepth, 0.08f, tWidth, tblW);
          
          // Legs (Left side, towards room)
          box(txW + 0.08f, Y + 0.0f, tzW + 0.08f, 0.08f, 1.50f, 0.08f, legW2);
          box(txW + 0.08f, Y + 0.0f, tzW + tWidth - 0.16f, 0.08f, 1.50f, 0.08f, legW2);
          
          // Right drawer cabinet (Against the wall)
          box(txW + tDepth - 0.45f, Y + 0.0f, tzW, 0.45f, 1.50f, tWidth, tblD);
          for (int d = 0; d < 3; d++) {
            box(txW + 0.35f, Y + 0.15f + d * 0.44f, tzW + 0.10f, 0.06f, 0.35f, 0.40f, glm::vec3(0.55f, 0.40f, 0.20f));
            box(txW + 0.32f, Y + 0.32f + d * 0.44f, tzW + 0.26f, 0.04f, 0.06f, 0.12f, glm::vec3(0.75f, 0.60f, 0.32f));
          }

          // Monitor flush on right wall behind desk
          box(txW + tDepth - 0.10f, Y + 1.80f, tzW + 0.20f, 0.10f, 1.30f, 1.40f, mirF);
          mirrorBox(txW + tDepth - 0.13f, Y + 1.86f, tzW + 0.26f, 0.05f, 1.14f, 1.28f, mirG);

          // Books on desk (aligned right)
          float bcols[4][3] = {{0.72f, 0.18f, 0.18f}, {0.18f, 0.38f, 0.72f}, {0.20f, 0.60f, 0.28f}, {0.80f, 0.68f, 0.12f}};
          for (int b2 = 0; b2 < 4; b2++)
            box(txW + 0.10f, Y + 1.60f, tzW + 0.10f + b2 * 0.20f, 0.08f, 0.42f, 0.18f, glm::vec3(bcols[b2][0], bcols[b2][1], bcols[b2][2]));

          // Desk lamp
          box(txW + tDepth - 0.25f, Y + 1.58f, tzW + tWidth - 0.30f, 0.14f, 0.06f, 0.14f, glm::vec3(0.88f, 0.72f, 0.25f));
          box(txW + tDepth - 0.20f, Y + 1.64f, tzW + tWidth - 0.25f, 0.04f, 0.32f, 0.04f, glm::vec3(0.88f, 0.72f, 0.25f));
          box(txW + tDepth - 0.38f, Y + 1.92f, tzW + tWidth - 0.40f, 0.35f, 0.20f, 0.38f, glm::vec3(0.92f, 0.82f, 0.40f));

          // ── Clean modern Chair (Pulled back, perfectly oriented) ──
          float chX = txW - 0.70f;  // Pulled comfortably back from table
          float chZ = tzW + 0.80f;  // Centered along desk Z
          glm::vec3 chrCol(0.18f, 0.38f, 0.55f);
          glm::vec3 chrMetal(0.52f, 0.52f, 0.55f);

          // Central metal pole & wheelbase star
          box(chX + 0.30f, Y + 0.05f, chZ + 0.30f, 0.10f, 0.80f, 0.10f, chrMetal);
          box(chX + 0.0f, Y + 0.05f, chZ + 0.32f, 0.70f, 0.05f, 0.06f, chrMetal);
          box(chX + 0.32f, Y + 0.05f, chZ + 0.0f, 0.06f, 0.05f, 0.70f, chrMetal);
          // Seat
          box(chX, Y + 0.85f, chZ, 0.70f, 0.10f, 0.70f, chrCol);
          // Backrest (facing table, on the left of seat)
          box(chX - 0.05f, Y + 0.85f, chZ, 0.10f, 1.00f, 0.70f, chrCol);
          // Arm rests
          box(chX + 0.05f, Y + 0.95f, chZ, 0.40f, 0.40f, 0.05f, chrMetal);
          box(chX + 0.05f, Y + 0.95f, chZ + 0.65f, 0.40f, 0.40f, 0.05f, chrMetal);
          box(chX + 0.05f, Y + 1.35f, chZ, 0.40f, 0.05f, 0.05f, glm::vec3(0.1f));
          box(chX + 0.05f, Y + 1.35f, chZ + 0.65f, 0.40f, 0.05f, 0.05f, glm::vec3(0.1f));
        }

        // ── WARDROBE – flush against left wall ────────────────────
        {
          glm::vec3 wdCol(0.28f, 0.18f, 0.08f);
          glm::vec3 wdDoor(0.36f, 0.24f, 0.12f);
          glm::vec3 wdHnd(0.75f, 0.60f, 0.30f);
          // Left wall at x=-4; wardrobe body: x=-4..-3.52 (0.48 deep)
          float wdx = -4.0f, wdz = -12.5f;
          // Main body
          box(wdx, Y + 0.0f, wdz - 1.5f, 0.48f, 5.0f, 3.0f, wdCol);
          // Left door panel
          box(wdx + 0.44f, Y + 0.05f, wdz - 1.45f, 0.05f, 4.90f, 1.40f, wdDoor);
          // Right door panel
          box(wdx + 0.44f, Y + 0.05f, wdz + 0.05f, 0.05f, 4.90f, 1.40f, wdDoor);
          // Door handles
          box(wdx + 0.48f, Y + 2.40f, wdz - 0.15f, 0.04f, 0.22f, 0.05f, wdHnd);
          box(wdx + 0.48f, Y + 2.40f, wdz - 1.30f, 0.04f, 0.22f, 0.05f, wdHnd);
          // Top cornice
          box(wdx - 0.02f, Y + 5.02f, wdz - 1.52f, 0.52f, 0.18f, 3.04f,
              glm::vec3(0.22f, 0.13f, 0.06f));
        }

        // ── CEIL FAN (bedroom center) ────────────────────────────
        {
          float fanX2 = -0.5f, fanZ2 = -11.0f,
                fanY2 = Y + RH - 0.62f; // centered over room
          float tFan = fanOn ? (float)glfwGetTime() : 0.f;
          // Drop rod
          box(fanX2 - 0.03f, fanY2 + 0.12f, fanZ2 - 0.03f, 0.06f, 0.52f, 0.06f,
              glm::vec3(0.48f, 0.48f, 0.50f));
          // Hub
          box(fanX2 - 0.18f, fanY2 - 0.04f, fanZ2 - 0.18f, 0.36f, 0.18f, 0.36f,
              glm::vec3(0.50f, 0.50f, 0.52f));
          // Blades (4)
          for (int fb = 0; fb < 4; fb++) {
            glm::mat4 bld =
                glm::translate(I, glm::vec3(fanX2, fanY2 + 0.06f, fanZ2));
            bld = glm::rotate(bld, glm::radians(tFan * 100.f + fb * 90.f),
                              glm::vec3(0, 1, 0));
            bld = glm::translate(bld, glm::vec3(0.55f, 0, 0));
            bld = glm::scale(bld, glm::vec3(1.5f, 0.04f, 0.38f));
            drawCube(VAO, lightingShader, bld, glm::vec3(0.52f, 0.40f, 0.24f));
          }
          // Lamp globe
          box(fanX2 - 0.14f, fanY2 - 0.18f, fanZ2 - 0.14f, 0.28f, 0.20f, 0.28f,
              glm::vec3(1.f, 0.97f, 0.82f));
        }

        // ── RUG ──────────────────────────────────────────────────
        box(-3.5f, Y + 0.04f, -13.5f, 2.0f, 0.04f, 3.0f,
            glm::vec3(0.40f, 0.08f, 0.12f));
        box(-3.35f, Y + 0.05f, -13.35f, 1.70f, 0.04f, 2.70f,
            glm::vec3(0.62f, 0.48f, 0.22f));
        box(-3.20f, Y + 0.06f, -13.20f, 1.40f, 0.04f, 2.40f,
            glm::vec3(0.38f, 0.07f, 0.10f));

        // Back-wall art removed to keep window/back-wall clean and readable.

        // ── CORNER PLANT – pot + fractal-style multi-layer canopy ─
        {
          float px = -3.55f, pz = -14.5f;
          // Terracotta pot
          box(px - 0.20f, Y + 0.0f, pz - 0.20f, 0.40f, 0.45f, 0.40f,
              glm::vec3(0.72f, 0.38f, 0.20f));
          box(px - 0.22f, Y + 0.40f, pz - 0.22f, 0.44f, 0.06f, 0.44f,
              glm::vec3(0.62f, 0.30f, 0.14f));
          // Thin trunk
          box(px - 0.03f, Y + 0.46f, pz - 0.03f, 0.06f, 0.90f, 0.06f,
              glm::vec3(0.30f, 0.20f, 0.08f));
          // Layer 1 – wide base tuft
          glm::vec3 g1(0.16f, 0.48f, 0.20f), g2(0.22f, 0.58f, 0.26f),
              g3(0.30f, 0.68f, 0.32f);
          box(px - 0.36f, Y + 1.20f, pz - 0.36f, 0.72f, 0.28f, 0.72f, g1);
          // Layer 2 – mid
          box(px - 0.26f, Y + 1.44f, pz - 0.26f, 0.52f, 0.26f, 0.52f, g2);
          // Layer 3 – narrower mid
          box(px - 0.18f, Y + 1.66f, pz - 0.18f, 0.36f, 0.24f, 0.36f, g1);
          // Layer 4 – small upper puff
          box(px - 0.12f, Y + 1.86f, pz - 0.12f, 0.24f, 0.22f, 0.24f, g3);
          // Branch offsets (asymmetric side tufts)
          box(px + 0.20f, Y + 1.35f, pz - 0.08f, 0.20f, 0.18f, 0.20f, g2);
          box(px - 0.36f, Y + 1.50f, pz + 0.16f, 0.20f, 0.16f, 0.20f, g1);
          // Top bud
          box(px - 0.05f, Y + 2.06f, pz - 0.05f, 0.10f, 0.14f, 0.10f, g3);
        }

        // ════════════════════════════════════════════════════════
        //  BATHROOM FIXTURES  (properly positioned)
        // ════════════════════════════════════════════════════════

        // ── BATHTUB – SLEEK RECTANGULAR FREESTANDING ──
        {
          float btx = -1.50f, btz = -2.50f; // Center of bathroom space
          float bl = 3.60f; // Length (Z)
          float bw = 1.60f; // Width (X)
          float bh = 0.80f; // Height
          float bThk = 0.15f; // Wall thickness
          glm::vec3 tubCol(0.98f, 0.99f, 1.0f); // Bright pure white tub
          
          lightingShader.use();
          // Five boxes to build a perfect modern hollow tub
          box(btx, Y, btz, bw, 0.10f, bl, tubCol); // Floor
          box(btx, Y + 0.10f, btz, bThk, bh, bl, tubCol); // Left wall
          box(btx + bw - bThk, Y + 0.10f, btz, bThk, bh, bl, tubCol); // Right wall
          box(btx + bThk, Y + 0.10f, btz, bw - 2*bThk, bh, bThk, tubCol); // Back wall
          box(btx + bThk, Y + 0.10f, btz + bl - bThk, bw - 2*bThk, bh, bThk, tubCol); // Front wall

          // ── WATER RECTANGLE SURFACE ──
          float waterLevel = tapOn ? bh - 0.15f : 0.40f;
          float splashLevel = Y + waterLevel + 0.05f;
          glm::vec3 waterCol = tapOn ? glm::vec3(0.50f, 0.75f, 0.90f) : glm::vec3(0.60f, 0.82f, 0.92f);
          box(btx + bThk + 0.02f, Y + waterLevel, btz + bThk + 0.02f, bw - 2*bThk - 0.04f, 0.03f, bl - 2*bThk - 0.04f, waterCol);

          // ── FLOOR STANDING TALL FAUCET (Moved closer to tub) ──
          float tapX = btx - 0.20f;
          float tapZ = btz + bl/2.0f;
          // Base pole
          box(tapX, Y, tapZ, 0.10f, 1.40f, 0.10f, fixtureC);
          // Horizontal spout reaching into tub
          box(tapX + 0.10f, Y + 1.30f, tapZ + 0.02f, 0.65f, 0.06f, 0.06f, fixtureC);
          // Spout tip pointing down
          box(tapX + 0.69f, Y + 1.20f, tapZ + 0.02f, 0.06f, 0.12f, 0.06f, fixtureC);

          // Knobs
          glm::vec3 hotCol = tapOn ? glm::vec3(0.95f, 0.20f, 0.15f) : glm::vec3(0.70f, 0.15f, 0.12f);
          box(tapX + 0.05f, Y + 1.00f, tapZ - 0.15f, 0.06f, 0.06f, 0.10f, hotCol);
          glm::vec3 coldCol = tapOn ? glm::vec3(0.15f, 0.40f, 0.95f) : glm::vec3(0.12f, 0.25f, 0.60f);
          box(tapX + 0.05f, Y + 1.00f, tapZ + 0.15f, 0.06f, 0.06f, 0.10f, coldCol);

          // ── ANIMATED WATER STREAM ──
          if (tapOn) {
            float waterTime = (float)glfwGetTime();
            glm::vec3 streamCol(0.40f, 0.72f, 0.92f);
            float spoutX = tapX + 0.72f; // Aligned perfectly under the spout tip
            float spoutZ = tapZ + 0.05f;
            float spoutY = Y + 1.20f;
            int numDrops = 10;
            for (int d = 0; d < numDrops; d++) {
              float phase = waterTime * 8.0f - d * 0.5f;
              float t = fmod(phase, 3.14159f) / 3.14159f;
              float dropY = spoutY - t * (spoutY - splashLevel);
              float wobbleX = sinf(phase * 2.3f) * 0.01f;
              float wobbleZ = cosf(phase * 1.7f) * 0.01f;
              float dropSize = 0.03f + 0.02f * (1.0f - t);
              box(spoutX + wobbleX - dropSize * 0.5f, dropY,
                  spoutZ + wobbleZ - dropSize * 0.5f,
                  dropSize, dropSize * 1.5f, dropSize, streamCol);
            }
            // Splash effect squarely in the tub
            for (int sp = 0; sp < 6; sp++) {
              float splashA = waterTime * 4.f + sp * 1.047f;
              float splR = 0.06f + 0.04f * sinf(splashA * 2.f);
              float sx = spoutX + cosf(splashA) * splR;
              float sz = spoutZ + sinf(splashA) * splR;
              float sy = splashLevel + fabsf(sinf(splashA * 3.f)) * 0.04f;
              box(sx - 0.01f, sy, sz - 0.01f, 0.02f, 0.02f, 0.02f, streamCol);
            }
            // Ripple rings
            for (int r = 0; r < 3; r++) {
              float ripR = 0.08f + fmod(waterTime * 0.3f + r * 0.15f, 0.30f);
              float ripAlpha = 1.0f - ripR / 0.35f;
              glm::vec3 ripCol = glm::vec3(0.60f, 0.85f, 0.98f) * ripAlpha;
              box(spoutX + ripR - 0.01f, splashLevel + 0.01f, spoutZ - 0.01f, 0.02f, 0.005f, 0.02f, ripCol);
              box(spoutX - ripR - 0.01f, splashLevel + 0.01f, spoutZ - 0.01f, 0.02f, 0.005f, 0.02f, ripCol);
              box(spoutX - 0.01f, splashLevel + 0.01f, spoutZ + ripR - 0.01f, 0.02f, 0.005f, 0.02f, ripCol);
              box(spoutX - 0.01f, splashLevel + 0.01f, spoutZ - ripR - 0.01f, 0.02f, 0.005f, 0.02f, ripCol);
            }
          }
        }

        // ── COMMODE / TOILET – left wall between tub and basin (Scaled Up) ──
        {
          float ctx = -3.70f, ctz = 1.20f; // Along left wall, facing +X
          // Base pedestal
          box(ctx + 0.15f, Y + 0.0f, ctz + 0.10f, 0.60f, 0.16f, 0.48f, commodeC);
          // Bowl body
          box(ctx + 0.18f, Y + 0.16f, ctz + 0.05f, 0.50f, 0.34f, 0.60f, commodeC);
          // Bowl top ring
          box(ctx + 0.16f, Y + 0.50f, ctz + 0.02f, 0.58f, 0.08f, 0.68f, commodeC);
          // Bowl inner
          box(ctx + 0.22f, Y + 0.44f, ctz + 0.08f, 0.44f, 0.14f, 0.54f, glm::vec3(0.90f, 0.92f, 0.96f));
          // Seat ring
          box(ctx + 0.16f, Y + 0.58f, ctz + 0.02f, 0.58f, 0.04f, 0.68f, glm::vec3(0.92f, 0.92f, 0.92f));
          // Lid
          box(ctx + 0.15f, Y + 0.62f, ctz + 0.00f, 0.60f, 0.05f, 0.72f, commodeC);
          // Cistern / tank (attached to left wall)
          box(ctx, Y + 0.16f, ctz + 0.06f, 0.28f, 0.90f, 0.58f, commodeC);
          // Cistern top lid
          box(ctx - 0.02f, Y + 1.06f, ctz + 0.04f, 0.32f, 0.06f, 0.62f, glm::vec3(0.94f, 0.94f, 0.94f));
          // Flush button
          box(ctx + 0.30f, Y + 1.12f, ctz + 0.26f, 0.06f, 0.08f, 0.20f, fixtureC);
          // TP holder on left wall slightly forward
          box(ctx, Y + 1.10f, ctz + 1.00f, 0.10f, 0.06f, 0.26f, fixtureC);
          box(ctx + 0.02f, Y + 1.12f, ctz + 1.02f, 0.06f, 0.24f, 0.22f, glm::vec3(0.96f));
        }

        // ── WASHBASIN – SPHERE-BASED (rotated to face inwards) ────
        {
          float bsx = -1.50f, bsz = 3.20f; // front-left area
          lightingShader.use();

          // Elegant pedestal using Bezier vase shape
          glm::mat4 pedM = glm::translate(I, glm::vec3(bsx, Y + 0.0f, bsz));
          pedM = glm::scale(pedM, glm::vec3(0.35f, 1.70f, 0.35f));
          bezVase.draw(lightingShader, pedM, glm::vec3(0.92f, 0.93f, 0.94f));

          // Counter-top slab
          box(bsx - 0.75f, Y + 1.68f, bsz - 0.50f, 1.50f, 0.14f, 1.00f, basinC);

          // ── SPHERE BASIN BOWL ──
          Sphere basinBowl(0.5f, 28, 16,
              glm::vec3(0.90f, 0.92f, 0.96f), glm::vec3(0.92f, 0.94f, 0.98f), glm::vec3(0.6f), 64.0f);
          glm::mat4 bowlM = glm::translate(I, glm::vec3(bsx, Y + 1.72f, bsz));
          bowlM = glm::scale(bowlM, glm::vec3(0.70f, 0.32f, 0.55f));
          basinBowl.drawSphere(lightingShader, bowlM);

          Sphere basinInner(0.5f, 24, 14,
              glm::vec3(0.82f, 0.90f, 0.96f), glm::vec3(0.85f, 0.92f, 0.98f), glm::vec3(0.4f), 32.0f);
          glm::mat4 innerM = glm::translate(I, glm::vec3(bsx, Y + 1.70f, bsz));
          innerM = glm::scale(innerM, glm::vec3(0.62f, 0.26f, 0.48f));
          basinInner.drawSphere(lightingShader, innerM);

          // Basin rim ring
          box(bsx - 0.38f, Y + 1.82f, bsz - 0.30f, 0.76f, 0.03f, 0.60f, glm::vec3(0.97f, 0.98f, 0.99f));

          // ── Elegant curved faucet (Opposite side) ──
          // Riser pipe (behind basin, closer to wall +Z)
          box(bsx - 0.04f, Y + 1.84f, bsz + 0.42f, 0.08f, 0.38f, 0.06f, fixtureC);
          // Gooseneck curve
          box(bsx - 0.06f, Y + 2.20f, bsz + 0.36f, 0.12f, 0.06f, 0.08f, fixtureC);
          box(bsx - 0.04f, Y + 2.16f, bsz + 0.18f, 0.08f, 0.06f, 0.18f, fixtureC);
          box(bsx - 0.03f, Y + 2.10f, bsz + 0.16f, 0.06f, 0.08f, 0.06f, fixtureC);
          // Spout tip
          box(bsx - 0.02f, Y + 2.02f, bsz + 0.14f, 0.04f, 0.10f, 0.04f, fixtureC);

          // H/C knobs
          box(bsx - 0.30f, Y + 1.86f, bsz + 0.40f, 0.12f, 0.08f, 0.06f, glm::vec3(0.88f, 0.14f, 0.14f)); // hot
          box(bsx + 0.18f, Y + 1.86f, bsz + 0.40f, 0.12f, 0.08f, 0.06f, glm::vec3(0.14f, 0.34f, 0.88f)); // cold

          // Mirror above basin on wall
          box(bsx - 0.60f, Y + 2.30f, bsz + 0.40f, 1.20f, 1.40f, 0.10f, glm::vec3(0.20f, 0.12f, 0.04f));
          mirrorBox(bsx - 0.52f, Y + 2.38f, bsz + 0.38f, 1.04f, 1.24f, 0.05f, glm::vec3(0.78f, 0.90f, 0.96f));

          // Soap dispenser
          box(bsx + 0.55f, Y + 1.84f, bsz + 0.04f, 0.14f, 0.22f, 0.14f, glm::vec3(0.88f, 0.92f, 0.88f));
          Sphere soapTop(0.5f, 12, 8, glm::vec3(0.80f), glm::vec3(0.82f), glm::vec3(0.3f), 16.0f);
          glm::mat4 stM = glm::translate(I, glm::vec3(bsx + 0.62f, Y + 2.08f, bsz + 0.11f));
          stM = glm::scale(stM, glm::vec3(0.10f, 0.08f, 0.10f));
          soapTop.drawSphere(lightingShader, stM);
          box(bsx + 0.60f, Y + 2.10f, bsz + 0.08f, 0.04f, 0.10f, 0.04f, fixtureC);

          // ── BASIN WATER ANIMATION ──
          if (tapOn) {
            float waterTime = (float)glfwGetTime();
            glm::vec3 streamCol(0.40f, 0.72f, 0.92f);
            float spoutX = bsx;
            float spoutZ = bsz + 0.16f;
            float spoutY = Y + 2.02f;
            float landY = Y + 1.60f; // bottom of basin
            for (int d = 0; d < 8; d++) {
              float phase = waterTime * 8.0f - d * 0.5f;
              float t = fmod(phase, 3.14159f) / 3.14159f;
              float dropY = spoutY - t * (spoutY - landY);
              float dropSize = 0.02f + 0.015f * (1.0f - t);
              box(spoutX - dropSize * 0.5f, dropY, spoutZ - dropSize * 0.5f, dropSize, dropSize * 1.5f, dropSize, streamCol);
            }
            // Small water pool in basin
            box(bsx - 0.2f, landY, bsz - 0.2f, 0.4f, 0.02f, 0.4f, glm::vec3(0.6f, 0.8f, 0.9f));
          }
        }

        // ── TOWEL RACK – right wall ───────────────────────────────
        box(2.92f, Y + 2.10f, -0.80f, 0.08f, 0.06f, 1.50f, fixtureC);
        box(2.92f, Y + 2.16f, -0.80f, 0.06f, 0.38f, 0.56f,
            glm::vec3(0.82f, 0.22f, 0.28f));
        box(2.92f, Y + 2.16f, -0.10f, 0.06f, 0.32f, 0.52f,
            glm::vec3(0.22f, 0.50f, 0.82f));

        // ── SMALL MEDICINE CABINET above toilet ───────────────────
        box(2.88f, Y + 2.50f, 3.10f, 0.10f, 1.10f, 1.00f,
            glm::vec3(0.84f, 0.85f, 0.87f));
        box(2.90f, Y + 2.56f, 3.16f, 0.06f, 0.98f, 0.86f,
            glm::vec3(0.78f, 0.90f, 0.96f));

        // ════════════════════════════════════════════════════════
        //  BALCONY – DOLNA + ROCKING CHAIR ONLY
        // ════════════════════════════════════════════════════════

        // ── DOLNA / PORCH SWING – High Precision Assembly ──────────
        {
          glm::vec3 swWood(0.48f, 0.30f, 0.12f);
          glm::vec3 swRope(0.72f, 0.62f, 0.42f);
          glm::vec3 swSeat(0.25f, 0.45f, 0.68f);
          glm::vec3 swMet(0.55f, 0.55f, 0.58f);
          
          glm::mat4 baseSw = glm::translate(I, glm::vec3(-6.2f, 4.05f, 2.0f));
          // Rotate -90 degrees around Y to face the entrance gate (-X direction) instead of +Z
          baseSw = glm::rotate(baseSw, glm::radians(-90.0f), glm::vec3(0, 1, 0));

          float swx = 0.0f, swy = 0.0f, swz = 0.0f;
          float legH = 2.80f, legW = 0.10f;
          float dx = 0.57f, dz = 0.36f; // Calculated for 11.8 and 7.5 deg tilts

          auto localBox = [&](glm::mat4 mat, float x, float y, float z, float sx, float sy, float sz, glm::vec3 col) {
            float px = sx < 0 ? x + sx : x;
            float py = sy < 0 ? y + sy : y;
            float pz = sz < 0 ? z + sz : z;
            glm::mat4 m = glm::translate(mat, glm::vec3(px, py, pz));
            m = glm::scale(m, glm::vec3(fabs(sx), fabs(sy), fabs(sz)));
            drawCube(VAO, lightingShader, m, col);
          };

          // LEFT A-FRAME
          glm::mat4 LF = glm::translate(baseSw, glm::vec3(swx - dx, swy, swz - dz));
          LF = glm::rotate(LF, glm::radians(11.8f), glm::vec3(0, 0, 1));
          LF = glm::rotate(LF, glm::radians(7.5f), glm::vec3(1, 0, 0));
          LF = glm::scale(LF, glm::vec3(legW, legH, legW));
          drawCube(VAO, lightingShader, LF, swWood);
          
          glm::mat4 LB = glm::translate(baseSw, glm::vec3(swx - dx, swy, swz + dz));
          LB = glm::rotate(LB, glm::radians(11.8f), glm::vec3(0, 0, 1));
          LB = glm::rotate(LB, glm::radians(-7.5f), glm::vec3(1, 0, 0));
          LB = glm::scale(LB, glm::vec3(legW, legH, legW));
          drawCube(VAO, lightingShader, LB, swWood);

          // RIGHT A-FRAME
          glm::mat4 RF = glm::translate(baseSw, glm::vec3(swx + dx, swy, swz - dz));
          RF = glm::rotate(RF, glm::radians(-11.8f), glm::vec3(0, 0, 1));
          RF = glm::rotate(RF, glm::radians(7.5f), glm::vec3(1, 0, 0));
          RF = glm::scale(RF, glm::vec3(legW, legH, legW));
          drawCube(VAO, lightingShader, RF, swWood);
          
          glm::mat4 RB = glm::translate(baseSw, glm::vec3(swx + dx, swy, swz + dz));
          RB = glm::rotate(RB, glm::radians(-11.8f), glm::vec3(0, 0, 1));
          RB = glm::rotate(RB, glm::radians(-7.5f), glm::vec3(1, 0, 0));
          RB = glm::scale(RB, glm::vec3(legW, legH, legW));
          drawCube(VAO, lightingShader, RB, swWood);

          // Cross-braces
          localBox(baseSw, swx - 0.55f, swy + 1.20f, swz - 0.35f, 0.10f, 0.08f, 0.70f, swWood);
          localBox(baseSw, swx + 0.45f, swy + 1.20f, swz - 0.35f, 0.10f, 0.08f, 0.70f, swWood);

          // Top cross-bar strictly at leg apex (y + 2.72)
          localBox(baseSw, swx - 1.20f, swy + 2.68f, swz - 0.07f, 2.40f, 0.14f, 0.14f, swWood);
          // Joint caps perfectly capping the apex
          localBox(baseSw, swx - 0.12f + dx - 0.57f, swy + 2.70f, swz - 0.10f, 0.24f, 0.18f, 0.20f, swMet);
          localBox(baseSw, swx - 0.12f, swy + 2.74f, swz - 0.08f, 0.24f, 0.12f, 0.16f, swMet);
          localBox(baseSw, swx - 0.12f - dx + 0.57f, swy + 2.70f, swz - 0.10f, 0.24f, 0.18f, 0.20f, swMet);

          // Rope hangers
          float ropeY = swy + 2.68f, seatY = swy + 0.70f, ropeLen = ropeY - seatY;
          localBox(baseSw, swx - 0.50f, seatY, swz - 0.30f, 0.04f, ropeLen, 0.04f, swRope);
          localBox(baseSw, swx - 0.50f, seatY, swz + 0.26f, 0.04f, ropeLen, 0.04f, swRope);
          localBox(baseSw, swx + 0.46f, seatY, swz - 0.30f, 0.04f, ropeLen, 0.04f, swRope);
          localBox(baseSw, swx + 0.46f, seatY, swz + 0.26f, 0.04f, ropeLen, 0.04f, swRope);

          // Swing seat frame
          localBox(baseSw, swx - 0.60f, seatY, swz - 0.34f, 1.20f, 0.10f, 0.68f, swWood);
          localBox(baseSw, swx - 0.56f, seatY + 0.10f, swz - 0.30f, 1.12f, 0.08f, 0.60f, swSeat);
          localBox(baseSw, swx - 0.58f, seatY + 0.10f, swz - 0.34f, 1.16f, 0.70f, 0.08f, swSeat);
          localBox(baseSw, swx - 0.60f, seatY + 0.10f, swz - 0.34f, 0.10f, 0.18f, 0.68f, swWood);
          localBox(baseSw, swx + 0.50f, seatY + 0.10f, swz - 0.34f, 0.10f, 0.18f, 0.68f, swWood);
        }

        // ── ROCKING CHAIR – next to dolna (Facing -X) ────────────
        {
          glm::vec3 rcW(0.52f, 0.34f, 0.15f);
          glm::vec3 rcSeat(0.35f, 0.55f, 0.72f);
          
          glm::mat4 baseRc = glm::translate(I, glm::vec3(-6.5f, 4.05f, -0.5f));
          // Rotate -90 degrees around Y. Its local +Z now points to world -X (gate direction).
          baseRc = glm::rotate(baseRc, glm::radians(-90.0f), glm::vec3(0, 1, 0));

          float rx = 0.0f, rz = 0.0f, ry = 0.0f;

          auto localBox = [&](glm::mat4 mat, float x, float y, float z, float sx, float sy, float sz, glm::vec3 col) {
            float px = sx < 0 ? x + sx : x;
            float py = sy < 0 ? y + sy : y;
            float pz = sz < 0 ? z + sz : z;
            glm::mat4 m = glm::translate(mat, glm::vec3(px, py, pz));
            m = glm::scale(m, glm::vec3(fabs(sx), fabs(sy), fabs(sz)));
            drawCube(VAO, lightingShader, m, col);
          };

          // Rockers
          for (int side = 0; side < 2; side++) {
            float sz2 = (side == 0) ? rz - 0.28f : rz + 0.48f;
            localBox(baseRc, rx - 0.52f, ry + 0.04f, sz2, 0.42f, 0.06f, 0.10f, rcW);
            localBox(baseRc, rx - 0.12f, ry + 0.00f, sz2, 0.45f, 0.06f, 0.10f, rcW);
            localBox(baseRc, rx + 0.32f, ry + 0.04f, sz2, 0.42f, 0.06f, 0.10f, rcW);
          }
          // Four legs (ensuring they touch rockers)
          localBox(baseRc, rx - 0.42f, ry + 0.06f, rz - 0.24f, 0.07f, 0.94f, 0.07f, rcW);
          localBox(baseRc, rx + 0.45f, ry + 0.06f, rz - 0.24f, 0.07f, 0.94f, 0.07f, rcW);
          localBox(baseRc, rx - 0.42f, ry + 0.06f, rz + 0.44f, 0.07f, 0.94f, 0.07f, rcW);
          localBox(baseRc, rx + 0.45f, ry + 0.06f, rz + 0.44f, 0.07f, 0.94f, 0.07f, rcW);
          // Seat
          localBox(baseRc, rx - 0.48f, ry + 1.00f, rz - 0.28f, 1.00f, 0.09f, 0.78f, rcSeat);
          // Backrest (joined to seat)
          for (int sl2 = 0; sl2 < 3; sl2++) {
            glm::mat4 br = glm::translate(baseRc, glm::vec3(rx - 0.32f + sl2 * 0.32f, ry + 1.09f, rz - 0.18f));
            br = glm::rotate(br, glm::radians(-12.f), glm::vec3(1, 0, 0));
            br = glm::scale(br, glm::vec3(0.06f, 0.85f, 0.06f));
            drawCube(VAO, lightingShader, br, rcW);
          }
          // Top rail (precisely capping slats)
          glm::mat4 tr2 = glm::translate(baseRc, glm::vec3(rx - 0.42f, ry + 1.90f, rz - 0.36f));
          tr2 = glm::rotate(tr2, glm::radians(-12.f), glm::vec3(1, 0, 0));
          tr2 = glm::scale(tr2, glm::vec3(0.88f, 0.09f, 0.09f));
          drawCube(VAO, lightingShader, tr2, rcW);
          // Armrests
          localBox(baseRc, rx - 0.42f, ry + 1.40f, rz - 0.22f, 0.08f, 0.07f, 0.65f, rcW);
          localBox(baseRc, rx + 0.34f, ry + 1.40f, rz - 0.22f, 0.08f, 0.07f, 0.65f, rcW);
        }

        // ── SPLINE & RULED SURFACE DECORATIVE POT ──────────────
        {
          lightingShader.use();
          glm::mat4 cm = glm::translate(I, glm::vec3(-9.5f, 4.26f, 3.5f)); // Corner of the floor (above the ground properly)
          cm = glm::scale(cm, glm::vec3(0.8f));
          flowerPotSurface.draw(lightingShader, cm, glm::vec3(0.85f, 0.40f, 0.14f)); // Terracotta clay color
          
          // And put a small plant/bush in it
          Sphere bush(0.5f, 12, 12, glm::vec3(0.1f, 0.6f, 0.2f), glm::vec3(0.2f, 0.8f, 0.3f), glm::vec3(0.1f), 4.f);
          glm::mat4 bm = glm::translate(cm, glm::vec3(0.0f, 1.2f, 0.0f));
          bm = glm::scale(bm, glm::vec3(0.8f, 0.6f, 0.8f));
          bush.drawSphere(lightingShader, bm);
        }
      }

      // ── DRESSING TABLE (ground-floor bedroom near stairs) ─────────────
      // The screenshot room shows the bed on the ground level; add a vanity
      // there too.
      {
        glm::vec3 wood(0.52f, 0.34f, 0.16f);
        glm::vec3 woodDark(0.32f, 0.20f, 0.10f);
        glm::vec3 mirrorFrame(0.22f, 0.13f, 0.04f);
        glm::vec3 mirrorGlass(0.72f, 0.84f, 0.92f);
        glm::vec3 stoolC(0.40f, 0.28f, 0.18f);

        // Positioned INSIDE the bedroom beside the bed, on the left wall.
        float vy = -1.0f;
        float vx = 7.0f;
        float vz = -20.0f; // Against the back wall

        box(vx, vy + 0.90f, vz, 1.45f, 0.10f, 0.80f, wood);     // tabletop
        box(vx, vy + 0.00f, vz, 1.45f, 0.90f, 0.80f, woodDark); // body
        // Mirror on table (standing upright at the back) – ensure facing the room (+z)
        box(vx + 0.20f, vy + 1.05f, vz + 0.05f, 0.95f, 1.45f, 0.08f,
            mirrorFrame);
        // Glancing glass face – facing toward +z (into the room)
        lightingShader.setFloat("material.shininess", 128.0f);
        mirrorBox(vx + 0.26f, vy + 1.15f, vz + 0.12f, 0.83f, 1.25f, 0.02f,
                  glm::vec3(0.9f, 0.95f, 1.0f));
        lightingShader.setFloat("material.shininess", 32.0f); // reset

        // Stool (moved forward relative to table)
        box(vx + 0.45f, vy + 0.45f, vz + 2.0f, 0.55f, 0.08f, 0.40f, stoolC);
        box(vx + 0.45f, vy + 0.45f, vz + 2.0f, 0.06f, -0.45f, 0.06f, woodDark);
        box(vx + 0.94f, vy + 0.45f, vz + 2.0f, -0.06f, -0.45f, 0.06f, woodDark);
        box(vx + 0.45f, vy + 0.45f, vz + 2.34f, 0.06f, -0.45f, -0.06f,
            woodDark);
        box(vx + 0.94f, vy + 0.45f, vz + 2.34f, -0.06f, -0.45f, -0.06f,
            woodDark);

        // Tabletop items (make it feel lived-in)
        glm::vec3 itemCream(0.92f, 0.90f, 0.84f);
        glm::vec3 itemGold(0.85f, 0.75f, 0.25f);
        glm::vec3 itemPink(0.82f, 0.48f, 0.62f);
        glm::vec3 itemBlue(0.55f, 0.72f, 0.86f);
        // Jewellery tray
        box(vx + 0.15f, vy + 1.01f, vz + 0.15f, 0.40f, 0.03f, 0.28f, itemCream);
        box(vx + 0.18f, vy + 1.04f, vz + 0.18f, 0.34f, 0.02f, 0.22f,
            woodDark * 0.9f);
        // Small lamp (base + shade)
        box(vx + 1.10f, vy + 1.00f, vz + 0.18f, 0.18f, 0.08f, 0.18f, itemGold);
        box(vx + 1.16f, vy + 1.08f, vz + 0.24f, 0.06f, 0.30f, 0.06f, itemGold);
        box(vx + 1.04f, vy + 1.38f, vz + 0.12f, 0.30f, 0.22f, 0.30f, itemCream);
        // Makeup bottles
        box(vx + 0.70f, vy + 1.01f, vz + 0.20f, 0.10f, 0.20f, 0.10f, itemPink);
        box(vx + 0.84f, vy + 1.01f, vz + 0.22f, 0.10f, 0.16f, 0.10f, itemBlue);
      }

      // ── BED (Ground Floor) ──────────────────────────────────────────
      // Height increased to 4.2 to sitting on floor properly.
      // Position moved forward (z = -17.0) to create a gap from the dresser.
      float bx = 6.0f, by = -1.0f, bz = -17.0f;
      box(bx, by, bz, 3.5f, 0.9f, 4.0f, bathC);             // Base
      box(bx + 0.2f, by + 0.5f, bz + 0.2f, 3.1f, 0.5f, 3.6f, bathInner); // Mattress

      // Bed-side steps/blocks
      box(bx + 2.5f, by + 1.8f, bz + 6.4f, 1.5f, 0.2f, 1.2f, bathC);
      box(bx, by, bz + 0.5f, 1.5f, 0.5f, 1.5f, bathC);
      box(bx, by + 0.5f, bz + 1.0f, 1.5f, 0.2f, 1.0f, bathC);
      box(bx + 3.5f, by + 2.0f, bz + 6.9f, 0.05f, 0.05f, -0.25f,
          glm::vec3(0.88f, 0.88f, 0.90f));

      // ── INTERACTIVE AC UNIT (Moved to back wall near dressing table) ───────────────────
      {
        float acX = 6.0f, acY = 2.8f, acZ = -19.6f; // High on back wall above dressing table
        glm::vec3 acBody(0.98f, 0.98f, 0.98f);       // cleaner bright white
        glm::vec3 acDark(0.12f, 0.12f, 0.15f);       // very dark grille
        glm::vec3 acFrame(0.92f, 0.92f, 0.92f);

        // AC body (main unit, wall-mounted, spans along X-axis because it's on back wall)
        box(acX, acY, acZ, 3.2f, 0.50f, 0.8f, acBody);
        // Top shell
        box(acX, acY + 0.50f, acZ - 0.02f, 3.2f, 0.06f, 0.84f, acFrame);
        // Back plate (flush to wall)
        box(acX, acY, acZ - 0.04f, 3.2f, 0.56f, 0.04f, acFrame);
        // Side panels
        box(acX - 0.02f, acY, acZ, 0.04f, 0.56f, 0.8f, acFrame);
        box(acX + 3.18f, acY, acZ, 0.04f, 0.56f, 0.8f, acFrame);

        // LED indicator (right side)
        glm::vec3 ledCol = acOn ? glm::vec3(0.1f, 0.95f, 0.2f) : glm::vec3(0.90f, 0.15f, 0.10f);
        box(acX + 2.80f, acY + 0.25f, acZ + 0.80f, 0.14f, 0.06f, 0.04f, ledCol);

        // Brand stripe (dark accent along bottom edge)
        box(acX + 0.40f, acY + 0.16f, acZ + 0.80f, 2.40f, 0.02f, 0.03f, acDark);

        // ── FRONT PANEL (opens/tilts like a gate when AC is ON) ──
        {
          // Pivot at bottom front edge of panel
          glm::mat4 panelBase = glm::translate(I, glm::vec3(acX, acY, acZ + 0.78f));
          // Rotate upwards from bottom to open downwards
          panelBase = glm::rotate(panelBase, glm::radians(acPanelAngle), glm::vec3(1, 0, 0));

          // Front panel slab
          glm::mat4 panelM = glm::scale(panelBase, glm::vec3(3.2f, 0.36f, 0.06f));
          drawCube(VAO, lightingShader, panelM, acBody);
        }

        // ── LOUVER BLADES (oscillate when AC is ON) ──
        if (acOn) {
          float louverSwing = sinf(acLouverPhase) * 25.0f; // degrees
          for (int lv = 0; lv < 5; lv++) {
            float lvX = acX + 0.30f + lv * 0.60f;
            glm::mat4 lvM = glm::translate(I, glm::vec3(lvX, acY + 0.05f, acZ + 0.60f));
            lvM = glm::rotate(lvM, glm::radians(louverSwing), glm::vec3(1, 0, 0));
            lvM = glm::scale(lvM, glm::vec3(0.50f, 0.02f, 0.08f));
            drawCube(VAO, lightingShader, lvM, glm::vec3(0.85f, 0.86f, 0.87f));
          }
        }

        // Mounting bracket (behind unit)
        box(acX + 0.50f, acY + 0.10f, acZ - 0.06f, 0.30f, 0.30f, 0.06f, acDark);
        box(acX + 2.40f, acY + 0.10f, acZ - 0.06f, 0.30f, 0.30f, 0.06f, acDark);
      }

      // ── KITCHEN ───────────────────────────────────────────────────────
      glm::vec3 cCol(1.10f, 1.02f, 0.90f);
      texBox(7.9f, -1.0f, 0.0f, 2.0f, 2.0f, 10.0f, cCol, texWood);
      texBox(7.9f, 1.0f, 0.0f, 2.0f, 0.1f, 10.0f, glm::vec3(1.0f), texCabinet);
      texBox(8.9f, 2.5f, 0.0f, 1.0f, 1.0f, 10.0f, cCol, texWood);
      texBox(8.9f, 3.5f, 0.0f, 1.0f, 0.1f, 10.0f, glm::vec3(1.0f), texCabinet);
      texBox(3.9f, -1.0f, 8.0f, 4.0f, 2.0f, 2.0f, cCol, texWood);
      texBox(3.9f, 1.0f, 8.0f, 4.0f, 0.1f, 2.0f, glm::vec3(1.0f), texCabinet);
      texBox(3.9f, 2.5f, 9.0f, 5.0f, 1.0f, 1.0f, cCol, texWood);
      texBox(3.8f, -1.0f, 8.0f, 0.1f, 2.1f, 2.0f, cCol, texWood);
      texBox(3.8f, 2.4f, 9.0f, 0.1f, 1.2f, 1.0f, cCol, texWood);

      // ── REFRIGERATOR ─────────────────────────────────────────────────
      // ── Fridge (rotated + position FIXED) ──
      {
        glm::vec3 fridgeBody(0.90f, 0.92f, 0.94f);
        glm::vec3 fridgeSide(0.72f, 0.74f, 0.76f);
        glm::vec3 fridgeDark(0.15f, 0.15f, 0.17f);
        glm::vec3 fridgeHandle(0.50f, 0.50f, 0.54f);
        glm::vec3 fridgeLogo(0.18f, 0.38f, 0.72f);
        glm::vec3 fridgeCol(0.86f, 0.88f, 0.90f);

        float fh = 5.0f, fw = 1.7f, fd = 1.5f, freezerH = 1.70f;
        float fx = 5.0f, fy = -1.0f, fz = 0.2f;

        // Base transform (pos & rotation)
        glm::mat4 base = glm::mat4(1.0f);
        base = glm::translate(base, glm::vec3(fx, fy, fz));
        // Removed 180 degree rotation so the front (z=fd) faces the kitchen (+z
        // direction).

        // 🔁 fridge-only box
        auto fridgeBox = [&](float x, float y, float z, float sx, float sy3,
                             float sz3, glm::vec3 c) {
          glm::mat4 m = glm::translate(base, glm::vec3(x, y, z));
          m = glm::scale(m, glm::vec3(sx, sy3, sz3));
          drawCube(VAO, lightingShader, m, c);
        };

        // Draw static body
        fridgeBox(0, 0, 0, 0.05f, fh, fd, fridgeSide);          // left
        fridgeBox(fw - 0.05f, 0, 0, 0.05f, fh, fd, fridgeSide); // right
        fridgeBox(0, 0, 0, fw, 0.05f, fd, fridgeSide);          // bottom
        fridgeBox(0, fh - 0.05f, 0, fw, 0.05f, fd, fridgeSide); // top
        fridgeBox(0, 0, 0, fw, fh, 0.05f, fridgeSide);          // back frame

        // Dark inner back panel (depth cue when door is open; thin to avoid
        // shelf z-fight)
        glm::vec3 innerCol(0.10f, 0.11f, 0.13f);
        fridgeBox(0.07f, 0.07f, 0.07f, fw - 0.14f, fh - 0.14f, 0.04f, innerCol);

        // Inside elements (shelves, food)
        fridgeBox(0.05f, 1.0f, 0.05f, fw - 0.1f, 0.04f, fd - 0.1f,
                  fridgeSide); // shelf 1
        fridgeBox(0.05f, 1.8f, 0.05f, fw - 0.1f, 0.04f, fd - 0.1f,
                  fridgeSide); // shelf 2
        fridgeBox(0.05f, 2.6f, 0.05f, fw - 0.1f, 0.04f, fd - 0.1f,
                  fridgeSide); // shelf 3

        // Groceries (positioned more forward within fridge depth)
        float doorOpenFactor = glm::clamp(fridgeDoorAngle / 108.0f, 0.0f, 1.0f);
        float openOffset = doorOpenFactor * 0.25f;
        fridgeBox(0.2f, 1.04f, 0.6f + openOffset, 0.32f, 0.55f, 0.28f,
                  glm::vec3(0.92f, 0.22f, 0.18f));
        fridgeBox(0.65f, 1.82f, 0.7f + openOffset, 0.28f, 0.48f, 0.22f,
                  glm::vec3(0.15f, 0.42f, 0.85f));
        fridgeBox(1.05f, 2.62f, 0.8f + openOffset, 0.38f, 0.22f, 0.28f,
                  glm::vec3(0.85f, 0.82f, 0.12f));
        fridgeBox(0.45f, 2.05f, 0.7f + openOffset, 0.22f, 0.18f, 0.20f,
                  glm::vec3(0.25f, 0.72f, 0.28f));
        fridgeBox(0.85f, 1.15f, 0.65f + openOffset, 0.20f, 0.25f, 0.18f,
                  glm::vec3(0.95f, 0.55f, 0.65f));

        float doorInsetX = 0.06f, doorInsetY = 0.06f, doorT = 0.08f;
        float gapH = 0.06f;
        float handleX = fw - 0.25f;
        float doorH = (fh - freezerH) - (gapH + 2 * doorInsetY);
        float doorW = fw - 2 * doorInsetX;
        float frontZ = fd - doorT; // 1.42

        // Gap dark box
        fridgeBox(0.02f, (fh - freezerH) - gapH * 0.5f, frontZ - 0.02f,
                  fw - 0.04f, gapH, 0.04f, fridgeDark);

        // TOP DOOR (Freezer) - Static
        fridgeBox(doorInsetX, fh - freezerH + doorInsetY, frontZ, doorW,
                  freezerH - 2 * doorInsetY, doorT, fridgeCol);
        fridgeBox(handleX, fh - freezerH + 0.25f, frontZ + doorT, 0.10f,
                  freezerH - 0.55f, 0.10f, fridgeHandle);

        // BOTTOM DOOR — moved pivot to the FRONT of fridge
        glm::vec3 piv(doorInsetX, doorInsetY, frontZ);
        glm::mat4 doorBase =
            glm::translate(base, piv) *
            glm::rotate(glm::mat4(1.f), glm::radians(fridgeDoorAngle),
                        glm::vec3(0.f, 1.f, 0.f));

        auto drawDoorPart = [&](float ox, float oy, float oz, float sx,
                                float sy3, float sz3, glm::vec3 c) {
          glm::mat4 m = glm::translate(doorBase, glm::vec3(ox, oy, oz));
          m = glm::scale(m, glm::vec3(sx, sy3, sz3));
          drawCube(VAO, lightingShader, m, c);
        };

        drawDoorPart(0.f, 0.f, 0.f, doorW, doorH, doorT, fridgeCol);
        // Handle bottom door
        drawDoorPart(handleX - doorInsetX, 0.25f, doorT, 0.10f, doorH - 0.55f,
                     0.10f, fridgeHandle);

        // base dark stand
        fridgeBox(0.02f, 0, -0.02f, fw - 0.04f, 0.12f, 0.04f, fridgeDark);

        // ── Shadow ──
        fridgeBox(-0.05f, -0.01f, -0.10f, fw + 0.10f, 0.02f, fd + 0.15f,
                  glm::vec3(0.08f, 0.08f, 0.08f));
      }

      // ── KITCHEN ITEMS ─────────────────────────────────────────────────
      {
        glm::vec3 plateCol(0.95f, 0.95f, 0.95f);
        glm::vec3 potCol(0.25f, 0.25f, 0.28f);
        glm::vec3 potRim(0.70f, 0.70f, 0.72f);
        glm::vec3 cupRed(0.80f, 0.18f, 0.12f);
        glm::vec3 cupBlue(0.15f, 0.35f, 0.72f);
        glm::vec3 panCol(0.20f, 0.20f, 0.22f);
        glm::vec3 panHandle(0.38f, 0.22f, 0.08f);
        glm::vec3 boxTan(0.80f, 0.64f, 0.38f);
        glm::vec3 boxRed(0.85f, 0.22f, 0.12f);
        glm::vec3 boxGreen(0.28f, 0.60f, 0.28f);
        glm::vec3 fruitR(0.82f, 0.10f, 0.10f);
        glm::vec3 fruitG(0.18f, 0.62f, 0.18f);
        glm::vec3 bowlCol(0.88f, 0.82f, 0.68f);
        glm::vec3 boardCol(0.72f, 0.50f, 0.25f);
        glm::vec3 knifeCol(0.80f, 0.80f, 0.82f);

        // Counter top sits at y=1.0, objects placed at y=1.02+
        // Cutting board (z~2, x counter face ~7.92)
        box(7.92f, 1.02f, 2.0f, 1.4f, 0.06f, 0.85f, boardCol);
        box(7.95f, 1.09f, 2.15f, 0.05f, 0.04f, 0.65f, knifeCol);
        box(7.95f, 1.09f, 2.80f, 0.05f, 0.04f, 0.18f, panHandle);

        // Stacked plates (z~4)
        box(7.92f, 1.02f, 4.0f, 0.85f, 0.06f, 0.85f, plateCol);
        box(7.92f, 1.08f, 4.0f, 0.85f, 0.06f, 0.85f, plateCol);
        box(7.92f, 1.14f, 4.0f, 0.85f, 0.06f, 0.85f, plateCol);

        // Frying pan (z~5.5)
        box(7.92f, 1.02f, 5.5f, 1.0f, 0.09f, 1.0f, panCol);
        box(8.92f, 1.02f, 5.85f, 1.1f, 0.07f, 0.18f, panHandle);

        // Large pot (z~7)
        box(7.92f, 1.02f, 7.0f, 1.1f, 0.65f, 1.1f, potCol);
        box(7.90f, 1.67f, 6.98f, 1.14f, 0.07f, 1.14f, potRim);
        box(8.30f, 1.74f, 7.50f, 0.30f, 0.14f, 0.10f, potRim);

        // Mugs on upper shelf (y=3.52, z~3–3.6)
        box(8.92f, 3.52f, 3.05f, 0.42f, 0.42f, 0.42f, cupRed);
        box(8.92f, 3.52f, 3.58f, 0.42f, 0.42f, 0.42f, cupBlue);

        // Food boxes on upper shelf (z~6–8)
        box(8.92f, 3.52f, 6.0f, 0.50f, 0.85f, 0.42f, boxTan);
        box(8.92f, 3.52f, 6.0f, 0.50f, 0.16f, 0.42f, boxRed);
        box(8.92f, 3.52f, 6.52f, 0.50f, 0.92f, 0.42f, boxGreen);
        box(8.92f, 3.52f, 7.04f, 0.50f, 0.72f, 0.42f, boxTan);

        // Fruit bowl (z~9)
        box(7.92f, 1.02f, 9.0f, 1.0f, 0.16f, 1.0f, bowlCol);
        box(8.10f, 1.19f, 9.10f, 0.36f, 0.36f, 0.36f, fruitR);
        box(8.50f, 1.19f, 9.42f, 0.36f, 0.36f, 0.36f, fruitG);
        box(8.20f, 1.19f, 9.52f, 0.30f, 0.30f, 0.30f, fruitR);
      }

      // ── WALL PORTRAITS ────────────────────────────────────────────────
      {
        glm::vec3 frameD(0.22f, 0.13f, 0.04f); // dark wood frame

        auto frameBorder = [&](float x, float y, float z, float w, float h,
                               float t, float border, glm::vec3 col) {
          // 4-piece border around a picture rectangle on a wall
          // Top
          box(x - border, y + h, z, w + 2 * border, border, t, col);
          // Bottom
          box(x - border, y - border, z, w + 2 * border, border, t, col);
          // Left
          box(x - border, y, z, border, h, t, col);
          // Right
          box(x + w, y, z, border, h, t, col);
        };

        // ── Portrait 1 – TV room south wall (z = -12) ──────────────────
        // Camera in TV room looks toward -z, sees the wall at z=-12.
        // tVAO FRONT face is at z = origin_z + sz (normal +z).
        // We want the image face to look INTO the room (+z direction = toward
        // camera). So place slab: origin z = -12.0, sz = +0.12 → FRONT face at
        // z=-11.88, normal +z, visible from room side. ✓
        frameBorder(-7.6f, 1.2f, -11.995f, 3.4f, 2.1f, 0.13f, 0.20f, frameD);
        texBox(-7.6f, 1.2f, -11.99f, 3.4f, 2.1f, 0.11f, // picture
               glm::vec3(1.0f, 1.0f, 1.0f), texPortrait1);

        // ── Portrait 2 – Dining room back wall (z = 10) ────────────────
        // The back wall inner face is at z=10 (wall drawn at z=10, -0.1 thick
        // going +z). Dining camera is at z < 5 looking toward +z (toward back
        // wall). tVAO BACK face is at z = origin_z (normal -z), visible from +z
        // looking back. Place slab touching the wall: origin z = 9.88, sz =
        // +0.12 → BACK face at z=9.88, normal -z, faces INTO the room (toward
        // -z viewer). ✓ Dining table x: -7.5→-0.5; center x=-4.0; portrait
        // width 4.0 units
        frameBorder(-7.1f, 1.5f, 9.865f, 3.6f, 2.1f, 0.13f, 0.20f, frameD);
        texBox(-7.1f, 1.5f, 9.895f, 3.6f, 2.1f, 0.11f,
               glm::vec3(1.0f, 1.0f, 1.0f), texPortrait2);

        // ── Portrait 3 – Bedroom Side Wall (Restored & Fixed) ──────────
        // On side wall (x=10), so x is thickness, z is width.
        float px = 9.85f, py = 1.4f, pz = -18.0f, pw = 3.2f, ph = 2.2f, pt = 0.12f,
              pb = 0.15f;
        // Border: Top/Bottom along Z
        box(px, py + ph, pz - pb, pt, pb, pw + 2 * pb, frameD);
        box(px, py - pb, pz - pb, pt, pb, pw + 2 * pb, frameD);
        // Border: Left/Right along Y
        box(px, py, pz - pb, pt, ph, pb, frameD);
        box(px, py, pz + pw, pt, ph, pb, frameD);

        // Picture face – nudged from wall and frame back
        texBox(px + 0.02f, py, pz, 0.08f, ph, pw, glm::vec3(1.0f, 1.0f, 1.0f),
               texPortrait2);
      }

      // ── FLOWER VASE on dining table ───────────────────────────────────
      // Table x: -7.5→-0.5, z: 5.0→8.0 → center x=-4.0, z=6.5
      // Table top y = 1.10
      {
        float cx = -4.00f, cz = 6.50f, cy = 1.14f;

        lightingShader.use();

        // Realistic vase using Bezier surface
        glm::mat4 vm = glm::translate(I, glm::vec3(cx, cy, cz));
        vm = glm::scale(vm, glm::vec3(0.3f, 0.45f, 0.3f));
        bezVase.draw(lightingShader, vm,
                     glm::vec3(0.85f, 0.90f, 0.95f)); // Light glass tint

        // Flowers using small spheres and cylinder stems
        Sphere fColorRed(0.5f, 12, 12, glm::vec3(0.92f, 0.14f, 0.18f),
                         glm::vec3(0.92f, 0.14f, 0.18f), glm::vec3(0.5f), 16.f);
        Sphere fColorYel(0.5f, 12, 12, glm::vec3(0.96f, 0.82f, 0.08f),
                         glm::vec3(0.96f, 0.82f, 0.08f), glm::vec3(0.5f), 16.f);
        Sphere fColorPnk(0.5f, 12, 12, glm::vec3(0.96f, 0.52f, 0.68f),
                         glm::vec3(0.96f, 0.52f, 0.68f), glm::vec3(0.5f), 16.f);
        Cylinder stemCyl(0.5f, 8, 2, glm::vec3(0.14f, 0.52f, 0.18f),
                         glm::vec3(0.14f, 0.52f, 0.18f), glm::vec3(0.1f), 4.f);

        // Oval sphere for smooth, beautiful petals rather than sharp long spiky
        // cones
        Sphere petalSph(0.5f, 10, 10, glm::vec3(1.f), glm::vec3(1.f),
                        glm::vec3(0.1f), 4.f);

        auto drawStem = [&](float ox, float oy, float oz, float stemLen) {
          glm::mat4 sm =
              glm::translate(I, glm::vec3(cx + ox, cy + 0.22f, cz + oz));
          sm = glm::scale(sm, glm::vec3(0.035f, stemLen, 0.035f));
          stemCyl.drawSphere(lightingShader, sm);
        };

        // Multi-petal bouquet: 8 petals + inner ring for fuller silhouette
        auto drawSymmetricFlower = [&](float ox, float oy, float oz,
                                       float stemLen, Sphere &coreObj,
                                       glm::vec3 pc) {
          drawStem(ox, oy, oz, stemLen);

          // Move flower core slightly up so petals sit above the table top
          float petalRaise = 0.22f; // raise petals/core a bit
          glm::mat4 fm = glm::translate(
              I, glm::vec3(cx + ox, cy + oy + petalRaise, cz + oz));
          glm::mat4 fmC = glm::scale(fm, glm::vec3(0.10f));
          coreObj.drawSphere(lightingShader, fmC);

          petalSph.diffuse = pc;
          petalSph.ambient = pc * 0.85f;
          for (int i = 0; i < 8; i++) {
            float ang = glm::radians(i * 45.f);
            glm::mat4 pm = glm::rotate(fm, ang, glm::vec3(0, 1, 0));
            pm = glm::translate(pm, glm::vec3(0.f, 0.02f, 0.13f));
            pm = glm::rotate(pm, glm::radians(68.f), glm::vec3(1, 0, 0));
            pm = glm::scale(pm, glm::vec3(0.055f, 0.14f, 0.018f));
            petalSph.drawSphere(lightingShader, pm);
          }
          petalSph.diffuse = pc * 1.08f;
          petalSph.ambient = pc * 0.7f;
          for (int i = 0; i < 8; i++) {
            float ang = glm::radians(i * 45.f + 22.5f);
            glm::mat4 pm = glm::rotate(fm, ang, glm::vec3(0, 1, 0));
            pm = glm::translate(pm, glm::vec3(0.f, -0.04f, 0.09f));
            pm = glm::rotate(pm, glm::radians(82.f), glm::vec3(1, 0, 0));
            pm = glm::scale(pm, glm::vec3(0.04f, 0.10f, 0.014f));
            petalSph.drawSphere(lightingShader, pm);
          }
        };

        drawSymmetricFlower(0.02f, 0.62f, 0.0f, 0.42f, fColorRed,
                            glm::vec3(0.88f, 0.12f, 0.18f));
        drawSymmetricFlower(-0.07f, 0.52f, -0.06f, 0.36f, fColorYel,
                            glm::vec3(0.94f, 0.78f, 0.10f));
        drawSymmetricFlower(0.06f, 0.56f, 0.07f, 0.40f, fColorPnk,
                            glm::vec3(0.92f, 0.48f, 0.65f));
      }

      // ── Bezier Pot + Fractal Trees (clean branch joints) ──────────────
      {
        auto potTree = [&](float px, float pz) {
          // Pot
          glm::mat4 pm = glm::translate(I, glm::vec3(px, -0.98f, pz));
          pm = glm::scale(pm, glm::vec3(0.52f, 0.70f, 0.52f));
          bezVase.draw(lightingShader, pm, glm::vec3(0.56f, 0.34f, 0.18f));

          // Pebble soil
          for (int i = 0; i < 12; i++) {
            float a = glm::radians(i * 30.0f);
            box(px + 0.16f * cosf(a), -0.50f, pz + 0.16f * sinf(a), 0.06f, 0.03f,
                0.06f, glm::vec3(0.26f, 0.18f, 0.10f));
          }

          // Start trunk exactly from soil for perfect join
          drawFractalTree(lightingShader, treeBark, treeLeaf, px, -0.50f, pz, 0.85f,
                          0.035f, 3);
        };

        // Dining corners
        potTree(-8.9f, 9.0f);
        potTree(-1.1f, 9.0f);
        // TV-room corner (new)
        potTree(-9.0f, -10.8f);
      }

      // ── EXTRA TABLE DECOR (dining) ────────────────────────────────────
      // Add a small runner + plates/glasses so the table feels lived-in.
      {
        glm::vec3 runner(0.82f, 0.78f, 0.70f);
        glm::vec3 plate(0.94f, 0.94f, 0.95f);
        glm::vec3 glass(0.72f, 0.86f, 0.90f);
        float ty = 1.10f;

        // Runner strip down the center
        box(-7.2f, ty + 0.01f, 6.35f, 6.4f, 0.02f, 0.30f, runner);

        // Two plates
        box(-6.2f, ty + 0.02f, 6.0f, 0.65f, 0.04f, 0.65f, plate);
        box(-2.3f, ty + 0.02f, 7.0f, 0.65f, 0.04f, 0.65f, plate);

        // Two glasses (stacked thin rings)
        box(-5.7f, ty + 0.06f, 6.2f, 0.18f, 0.18f, 0.18f, glass);
        box(-5.68f, ty + 0.24f, 6.22f, 0.14f, 0.04f, 0.14f, glass);
        box(-1.8f, ty + 0.06f, 7.2f, 0.18f, 0.18f, 0.18f, glass);
        box(-1.78f, ty + 0.24f, 7.22f, 0.14f, 0.04f, 0.14f, glass);
      }

      // ── FRUIT BOWL on dining table ────────────────────────────────────
      // Table top surface: y = 1.10  (table base y=1.0, sy=0.1)
      // Bowl centered at x=-2.0, z=6.5 (right half of table, away from vase)
      {
        glm::vec3 bowlBase(0.88f, 0.80f, 0.62f); // warm ceramic
        glm::vec3 bowlBody(0.95f, 0.88f, 0.70f); // lighter body
        glm::vec3 bowlRim(0.75f, 0.65f, 0.45f);  // darker rim accent
        glm::vec3 appleR(0.88f, 0.12f, 0.10f);   // red apple
        glm::vec3 appleG(0.20f, 0.62f, 0.18f);   // green apple
        glm::vec3 orangeC(0.95f, 0.50f, 0.06f);  // orange
        glm::vec3 bananaC(0.96f, 0.88f, 0.18f);  // banana
        glm::vec3 stemCol2(0.30f, 0.18f, 0.04f); // fruit stem dark

        float bx = -2.0f, bz = 6.5f, by = 1.10f;

        // ── Bowl (stacked rings, wide-to-narrow taper upward) ──
        // Base foot (flat, small)
        box(bx - 0.18f, by + 0.00f, bz - 0.18f, 0.36f, 0.04f, 0.36f, bowlBase);
        // Lower ring (widest)
        box(bx - 0.38f, by + 0.04f, bz - 0.38f, 0.76f, 0.08f, 0.76f, bowlBody);
        // Mid ring
        box(bx - 0.34f, by + 0.12f, bz - 0.34f, 0.68f, 0.08f, 0.68f, bowlBody);
        // Upper ring (narrowing)
        box(bx - 0.28f, by + 0.20f, bz - 0.28f, 0.56f, 0.06f, 0.56f, bowlBody);
        // Rim (thin accent strip, slightly wider)
        box(bx - 0.30f, by + 0.26f, bz - 0.30f, 0.60f, 0.04f, 0.60f, bowlRim);

        // ── Fruits sitting IN the bowl (y = by+0.30 = on rim level) ──
        float fy = by + 0.31f; // fruits rest on rim

        // Red apple (center, tallest)
        box(bx - 0.13f, fy + 0.00f, bz - 0.13f, 0.26f, 0.26f, 0.26f,
            appleR); // body
        box(bx - 0.09f, fy + 0.26f, bz - 0.09f, 0.18f, 0.04f, 0.18f,
            appleR); // top dome
        box(bx - 0.01f, fy + 0.30f, bz - 0.01f, 0.03f, 0.08f, 0.03f,
            stemCol2); // stem
        box(bx - 0.04f, fy + 0.24f, bz - 0.04f, 0.06f, 0.06f, 0.06f,
            glm::vec3(0.55f, 0.08f, 0.08f)); // calyx

        // Green apple (left-front)
        box(bx - 0.26f, fy + 0.00f, bz + 0.02f, 0.24f, 0.24f, 0.24f, appleG);
        box(bx - 0.22f, fy + 0.24f, bz + 0.06f, 0.16f, 0.04f, 0.16f, appleG);
        box(bx - 0.15f, fy + 0.28f, bz + 0.10f, 0.03f, 0.07f, 0.03f, stemCol2);

        // Orange (right side, rounder = square)
        box(bx + 0.06f, fy + 0.00f, bz - 0.12f, 0.24f, 0.22f, 0.24f, orangeC);
        box(bx + 0.10f, fy + 0.22f, bz - 0.08f, 0.16f, 0.04f, 0.16f, orangeC);
        // Navel dimple accent
        box(bx + 0.17f, fy + 0.10f, bz - 0.01f, 0.03f, 0.03f, 0.03f,
            glm::vec3(0.75f, 0.35f, 0.02f));

        // Banana (lying flat, front-right of bowl)
        box(bx + 0.02f, fy + 0.00f, bz + 0.06f, 0.22f, 0.10f, 0.18f,
            bananaC); // body
        box(bx + 0.20f, fy + 0.04f, bz + 0.08f, 0.06f, 0.06f, 0.10f,
            bananaC); // tip curve
        box(bx + 0.02f, fy + 0.04f, bz + 0.08f, 0.04f, 0.06f, 0.10f,
            glm::vec3(0.55f, 0.42f, 0.04f)); // stem end

        // Shading accent: subtle dark shadow under bowl
        box(bx - 0.36f, by - 0.01f, bz - 0.36f, 0.72f, 0.02f, 0.72f,
            glm::vec3(0.12f, 0.10f, 0.08f));
      }
      // ── SHELF ─────────────────────────────────────────────────────────
      box(-9.9f, -1, 0, 0.1f, 3, 2, shelfCol);
      box(-6.9f, -1, 0, 0.1f, 3, 2, shelfCol);
      for (int i = 0; i < 4; i++)
        box(-9.8f, -0.5f + i * 0.75f, 0, 3, 0.1f, 2,
            glm::vec3(0.98f, 0.75f, 0.30f));

      // ── LIFT CAR & DOORS ──────────────────────────────────────────────
      {
        float lX = 6.1f, lZ = -3.9f, lS = 3.8f;
        glm::vec3 carC(0.75f, 0.75f, 0.78f), carInC(0.15f, 0.15f, 0.18f);

        // Draw lift car at liftY
        // Floor
        texBox(lX, -1.0f + liftY, lZ, lS, 0.15f, lS, carC, texLift);
        // Ceiling
        texBox(lX, -1.0f + liftY + 3.85f, lZ, lS, 0.15f, lS, carC, texLift);
        // Back wall
        texBox(lX, -1.0f + liftY, lZ, lS, 4.0f, 0.1f, carC, texLift);
        // Side walls
        texBox(lX, -1.0f + liftY, lZ, 0.1f, 4.0f, lS, carC, texLift);
        texBox(lX + lS - 0.1f, -1.0f + liftY, lZ, 0.1f, 4.0f, lS, carC,
               texLift);

        // ── Interior Buttons ──
        float btnY = -1.0f + liftY + 1.8f;
        // Panel
        box(lX + 0.12f, btnY - 0.4f, lZ + 0.1f, 0.05f, 0.8f, 0.4f,
            glm::vec3(0.3f));
        // GF button (red)
        box(lX + 0.18f, btnY - 0.2f, lZ + 0.2f, 0.02f, 0.15f, 0.15f,
            liftUp ? glm::vec3(0.4f, 0.1f, 0.1f) : glm::vec3(1.0f, 0.1f, 0.1f));
        // 1F button (green)
        box(lX + 0.18f, btnY + 0.1f, lZ + 0.2f, 0.02f, 0.15f, 0.15f,
            liftUp ? glm::vec3(0.1f, 1.0f, 0.1f) : glm::vec3(0.1f, 0.4f, 0.1f));
        // Door-open indicator/button
        box(lX + 0.18f, btnY + 0.38f, lZ + 0.2f, 0.02f, 0.10f, 0.15f,
            glm::vec3(0.85f, 0.70f, 0.15f));

        // ── Automated Doors ──
        // Doors at Ground Floor (GF)
        if (std::abs(liftY - 0.0f) < 0.5f) {
          float dS = liftDoorAngle / 90.0f;
          texBox(lX + lS * 0.5f, -1.0f, lZ + lS - 0.05f,
                 lS * 0.5f * (1.0f - dS), 4.0f, 0.1f, carC,
                 texLift); // half door left
          texBox(lX, -1.0f, lZ + lS - 0.05f, lS * 0.5f * (1.0f - dS), 4.0f,
                 0.1f, carC, texLift); // half door right
        }
        // Doors at 1st Floor (1F)
        if (std::abs(liftY - 5.0f) < 0.5f) {
          float dS = liftDoorAngle / 90.0f;
          texBox(lX + lS * 0.5f, 4.0f, lZ + lS - 0.05f, lS * 0.5f * (1.0f - dS),
                 4.0f, 0.1f, carC, texLift);
          texBox(lX, 4.0f, lZ + lS - 0.05f, lS * 0.5f * (1.0f - dS), 4.0f, 0.1f,
                 carC, texLift);
        }

        // Landing call-button panels (outside lift area, both floors).
        glm::vec3 panelCol(0.26f, 0.26f, 0.28f);
        glm::vec3 onCol(0.15f, 0.80f, 0.20f);
        glm::vec3 offCol(0.12f, 0.28f, 0.12f);
        // Ground floor panel near the lift entrance.
        box(9.95f, 0.9f, -0.25f, 0.05f, 0.70f, 0.40f, panelCol);
        box(9.92f, 1.05f, -0.15f, 0.03f, 0.10f, 0.10f, liftUp ? offCol : onCol);
        box(9.92f, 1.32f, -0.15f, 0.03f, 0.10f, 0.10f, liftUp ? onCol : offCol);
        // First-floor panel near the top landing.
        box(9.95f, 5.9f, -0.25f, 0.05f, 0.70f, 0.40f, panelCol);
        box(9.92f, 6.05f, -0.15f, 0.03f, 0.10f, 0.10f, liftUp ? onCol : offCol);
        box(9.92f, 6.32f, -0.15f, 0.03f, 0.10f, 0.10f, liftUp ? offCol : onCol);
      }

      // ── FAN ───────────────────────────────────────────────────────────
      box(-6.35f, 3.5f, -5.15f, 0.15f, 0.5f, 0.15f, fanHub);
      box(-6.55f, 3.0f, -5.35f, 0.5f, 0.1f, 0.5f, fanHub);
      for (int i = 0; i < 4; i++) {
        float ang = fanRot + i * 90.0f;
        glm::mat4 T = glm::translate(I, glm::vec3(-6.3f, 3.05f, -5.1f));
        glm::mat4 R = glm::rotate(I, glm::radians(ang), glm::vec3(0, 1, 0));
        glm::mat4 S = glm::scale(I, glm::vec3(2.2f, 0.05f, 0.35f));
        drawCube(VAO, lightingShader, T * R * S, fanBlade);
      }

      // ── TREES (outdoor) ───────────────────────────────────────────────
      // Trees now use Sphere for the canopy instead of boxes
      glm::vec3 trunkCol(0.42f, 0.24f, 0.08f);
      auto tree = [&](float tx, float tz) {
        // trunk (box)
        box(tx - 0.2f, -1.0f, tz - 0.2f, 0.4f, 3.5f, 0.4f, trunkCol);
        // canopy – THREE sphere layers (Sphere object)
        // dark base sphere (textured)
        glm::mat4 ms = glm::translate(I, glm::vec3(tx, 2.8f, tz));
        ms = glm::scale(ms, glm::vec3(1.5f, 1.2f, 1.5f));
        setupTexShader();
        texShader.setVec3("objectColor", treeTopSphere.diffuse);
        texShader.setVec3("material.ambient", treeTopSphere.ambient * 0.4f);
        texShader.setVec3("material.diffuse", treeTopSphere.diffuse);
        texShader.setVec3("material.specular", treeTopSphere.specular);
        texShader.setFloat("material.shininess", treeTopSphere.shininess);
        treeTopSphere.drawSphereTexture(
            texShader, ms, texTreeLeaves, texTreeLeaves,
            treeTopSphere.diffuse); // রঙটা pass করো
        lightingShader.use();
        // mid sphere (textured)
        ms = glm::translate(I, glm::vec3(tx, 3.8f, tz));
        ms = glm::scale(ms, glm::vec3(1.1f, 1.0f, 1.1f));
        setupTexShader();
        texShader.setVec3("objectColor", treeTopSphere.diffuse);
        texShader.setVec3("material.ambient", treeTopSphere.ambient * 0.4f);
        texShader.setVec3("material.diffuse", treeTopSphere.diffuse);
        texShader.setVec3("material.specular", treeTopSphere.specular);
        texShader.setFloat("material.shininess", treeTopSphere.shininess);
        treeTopSphere.drawSphereTexture(texShader, ms, texTreeLeaves,
                                        texTreeLeaves);
        lightingShader.use();
        // top small sphere (textured)
        Sphere topS(1.0f, 16, 10, glm::vec3(0.30f, 0.85f, 0.35f),
                    glm::vec3(0.30f, 0.85f, 0.35f),
                    glm::vec3(0.1f, 0.3f, 0.1f), 16.0f);
        ms = glm::translate(I, glm::vec3(tx, 4.7f, tz));
        ms = glm::scale(ms, glm::vec3(0.7f, 0.7f, 0.7f));
        setupTexShader();
        texShader.setVec3("objectColor", topS.diffuse);
        texShader.setVec3("material.ambient", topS.ambient * 0.4f);
        texShader.setVec3("material.diffuse", topS.diffuse);
        texShader.setVec3("material.specular", topS.specular);
        texShader.setFloat("material.shininess", topS.shininess);
        topS.drawSphereTexture(texShader, ms, texTreeLeaves, texTreeLeaves);
        lightingShader.use();
      };

      {

        tree(-16.0f, -14.0f); // Left yard
        tree(-17.0f, 2.0f);
        tree(-16.5f, -24.0f);
        tree(16.0f, -14.0f); // Right yard (symmetric)
        tree(17.0f, 2.0f);
        tree(16.5f, -24.0f);
        tree(3.5f, -26.0f);  // Back yard
        tree(-3.5f, -26.0f); // Back yard (symmetric)
        tree(14.0f, -6.0f);
      }

      // ── COLONY ARCHITECTURE (Distant Neighborhood) ───────────────────
      auto drawNeighborhoodHouse = [&](float hx, float hz, int seed) {
          srand(seed);
          glm::vec3 hCol = (seed % 2 == 0) ? glm::vec3(0.7f, 0.72f, 0.75f) : glm::vec3(0.65f, 0.58f, 0.52f);
          if (seed % 3 == 0) hCol = glm::vec3(0.48f, 0.52f, 0.62f);
          
          float hW = 6.0f + (float)(rand() % 4);
          float hL = 8.0f + (float)(rand() % 4);
          float hH = 4.0f + (float)(rand() % 3);
          
          // Main Body
          box(hx, -1.0f, hz, hW, hH, hL, hCol);
          // Roof (flat/slanted block for simplicity in colony)
          glm::vec3 rCol(0.45f, 0.35f, 0.32f);
          box(hx - 0.5f, -1.0f + hH, hz - 0.5f, hW + 1.0f, 0.6f, hL + 1.0f, rCol);
          // Windows (simple blue blocks)
          glm::vec3 wCol(0.5f, 0.75f, 0.9f);
          box(hx + 1.5f, hH * 0.5f, hz + hL - 0.05f, 2.0f, 1.2f, 0.1f, wCol);
      };


      // ── BOLLARD CONES (outdoor path markers) ──────────────────────────
      // Decorative orange safety cones along the driveway
      {
        auto bollard = [&](float bx, float bz) {
          glm::mat4 mc = glm::translate(I, glm::vec3(bx, -1.0f, bz));
          // draw bollard with texture
          setupTexShader();
          texShader.setVec3("objectColor", bollardCone.diffuse);
          texShader.setVec3("material.ambient", bollardCone.ambient * 0.4f);
          texShader.setVec3("material.diffuse", bollardCone.diffuse);
          texShader.setVec3("material.specular", bollardCone.specular);
          texShader.setFloat("material.shininess", bollardCone.shininess);
          bollardCone.drawConeTexture(texShader, mc, texBollard, texBollard);
          lightingShader.use();
        };
        bollard(-11.0f, -22.5f);
        bollard(11.5f, -22.5f);
        bollard(-11.0f, 11.0f);
        bollard(11.5f, 11.0f);
      }

      // ── DECORATIVE SPHERE (living room globe lamp) ────────────────────
      // Glowing cream sphere hanging from ceiling in living room
      // ── GROUND FLOOR TV  (TV room, south wall z≈-12) ──────────────────
      {
        // TV stand/unit – moved to RIGHT side to avoid portrait overlap
        glm::vec3 tvFrm(0.08f, 0.08f, 0.10f);
        glm::vec3 tvScr(0.05f, 0.15f, 0.30f);
        glm::vec3 tvStnd(0.18f, 0.18f, 0.20f);

        // TV screen (box on wall) – positioned in front of wall
        box(-3.6f, 1.0f, -11.90f, 3.2f, 2.0f, -0.12f, tvFrm);
        box(-3.48f, 1.12f, -11.88f, 2.96f, 1.76f, -0.06f, tvScr);
        // TV image slideshow (tv.jpg, tv2.jpg, tv3.jpg)
        unsigned int tvSlides[3] = {texTV, texTV2, texTV3};
        int tvIdx = ((int)(glfwGetTime() / 4.0)) % 3;
        
        // Using texBox automatically applies the normal-fix and correctly draws the texture scaled correctly
        texBox(-3.44f, 1.16f, -11.86f, 2.88f, 1.68f, -0.04f, glm::vec3(1.0f), tvSlides[tvIdx]);
        // Screen highlights
        box(-3.46f, 2.74f, -11.85f, 2.92f, 0.14f, -0.04f,
            glm::vec3(0.08f, 0.24f, 0.45f));
      }

      {
        glm::mat4 ms = glm::translate(I, glm::vec3(-6.3f, 3.1f, -5.1f));
        ms = glm::scale(ms, glm::vec3(0.4f, 0.6f, 0.4f));
        // draw decorative sphere with texture
        setupTexShader();
        texShader.setVec3("objectColor", decorSphere.diffuse);
        texShader.setVec3("material.ambient", decorSphere.ambient * 0.4f);
        texShader.setVec3("material.diffuse", decorSphere.diffuse);
        texShader.setVec3("material.specular", decorSphere.specular);
        texShader.setFloat("material.shininess", decorSphere.shininess);
        decorSphere.drawSphereTexture(texShader, ms, texDecorSphere,
                                      texDecorSphere);
        lightingShader.use();
      }

      //// ── DECORATIVE CONE (lamp shade above dining table) ───────────────
      //{
      //    glm::mat4 mc = glm::translate(I, glm::vec3(-4.0f, 3.5f, 6.5f));
      //    mc = glm::rotate(mc, glm::radians(180.0f), glm::vec3(1, 0, 0));
      //    // draw decorative cone with texture
      //    setupTexShader();
      //    texShader.setVec3("objectColor", decorCone.diffuse);
      //    texShader.setVec3("material.ambient", decorCone.ambient * 0.4f);
      //    texShader.setVec3("material.diffuse", decorCone.diffuse);
      //    texShader.setVec3("material.specular", decorCone.specular);
      //    texShader.setFloat("material.shininess", decorCone.shininess);
      //    decorCone.drawConeTexture(texShader, mc, texDecorCone,
      //    texDecorCone); lightingShader.use();
      //}

      // ── ROAD MARKINGS ─────────────────────────────────────────────────
      {
        // ── ROAD MARKINGS ─────────────────────────────────────────────────
        {
          glm::vec3 roadMark(0.95f, 0.95f, 0.92f);
          for (int i = -9; i <= 9; i += 3)
            box((float)i - 0.5f, -0.985f, -34.0f, 1.5f, 0.01f, 0.2f, roadMark);
          for (int i = -8; i <= 8; i += 3)
            box(-19.5f, -0.985f, (float)i - 0.1f, 0.2f, 0.01f, 1.5f, roadMark);
          for (int i = -8; i <= 8; i += 3)
            box(14.5f, -0.985f, (float)i - 0.1f, 0.2f, 0.01f, 1.5f, roadMark);

          // Road center dashed stripes (white lines) to make road realistic
          for (float z = -40; z <= 20; z += 4)
            box(-25.2f, -0.985f, (float)z, 0.4f, 0.01f, 2.0f, roadMark);
          for (float z = -40; z <= 20; z += 4)
            box(24.8f, -0.985f, (float)z, 0.4f, 0.01f, 2.0f, roadMark);
          for (float x = -30; x <= 20; x += 4)
            box((float)x, -0.985f, -35.2f, 2.0f, 0.01f, 0.4f, roadMark);
          for (float x = -30; x <= 20; x += 4)
            box((float)x, -0.985f, 24.8f, 2.0f, 0.01f, 0.4f, roadMark);
        }
      }

      // ── EXTERIOR ACCENTS ──────────────────────────────────────────────
      {
        glm::vec3 accentCol(0.10f, 0.18f, 0.28f), railCol(0.62f, 0.65f, 0.68f);
        glm::vec3 curbCol(0.82f, 0.82f, 0.80f);
        box(-10.1f, 2.8f, 10.1f, 20.2f, 0.3f, -0.08f, accentCol);
        box(-10.2f, -1.0f, -20.2f, 0.4f, 7.0f, 0.4f, accentCol);
        box(10.1f, -1.0f, -20.2f, 0.4f, 7.0f, 0.4f, accentCol);
        box(-10.2f, -1.0f, 10.1f, 0.4f, 7.0f, 0.4f, accentCol);
        box(10.1f, -1.0f, 10.1f, 0.4f, 7.0f, 0.4f, accentCol);
        box(-4.0f, 9.1f, 5.0f, 14.0f, 0.12f, 0.08f, railCol);
        for (float rx = -3.8f; rx < 9.8f; rx += 0.7f)
          box(rx, 8.0f, 5.0f, 0.08f, 1.1f, 0.08f, railCol);
        box(-4.0f, 9.1f, -15.0f, 14.0f, 0.12f, 0.08f, railCol);
        for (float rx = -3.8f; rx < 9.8f; rx += 0.7f)
          box(rx, 8.0f, -15.0f, 0.08f, 1.1f, 0.08f, railCol);
        // Curb box around building: symmetric at x:[-11.5, 11.5],
        // z:[-21.5, 11.5]
        box(-11.5f, -1.0f, -21.50f, 23.0f, 0.15f, 0.5f, curbCol); // Back curb
        box(-11.5f, -1.0f, 11.0f, 23.0f, 0.15f, 0.5f, curbCol);   // Front curb
        box(-11.50f, -1.0f, -21.5f, 0.5f, 0.15f, 33.0f, curbCol); // Left curb
        box(11.0f, -1.0f, -21.5f, 0.5f, 0.15f, 33.0f, curbCol);   // Right curb
        glm::vec3 parkLine(0.90f, 0.90f, 0.88f);
        box(-2.0f, -0.985f, -20.5f, 0.12f, 0.01f, 7.0f, parkLine);
        box(2.0f, -0.985f, -20.5f, 0.12f, 0.01f, 7.0f, parkLine);
        box(-1.88f, -0.985f, -20.5f, 3.88f, 0.01f, 0.12f, parkLine);
        box(-1.88f, -0.985f, -13.5f, 3.88f, 0.01f, 0.12f, parkLine);
        box(2.15f, -0.985f, -20.5f, 0.12f, 0.01f, 7.0f, parkLine);
        box(5.5f, -0.985f, -20.5f, 0.12f, 0.01f, 7.0f, parkLine);
        box(2.27f, -0.985f, -20.5f, 3.23f, 0.01f, 0.12f, parkLine);
        box(2.27f, -0.985f, -13.5f, 3.23f, 0.01f, 0.12f, parkLine);

        // Simple car composed of boxes (non-textured)
        // auto drawCar = [&](float cx, float cz, glm::vec3 bodyCol, glm::vec3
        // roofCol) {
        //     float cy = -0.5f; // sits on the ground level used elsewhere
        //     // main body
        //     box(cx, cy, cz, 2.0f, 0.5f, 1.0f, bodyCol);
        //     // cabin/roof
        //     box(cx, cy + 0.5f, cz, 1.0f, 0.5f, 0.8f, roofCol);
        //     // wheels (small boxes)
        //     glm::vec3 wheelCol(0.05f, 0.05f, 0.05f);
        //     box(cx - 0.8f, cy - 0.4f, cz - 0.5f, 0.2f, 0.2f, 0.2f, wheelCol);
        //     box(cx + 0.8f, cy - 0.4f, cz - 0.5f, 0.2f, 0.2f, 0.2f, wheelCol);
        //     box(cx - 0.8f, cy - 0.4f, cz + 0.5f, 0.2f, 0.2f, 0.2f, wheelCol);
        //     box(cx + 0.8f, cy - 0.4f, cz + 0.5f, 0.2f, 0.2f, 0.2f, wheelCol);
        // };
        // place one car near the far-left road
        // drawCar(-25.0f, -10.0f, glm::vec3(0.8f, 0.1f, 0.1f), glm::vec3(0.95f,
        // 0.95f, 0.95f));
      }

      // ════════════════════════════════════════════════════════════════
      //  ENHANCED EXTERIOR: Bezier Curves, Fractal Trees, Grand Gate
      // ════════════════════════════════════════════════════════════════

      // ── COLONY & NEIGHBORHOOD GENERATION ────────────────────────────
      {
        srand(6789);
        glm::vec3 colRd(0.35f, 0.35f, 0.38f);
        // Distant Colony Roads
        box(-150.f, -1.01f, -80.f, 300.f, -0.05f, 12.0f, colRd); // Northern parallel road
        box(70.f, -1.01f, -150.f, 10.f, -0.05f, 300.0f, colRd);   // Eastern parallel road

        for(int i = 0; i < 25; i++) {
            float rx = (float)((rand() % 240) - 120);
            float rz = (float)((rand() % 240) - 120);
            
            // Keep strictly outside the apartment and central roads
            if (rx > -40.0f && rx < 40.0f && rz > -50.0f && rz < 40.0f) continue;
            
            // Sparse Nature (Normal Style)
            if (i % 2 == 0) {
                // Sphere Tree
                tree(rx, rz);
            } else {
                // Fractal Tree (The branching one at the gate)
                lightingShader.use();
                drawFractalTree(lightingShader, treeBark, treeLeaf, rx, -1.0f, rz, 2.5f, 0.18f, 3);
            }

            // Distant Houses
            if (i % 3 == 0) {
                drawNeighborhoodHouse(rx + 8.1f, rz + 8.1f, (int)(i * 123));
            }
        }
      }

      // ── BOUNDARY WALL ────────────────────────────────────────────
      {
        glm::vec3 brkC(0.72f, 0.35f, 0.22f);
        glm::vec3 capC(0.78f, 0.75f, 0.70f);
        float WH = 2.5f;
        // Symmetric boundary x:[-20.2, 20.2], z:[-30.2, 20.2]
        // Side walls (X-aligned boundaries)
        box(-20.37f, -1.f, -30.2f, 0.35f, WH, 21.2f, brkC); // Front-left
        box(-20.37f, -1.f, -3.f, 0.35f, WH, 23.2f,
            brkC); // Front-left with gate gap
        box(20.02f, -1.f, -30.2f, 0.35f, WH, 50.4f,
            brkC); // Right wall (symmetric space)

        // Front/Back walls (Z-aligned boundaries)
        box(-20.2f, -1.f, -30.37f, 40.57f, WH, 0.35f, brkC); // Back boundary
        box(-20.2f, -1.f, 20.02f, 40.57f, WH, 0.35f, brkC);  // Front boundary
        // Wall cap stones (Split the left cap to accommodate the gate gap)
        box(-20.42f, WH - 1.f, -30.2f, 0.45f, 0.15f, 21.2f,
            capC); // Front-left cap
        box(-20.42f, WH - 1.f, -3.f, 0.45f, 0.15f, 23.2f,
            capC); // Front-left cap (after gate)
        box(19.97f, WH - 1.f, -30.2f, 0.45f, 0.15f, 50.4f, capC);  // Right cap
        box(-20.2f, WH - 1.f, -30.42f, 40.4f, 0.15f, 0.45f, capC); // Back cap
        box(-20.2f, WH - 1.f, 19.97f, 40.4f, 0.15f, 0.45f, capC);  // Front cap

        // Corner post pillars (Shifted to symmetric corners)
        float cpx[] = {-20.37f, 20.02f};
        float cpz[] = {-30.37f, 20.02f};
        for (int ci = 0; ci < 2; ci++)
          for (int cj = 0; cj < 2; cj++)
            box(cpx[ci] - 0.15f, -1.f, cpz[cj] - 0.15f, 0.7f, WH + 0.8f, 0.7f,
                capC);
      }

      // ── GRAND ENTRANCE GATE ──────────────────────────────────────
      {
        glm::vec3 stnC(0.75f, 0.73f, 0.68f);
        glm::vec3 dkC(0.28f, 0.25f, 0.22f);
        glm::vec3 goldC(0.85f, 0.72f, 0.25f);
        glm::vec3 gwBrk(0.72f, 0.35f, 0.22f);
        float GX = -20.3f, GZ = -6.f;
        float GAP = 3.f, PW = 0.8f, PH = 5.5f;
        for (int s = -1; s <= 1; s += 2) {
          float pz = GZ + s * GAP;
          // Plinth base
          box(GX - 0.25f, -1.f, pz - PW * 0.65f, PW + 0.5f, 0.55f, PW * 1.3f,
              stnC);
          // Main shaft
          box(GX, -0.45f, pz - PW * 0.5f, PW, PH, PW, stnC);
          // Capital block
          box(GX - 0.15f, -0.45f + PH, pz - PW * 0.6f, PW + 0.3f, 0.35f,
              PW * 1.2f, stnC);
          // Decorative groove lines on shaft
          for (int g = -1; g <= 1; g++)
            box(GX - 0.02f, 0.f, pz + g * PW * 0.28f, 0.06f, PH - 1.5f, 0.06f,
                dkC);
          // Sphere finial on top
          {
            glm::mat4 ms =
                glm::translate(I, glm::vec3(GX + PW * 0.4f, PH + 0.15f, pz));
            ms = glm::scale(ms, glm::vec3(0.65f));
            gateFinial.drawSphere(lightingShader, ms);
          }
        }
        // Lintel beam spanning both pillars
        float lintY = PH - 0.45f;
        box(GX - 0.05f, lintY, GZ - GAP - PW * 0.5f, PW + 0.1f, 0.65f,
            GAP * 2.f + PW, stnC);
        // Keystone accent
        box(GX + 0.05f, lintY + 0.65f, GZ - 0.45f, PW * 0.7f, 0.45f, 0.9f,
            stnC);
        // Bezier arch ring decoration over gate opening
        lightingShader.use();
        {
          glm::mat4 am =
              glm::translate(I, glm::vec3(GX + PW * 0.4f, lintY + 0.1f, GZ));
          am = glm::scale(am, glm::vec3(0.4f, 0.45f, GAP * 0.85f));
          bezArch.draw(lightingShader, am, stnC);
        }
        // Name plate (gold on dark backing)
        box(GX - 0.02f, lintY + 0.12f, GZ - 1.5f, 0.08f, 0.42f, 3.f, dkC);
        box(GX - 0.03f, lintY + 0.17f, GZ - 1.3f, 0.06f, 0.32f, 2.6f, goldC);
        // Wing walls extending from gate pillars
        box(GX, -1.f, GZ - GAP - PW * 0.5f - 4.5f, 0.3f, 2.2f, 4.5f, gwBrk);
        box(GX, -1.f, GZ + GAP + PW * 0.5f, 0.3f, 2.2f, 4.5f, gwBrk);
      }

      // ── TILED ENTRANCE PATH (gate → building) ────────────────────
      {
        glm::vec3 tileC(0.82f, 0.82f, 0.80f);
        glm::vec3 groutC(0.62f, 0.60f, 0.58f);
        glm::vec3 kerbC(0.55f, 0.55f, 0.53f);
        float tY = -0.97f;
        for (float px = -19.5f; px < -10.8f; px += 1.3f)
          for (float pz = -8.f; pz < -3.8f; pz += 1.3f) {
            box(px, tY, pz, 1.2f, 0.04f, 1.2f, tileC);
            box(px - 0.04f, tY - 0.02f, pz - 0.04f, 1.28f, 0.02f, 1.28f,
                groutC);
          }
        // Kerb edges along path
        box(-19.5f, tY + 0.05f, -8.3f, 9.f, 0.1f, 0.2f, kerbC);
        box(-19.5f, tY + 0.05f, -3.6f, 9.f, 0.1f, 0.2f, kerbC);
        // Entrance landing platform
        box(-11.f, -0.96f, -8.5f, 1.4f, 0.06f, 5.f, tileC);
      }

      // ── ENHANCED ARCHITECTURAL PILASTERS (entrance flanking wall-attached pillars) ──────────
      {
        glm::vec3 plC(0.92f, 0.88f, 0.84f); // Elegant warm white/marble color
        float pzArr[] = {-8.5f, -3.5f, 1.f};
        
        for (int pi = 0; pi < 3; pi++) {
            // Base (thick lower block attached to wall)
            box(-10.25f, -1.0f, pzArr[pi] - 0.3f, 0.3f, 0.4f, 0.6f, plC);
            
            // Shaft (main pillar body smoothly attached to wall at -10.1)
            box(-10.2f, -0.6f, pzArr[pi] - 0.2f, 0.2f, 4.3f, 0.4f, plC);
            
            // Capital (Top Support block attached right below ceiling/balcony floor)
            box(-10.25f, 3.7f, pzArr[pi] - 0.3f, 0.3f, 0.3f, 0.6f, plC);
        }
      }

      // (Bezier Arch removed to clear floating elements)

      // (dome removed — was visible through doorway as dark cone)

      // ── BEZIER RAILING SPINDLES (2F balconies) ───────────────────
      lightingShader.use();
      {
        glm::vec3 rlC(0.55f, 0.52f, 0.48f);
        // North balcony (z = 5)
        for (float rx = -3.5f; rx < 9.5f; rx += 0.35f) {
          glm::mat4 rm = glm::translate(I, glm::vec3(rx, 8.f, 5.05f));
          rm = glm::scale(rm, glm::vec3(0.12f, 1.1f, 0.12f));
          bezRailing.draw(lightingShader, rm, rlC);
        }
        // South balcony (z = -15)
        for (float rx = -3.5f; rx < 9.5f; rx += 0.35f) {
          glm::mat4 rm = glm::translate(I, glm::vec3(rx, 8.f, -15.05f));
          rm = glm::scale(rm, glm::vec3(0.12f, 1.1f, 0.12f));
          bezRailing.draw(lightingShader, rm, rlC);
        }
      }

      // ── BEZIER DECORATIVE VASES flanking entrance ────────────────
      lightingShader.use();
      for (int vs = -1; vs <= 1; vs += 2) {
        glm::mat4 vm =
            glm::translate(I, glm::vec3(-10.7f, -1.f, -6.f + vs * 4.f));
        vm = glm::scale(vm, glm::vec3(0.45f, 1.4f, 0.45f));
        bezVase.draw(lightingShader, vm, glm::vec3(0.82f, 0.78f, 0.72f));
      }

      // ── FOUNTAIN in left yard ────────────────────────────────────
      {
        glm::vec3 mrb(0.88f, 0.86f, 0.82f), wtr(0.25f, 0.55f, 0.82f);
        float fx = -15.f, fz = 2.f, fr = 1.8f;
        // Basin walls (4 sides)
        box(fx - fr, -1.f, fz - fr, fr * 2.f, 0.55f, 0.25f, mrb);
        box(fx - fr, -1.f, fz + fr - 0.25f, fr * 2.f, 0.55f, 0.25f, mrb);
        box(fx - fr, -1.f, fz - fr, 0.25f, 0.55f, fr * 2.f, mrb);
        box(fx + fr - 0.25f, -1.f, fz - fr, 0.25f, 0.55f, fr * 2.f, mrb);
        // Water surface
        box(fx - fr + 0.25f, -0.5f, fz - fr + 0.25f, (fr - 0.25f) * 2.f, 0.05f,
            (fr - 0.25f) * 2.f, wtr);
        // Central pedestal + sphere
        box(fx - 0.15f, -1.f, fz - 0.15f, 0.3f, 2.f, 0.3f, mrb);
        box(fx - 0.25f, 0.8f, fz - 0.25f, 0.5f, 0.15f, 0.5f, mrb);
        {
          glm::mat4 ms = glm::translate(I, glm::vec3(fx, 1.3f, fz));
          ms = glm::scale(ms, glm::vec3(0.55f));
          gateFinial.drawSphere(lightingShader, ms);
        }

        // Animated Flowing Water!
        float hTime = glfwGetTime() * 6.0f;

        // Instead of static boxes, dynamically draw elegant water arcs using
        // small spheres
        Sphere waterDrop(0.5f, 8, 8, glm::vec3(0.4f, 0.8f, 0.95f),
                         glm::vec3(0.4f, 0.8f, 0.95f), glm::vec3(0.5f), 10.f);

        lightingShader.use();
        // Centered bouncing upward spout
        for (int i = 0; i < 4; i++) {
          float sy = 1.3f + (sinf(hTime - i) * 0.5f + 0.5f) *
                                0.5f; // Bounces between 1.3 and 1.8
          glm::mat4 wm = glm::translate(I, glm::vec3(fx, sy, fz));
          wm = glm::scale(wm, glm::vec3(0.1f, 0.15f, 0.1f));
          waterDrop.drawSphere(lightingShader, wm);
        }
        // Streams spreading outwards into pool over a parabolic arc
        for (int w = 0; w < 8; w++) {
          float wA = w * (3.14159f / 4.0f); // 8 streams outward radially
          for (int d = 0; d < 4; d++) {
            // Phase shift based on droplet depth
            float phase = hTime - d * 0.6f;
            float tVal = fmod(phase, 3.14159f); // Time scale 0 to pi

            // Parabolic trajectory mathematics
            float pathR = sinf(tVal) * 1.5f; // Horizontal radius outward
            float dx = cosf(wA) * pathR;
            float dz = sinf(wA) * pathR;
            // Gravity affecting height smoothly: goes up a bit, then falls
            float pathY = 1.6f + sinf(tVal) * 0.4f - tVal * 0.6f;

            if (pathY > -0.4f) { // only draw droplets that are visibly above
              // water surface
              glm::mat4 wm =
                  glm::translate(I, glm::vec3(fx + dx, pathY, fz + dz));
              wm = glm::scale(wm, glm::vec3(0.08f, 0.1f, 0.08f));
              waterDrop.drawSphere(lightingShader, wm);
            }
          }
        }
      }

      // ── BENCHES near fountain (2 benches, facing fountain) ───────
      {
        glm::vec3 wdB(0.45f, 0.28f, 0.12f), mtB(0.35f, 0.35f, 0.38f);
        // Bench facing +z (toward fountain, along z-axis)
        auto bnchZ = [&](float bx, float bz) {
          box(bx, -0.4f, bz, 0.5f, 0.12f, 2.f, wdB);
          box(bx, -1.f, bz, 0.1f, 0.6f, 0.1f, mtB);
          box(bx + 0.4f, -1.f, bz, 0.1f, 0.6f, 0.1f, mtB);
          box(bx, -1.f, bz + 1.9f, 0.1f, 0.6f, 0.1f, mtB);
          box(bx + 0.4f, -1.f, bz + 1.9f, 0.1f, 0.6f, 0.1f, mtB);
          box(bx + 0.5f, -0.28f, bz, 0.08f, 0.5f, 2.f, wdB);
        };
        // bnchZ(-17.2f, 1.f);   // left of fountain
        bnchZ(-12.5f, 1.f); // right of fountain
      }

      // ── LAMP POSTS (2 with sphere heads) ────────────────────────
      // ── LAMP POSTS (Realistic design, side of gate) ──────────────
      {
        glm::vec3 polC(0.20f, 0.20f, 0.22f); // Dark cast iron color
        Sphere lampBulb(0.5f, 16, 12, glm::vec3(1.f, 0.95f, 0.8f),
                        glm::vec3(1.f, 0.95f, 0.8f), glm::vec3(0.5f), 16.f);
        Cylinder poleCyl(0.5f, 12, 4, polC, polC, glm::vec3(0.3f), 16.f);

        auto lmp = [&](float lx, float lz) {
          // Pillar base
          box(lx - 0.2f, -1.f, lz - 0.2f, 0.4f, 0.5f, 0.4f, polC);
          box(lx - 0.15f, -0.5f, lz - 0.15f, 0.3f, 0.2f, 0.3f, polC);

          lightingShader.use();
          // Instead of the tricky bounding scale of `poleCyl` missing the floor
          // base, we use a precise bounding box coordinate for the center iron
          // cast iron pole.
          box(lx - 0.08f, -0.3f, lz - 0.08f, 0.16f, 4.7f, 0.16f, polC);

          // Lamp housing / base tray
          box(lx - 0.3f, 4.4f, lz - 0.3f, 0.6f, 0.1f, 0.6f, polC);
          box(lx - 0.2f, 4.5f, lz - 0.2f, 0.4f, 0.1f, 0.4f, polC);

          // Sphere lamp head
          glm::mat4 lm = glm::translate(I, glm::vec3(lx, 5.0f, lz));
          lm = glm::scale(lm, glm::vec3(0.7f));
          lampBulb.drawSphere(lightingShader, lm);

          // Lamp cap ring
          box(lx - 0.25f, 5.3f, lz - 0.25f, 0.5f, 0.1f, 0.5f, polC);
        };
        lmp(-21.5f, -9.f); // Left side of gate
        lmp(-21.5f, -3.f); // Right side of gate
      }

      // ── FACADE ACCENTS (window frames only on living room side) ──
      {
        glm::vec3 frmC(0.68f, 0.65f, 0.60f);
        glm::vec3 extGlassC(0.72f, 0.88f, 0.97f);
        // Window frames only along living room (z = 3..7, avoiding garage side)
        float wZs[] = {3.f, 5.f, 7.f};
        for (int wi = 0; wi < 3; wi++) {
          float wz = wZs[wi];
          // Frame block (opaque)
          box(-10.14f, 0.5f, wz, 0.18f, 2.f, 1.4f, frmC);

          // Windowsill
          box(-10.18f, 0.38f, wz - 0.1f, 0.25f, 0.12f, 1.6f, frmC);
        }
      }

      // ── FRACTAL TREES (new positions, complementing sphere trees) ─
      lightingShader.use();
      drawFractalTree(lightingShader, treeBark, treeLeaf, -18.f, -1.f, -20.f,
                      3.f, 0.3f, 4);
      drawFractalTree(lightingShader, treeBark, treeLeaf, -18.f, -1.f, 7.f,
                      3.5f, 0.35f, 4);
      drawFractalTree(lightingShader, treeBark, treeLeaf, 15.f, -1.f, 5.f, 3.f,
                      0.3f, 4);

      // ── CLOUDS (ANIMATED – floating drift across the sky) ──────────
      lightingShader.use();
      {
        Sphere cldSph(0.5f, 16, 12, glm::vec3(1.f, 1.f, 1.f),
                      glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.3f, 0.3f, 0.3f),
                      4.f);
        auto cloudPuff = [&](float cx, float cy, float cz, float rx, float ry,
                             float rz) {
          glm::mat4 cm = glm::translate(I, glm::vec3(cx, cy, cz));
          cm = glm::scale(cm, glm::vec3(rx, ry, rz));
          cldSph.drawSphere(lightingShader, cm);
        };

        auto fluffyCloud = [&](float ox, float oy, float oz, float s) {
          // Wide, horizontally squashed spheres for realistic scattered
          // cirrocumulus
          cloudPuff(ox, oy, oz, 7.0f * s, 2.0f * s, 5.0f * s); // Core
          cloudPuff(ox + 4.0f * s, oy + 0.5f * s, oz + 1.5f * s, 5.5f * s,
                    1.8f * s, 4.0f * s);
          cloudPuff(ox - 3.5f * s, oy - 0.5f * s, oz - 1.0f * s, 6.0f * s,
                    1.5f * s, 3.5f * s);
          cloudPuff(ox + 2.5f * s, oy - 1.0f * s, oz - 2.0f * s, 4.0f * s,
                    1.2f * s, 3.0f * s);
          cloudPuff(ox - 2.5f * s, oy + 0.8f * s, oz + 1.5f * s, 4.5f * s,
                    1.6f * s, 3.5f * s);
        };

        // Each cloud drifts with unique speed/phase for organic parallax
        float t = cloudDriftTime;
        // Cloud 1: slow drift +X, gentle bob
        float c1x = -8.f + sinf(t * 0.12f) * 15.f;
        float c1z = -10.f + cosf(t * 0.08f) * 8.f;
        float c1y = 27.f + sinf(t * 0.15f) * 1.2f;
        fluffyCloud(c1x, c1y, c1z, 1.1f);

        // Cloud 2: drift -X, different phase
        float c2x = 22.f + cosf(t * 0.10f + 1.5f) * 18.f;
        float c2z = 5.f + sinf(t * 0.06f + 0.8f) * 10.f;
        float c2y = 32.f + cosf(t * 0.18f + 2.f) * 0.8f;
        fluffyCloud(c2x, c2y, c2z, 0.9f);

        // Cloud 3: large slow mover
        float c3x = -25.f + sinf(t * 0.07f + 3.1f) * 20.f;
        float c3z = 18.f + cosf(t * 0.09f + 1.2f) * 12.f;
        float c3y = 25.f + sinf(t * 0.12f + 0.5f) * 1.5f;
        fluffyCloud(c3x, c3y, c3z, 1.3f);

        // Cloud 4: small fast drifter
        float c4x = 15.f + cosf(t * 0.15f + 4.7f) * 12.f;
        float c4z = -25.f + sinf(t * 0.11f + 2.3f) * 8.f;
        float c4y = 30.f + cosf(t * 0.20f + 1.f) * 0.6f;
        fluffyCloud(c4x, c4y, c4z, 0.8f);

        // Cloud 5: extra cloud for fuller sky
        float c5x = 0.f + sinf(t * 0.09f + 5.2f) * 22.f;
        float c5z = 30.f + cosf(t * 0.07f + 3.8f) * 14.f;
        float c5y = 28.f + sinf(t * 0.14f + 2.5f) * 1.0f;
        fluffyCloud(c5x, c5y, c5z, 1.0f);
      }

      // ── ANIMATED FLYING BIRDS ─────────────────────────────────────
      {
        birdTime += deltaTime;
        glm::vec3 birdC(0.12f, 0.10f, 0.08f);
        auto drawBird = [&](float phase, float radius, float height,
                            float speed) {
          float angle = birdTime * speed + phase;
          float bx = radius * cosf(angle);
          float bz = radius * sinf(angle);
          float by = height + 0.5f * sinf(birdTime * 2.f + phase);
          // Body
          box(bx - 0.15f, by, bz - 0.05f, 0.3f, 0.08f, 0.1f, birdC);
          // Left wing (flapping)
          float wingFlap = 0.15f * sinf(birdTime * 8.f + phase);
          box(bx - 0.5f, by + wingFlap, bz - 0.03f, 0.35f, 0.03f, 0.06f, birdC);
          // Right wing (flapping)
          box(bx + 0.15f, by + wingFlap, bz - 0.03f, 0.35f, 0.03f, 0.06f,
              birdC);
        };
        drawBird(0.f, 12.f, 18.f, 0.4f);
        drawBird(1.57f, 15.f, 20.f, 0.35f);
        drawBird(3.14f, 10.f, 16.f, 0.5f);
        drawBird(4.71f, 18.f, 22.f, 0.3f);
        drawBird(0.8f, 8.f, 15.f, 0.45f);
      }

      // ── CAR DRAWING ABSTRACTION ───────────────────────────────────────
      auto drawCarShape = [&](glm::mat4 baseM) {
        glm::vec3 bodyC(0.12f, 0.12f, 0.72f);  // blue/red swap depending on use
        glm::vec3 roofC(0.20f, 0.20f, 0.22f);  // dark roof
        glm::vec3 glassC(0.45f, 0.60f, 0.75f); // windshield
        glm::vec3 chromeC(0.78f, 0.78f, 0.80f);
        glm::vec3 blackC(0.08f, 0.08f, 0.08f);
        glm::vec3 headC(1.f, 0.95f, 0.7f);  // headlights
        glm::vec3 tailC(0.9f, 0.1f, 0.05f); // taillights
        glm::vec3 hubC(0.65f, 0.65f, 0.68f);

        auto cbox = [&](float lx, float ly, float lz, float sx, float sy, float sz, glm::vec3 c) {
          float px = sx < 0 ? lx + sx : lx;
          float py = sy < 0 ? ly + sy : ly;
          float pz = sz < 0 ? lz + sz : lz;
          glm::mat4 bm = glm::translate(baseM, glm::vec3(px, py, pz));
          bm = glm::scale(bm, glm::vec3(fabs(sx), fabs(sy), fabs(sz)));
          lightingShader.use();
          lightingShader.setVec3("material.ambient", c * 0.35f);
          lightingShader.setVec3("material.diffuse", c);
          lightingShader.setVec3("material.specular", glm::vec3(0.5f));
          lightingShader.setFloat("material.shininess", 64.f);
          lightingShader.setMat4("model", bm);
          glBindVertexArray(VAO);
          glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        };

        // Main body lower (chassis)
        cbox(-1.8f, 0.f, -0.8f, 3.6f, 0.55f, 1.6f, bodyC);
        // Hood & Cabin & Trunk
        cbox(-1.8f, 0.55f, -0.7f, 0.9f, 0.2f, 1.4f, bodyC);
        cbox(-0.7f, 0.55f, -0.65f, 1.8f, 0.7f, 1.3f, roofC);
        cbox(1.1f, 0.55f, -0.7f, 0.7f, 0.2f, 1.4f, bodyC);
        cbox(-0.5f, 1.25f, -0.6f, 1.3f, 0.08f, 1.2f, roofC);

        // Windows
        cbox(-0.72f, 0.7f, -0.5f, 0.08f, 0.5f, 1.0f, glassC);
        cbox(1.08f, 0.7f, -0.5f, 0.08f, 0.5f, 1.0f, glassC);
        cbox(-0.6f, 0.75f, -0.66f, 1.5f, 0.4f, 0.04f, glassC);
        cbox(-0.6f, 0.75f, 0.62f, 1.5f, 0.4f, 0.04f, glassC);

        // Bumpers & Lights
        cbox(-1.88f, -0.05f, -0.85f, 0.15f, 0.25f, 1.7f, chromeC);
        cbox(1.75f, -0.05f, -0.85f, 0.15f, 0.25f, 1.7f, chromeC);
        cbox(-1.84f, 0.2f, -0.55f, 0.06f, 0.15f, 0.25f, headC);
        cbox(-1.84f, 0.2f, 0.3f, 0.06f, 0.15f, 0.25f, headC);
        cbox(1.82f, 0.2f, -0.55f, 0.06f, 0.15f, 0.25f, tailC);
        cbox(1.82f, 0.2f, 0.3f, 0.06f, 0.15f, 0.25f, tailC);

        // Mirrors & Details
        cbox(-0.5f, 0.85f, -0.9f, 0.12f, 0.1f, 0.15f, blackC);
        cbox(-0.5f, 0.85f, 0.75f, 0.12f, 0.1f, 0.15f, blackC);
        cbox(-0.1f, 0.05f, -0.82f, 0.04f, 0.5f, 0.04f, blackC);
        cbox(-0.1f, 0.05f, 0.78f, 0.04f, 0.5f, 0.04f, blackC);

        // Wheels (layered tyre + rim + hubs for clearer look)
        float wxArr[] = {-1.2f, 1.2f};
        float wzArr[] = {-0.85f, 0.55f};
        for (int wi = 0; wi < 2; wi++) {
          for (int wj = 0; wj < 2; wj++) {
            float wx = wxArr[wi];
            float wz = wzArr[wj];

            // Tyre body (thicker + taller)
            cbox(wx - 0.24f, -0.22f, wz - 0.11f, 0.48f, 0.44f, 0.22f, blackC);
            // Tyre sidewalls (slightly lighter for shape readability)
            cbox(wx - 0.20f, -0.18f, wz - 0.115f, 0.40f, 0.36f, 0.03f,
                 glm::vec3(0.12f, 0.12f, 0.12f));
            cbox(wx - 0.20f, -0.18f, wz + 0.085f, 0.40f, 0.36f, 0.03f,
                 glm::vec3(0.12f, 0.12f, 0.12f));

            // Rims (both sides)
            cbox(wx - 0.16f, -0.14f, wz - 0.105f, 0.32f, 0.28f, 0.02f,
                 glm::vec3(0.70f, 0.71f, 0.74f));
            cbox(wx - 0.16f, -0.14f, wz + 0.085f, 0.32f, 0.28f, 0.02f,
                 glm::vec3(0.70f, 0.71f, 0.74f));

            // Hub caps (both sides)
            cbox(wx - 0.07f, -0.05f, wz - 0.11f, 0.14f, 0.14f, 0.03f, hubC);
            cbox(wx - 0.07f, -0.05f, wz + 0.08f, 0.14f, 0.14f, 0.03f, hubC);

            // Simple cross spokes so wheels don't look flat
            cbox(wx - 0.015f, -0.12f, wz - 0.108f, 0.03f, 0.24f, 0.01f,
                 glm::vec3(0.78f, 0.79f, 0.80f));
            cbox(wx - 0.12f, -0.015f, wz - 0.108f, 0.24f, 0.03f, 0.01f,
                 glm::vec3(0.78f, 0.79f, 0.80f));
            cbox(wx - 0.015f, -0.12f, wz + 0.088f, 0.03f, 0.24f, 0.01f,
                 glm::vec3(0.78f, 0.79f, 0.80f));
            cbox(wx - 0.12f, -0.015f, wz + 0.088f, 0.24f, 0.03f, 0.01f,
                 glm::vec3(0.78f, 0.79f, 0.80f));
          }
        }
      };

      // ── INTERACTIVE CAR on road ───────────────────────────────────────
      {
        glm::mat4 carBase = glm::translate(I, glm::vec3(carX, -1.f, carZ));
        carBase = glm::rotate(carBase, glm::radians(carAngle), glm::vec3(0, 1, 0));
        drawCarShape(carBase);
      }

      // ── PARKED CAR in Garage ──────────────────────────────────────────
      {
        // Parked elegantly in the exact center of the garage (width: x ≈ -10 to -4 | depth: z ≈ -19 to -13)
        // With 0 degree rotation, its headlights face directly OUT of the garage (-X direction point exactly to the gate)
        glm::mat4 parkedCar = glm::translate(I, glm::vec3(-7.0f, -1.0f, -16.0f));
        parkedCar = glm::scale(parkedCar, glm::vec3(1.25f)); // Increased scale as requested
        drawCarShape(parkedCar);
      }

      // ── DRAW TRANSPARENT WINDOWS LAST ──────────────────────────────
      glm::vec4 winGlass(0.70f, 0.87f, 0.98f,
                         0.4f); // semi-transparent sky blue
      setupTexShader();         // Reset to lighting without texture
      lightingShader.use();
      lightingShader.setVec3("material.ambient", glm::vec3(winGlass) * 0.5f);
      lightingShader.setVec3("material.diffuse", glm::vec3(winGlass));
      lightingShader.setVec3("material.specular", glm::vec3(1.0f));
      lightingShader.setFloat("material.shininess", 128.f);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDepthMask(GL_FALSE); // Disable depth writes for transparency

      auto drawWin = [&](float x, float y, float z, float sx, float sy,
                         float sz) {
        glm::mat4 mw = glm::translate(I, glm::vec3(x, y, z));
        mw = glm::scale(mw, glm::vec3(sx, sy, sz));
        lightingShader.setMat4("model", mw);
        lightingShader.setFloat("alpha", 0.45f);
        drawCube(VAO, lightingShader, mw, glm::vec3(0.52f, 0.78f, 0.97f));
        lightingShader.setFloat("alpha", 1.0f); // Reset
      };

      // Living room transparent glass windows:
      float wZglass[] = {3.f, 5.f, 7.f};
      for(int wi = 0; wi < 3; wi++) {
        drawWin(-10.12f, 0.6f, wZglass[wi] + 0.1f, 0.06f, 1.8f, 1.2f);
      }

      glDepthMask(GL_TRUE); // Re-enable depth writes
      glDisable(GL_BLEND);

      // ── EMISSIVE BULB MARKERS ─────────────────────────────────────────
      ourShader.use();
      ourShader.setMat4("projection", proj_);
      ourShader.setMat4("view", view_);
      glBindVertexArray(lightCubeVAO);
      auto bulb = [&](glm::vec3 pos, glm::vec3 col) {
        glm::mat4 lm = glm::translate(I, pos);
        lm = glm::scale(lm, glm::vec3(0.3f, -0.3f, 0.3f));
        ourShader.setMat4("model", lm);
        ourShader.setVec3("color", col);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
      };
      bulb(ptPos[0], glm::vec3(1.0f, 0.6f, 0.3f));
      bulb(ptPos[1], glm::vec3(0.4f, 0.6f, 1.0f));
      bulb(ptPos[2], lampGlow2);
      bulb(ptPos[3], lampGlow2);
      {
        glm::mat4 lm = glm::translate(I, glm::vec3(-6.0f, 3.3f, 3.5f));
        lm = glm::scale(lm, glm::vec3(0.35f, -0.25f, 0.35f));
        ourShader.setMat4("model", lm);
        ourShader.setVec3("color", glm::vec3(1.0f, 0.2f, 1.0f));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
      }
    }; // end setupAndRender

    // ════════════════════════════════════════════════════════════════════
    //  Viewport dispatch (same as B1)
    // ════════════════════════════════════════════════════════════════════
      glm::mat4 perspFull =
        myProjection(glm::radians(60.0f), fullAspect, 0.8f, 120.0f);

    if (!splitViewport) {
      glViewport(0, 0, winW, winH);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      // Draw skybox first
      glDepthFunc(GL_LEQUAL);
      skyboxShader.use();
      glm::mat4 view_no_translate = glm::mat4(glm::mat3(viewInside));
      skyboxShader.setMat4("view", view_no_translate);
      skyboxShader.setMat4("projection", perspFull);
      glBindVertexArray(skyboxVAO);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
      glDrawArrays(GL_TRIANGLES, 0, 36);
      glBindVertexArray(0);
      glDepthFunc(GL_LESS);

      setupAndRender(viewInside, perspFull, camera.Position, directionLightOn,
                     point1On || point2On, spotLightOn, false, false, false);
    } else {
      int vpW = winW / 2, vpH = winH / 2;
      float splitAspect = (float)vpW / (float)vpH;
      glm::mat4 perspSplit =
          myProjection(glm::radians(60.0f), splitAspect, 0.8f, 120.0f);
      glm::mat4 orthoSplit =
          glm::ortho(-20.0f, 20.0f, -15.0f, 15.0f, 0.8f, 120.0f);
      glm::mat4 viewIso =
          glm::lookAt(glm::vec3(25, 20, 25), glm::vec3(0), glm::vec3(0, 1, 0));
      glm::mat4 viewTop =
          glm::lookAt(glm::vec3(0, 30, 0), glm::vec3(0), glm::vec3(0, 0, -1));
      glm::mat4 viewFront = glm::lookAt(
          glm::vec3(-38, 4, -5), glm::vec3(0, 3, -5), glm::vec3(0, 1, 0));
      glm::mat4 viewFixedInside =
          glm::lookAt(glm::vec3(-8.5f, 2.5f, 9), glm::vec3(0, 1.5f, -5),
                      glm::vec3(0, 1, 0));

      glEnable(GL_SCISSOR_TEST);
      // Top-left: combined
      glViewport(0, vpH, vpW, vpH);
      glScissor(0, vpH, vpW, vpH);
      glClearColor(0.45f, 0.70f, 0.92f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      setupAndRender(viewIso, perspSplit, glm::vec3(25, 20, 25),
                     directionLightOn, point1On || point2On, spotLightOn, false,
                     false, false);
      // Top-right: ambient
      glViewport(vpW, vpH, vpW, vpH);
      glScissor(vpW, vpH, vpW, vpH);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      // skybox for top-right
      glDepthFunc(GL_LEQUAL);
      skyboxShader.use();
      glm::mat4 view_no_translate_top = glm::mat4(glm::mat3(viewTop));
      skyboxShader.setMat4("view", view_no_translate_top);
      skyboxShader.setMat4("projection", perspSplit);
      glBindVertexArray(skyboxVAO);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
      glDrawArrays(GL_TRIANGLES, 0, 36);
      glBindVertexArray(0);
      glDepthFunc(GL_LESS);

      setupAndRender(viewTop, orthoSplit, glm::vec3(0, 30, 0), directionLightOn,
                     false, false, true, false, false);
      // Bottom-left: diffuse
      glViewport(0, 0, vpW, vpH);
      glScissor(0, 0, vpW, vpH);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      // skybox for bottom-left
      glDepthFunc(GL_LEQUAL);
      skyboxShader.use();
      glm::mat4 view_no_translate_front = glm::mat4(glm::mat3(viewFront));
      skyboxShader.setMat4("view", view_no_translate_front);
      skyboxShader.setMat4("projection", perspSplit);
      glBindVertexArray(skyboxVAO);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
      glDrawArrays(GL_TRIANGLES, 0, 36);
      glBindVertexArray(0);
      glDepthFunc(GL_LESS);

      setupAndRender(viewFront, perspSplit, glm::vec3(-38, 4, -5),
                     directionLightOn, false, false, false, true, false);
      // Bottom-right: directional
      glViewport(vpW, 0, vpW, vpH);
      glScissor(vpW, 0, vpW, vpH);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      // skybox for bottom-right
      glDepthFunc(GL_LEQUAL);
      skyboxShader.use();
      glm::mat4 view_no_translate_fixed = glm::mat4(glm::mat3(viewFixedInside));
      skyboxShader.setMat4("view", view_no_translate_fixed);
      skyboxShader.setMat4("projection", perspSplit);
      glBindVertexArray(skyboxVAO);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
      glDrawArrays(GL_TRIANGLES, 0, 36);
      glBindVertexArray(0);
      glDepthFunc(GL_LESS);

      setupAndRender(viewFixedInside, perspSplit, glm::vec3(-8.5f, 2.5f, 9),
                     directionLightOn, false, false, false, false, true);
      glDisable(GL_SCISSOR_TEST);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &VAO);
  glDeleteVertexArrays(1, &lightCubeVAO);
  glDeleteVertexArrays(1, &tVAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteBuffers(1, &tVBO);
  glDeleteBuffers(1, &tEBO);
  glfwTerminate();
  return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  drawCube – same as B1
// ─────────────────────────────────────────────────────────────────────────────
void drawCube(unsigned int &VAO, Shader &s, glm::mat4 model, glm::vec3 color) {
  s.use();
  s.setVec3("material.ambient", color);
  s.setVec3("material.diffuse", color);
  s.setVec3("material.specular", glm::vec3(0.4f));
  s.setFloat("material.shininess", 48.0f);
  s.setMat4("model", model);
  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  processInput
// ─────────────────────────────────────────────────────────────────────────────
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // ── Speed multiplier: hold LEFT SHIFT for 4× fast movement ──────────────
  float speedMult =
      (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 4.0f : 1.0f;
  float dt = deltaTime * speedMult;

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, dt);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, dt);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, dt);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, dt);
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    camera.ProcessKeyboard(UP, dt);
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    camera.ProcessKeyboard(DOWN, dt);
  if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
    camera.ProcessKeyboard(P_UP, dt);
  if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
    camera.ProcessKeyboard(P_DOWN, dt);
  if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
    camera.ProcessKeyboard(Y_RIGHT, dt);
  if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
    camera.ProcessKeyboard(Y_LEFT, dt);
  if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
    camera.ProcessKeyboard(R_LEFT, dt);
  if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
    camera.ProcessKeyboard(R_RIGHT, dt);

  if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    openDoor = true;
  if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    openDoor = false;

  // Toggle interactive fridge door
  static bool kX = false;
  if ((glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS ||
       glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) &&
      !kX) {
    openFridge = !openFridge;
    kX = true;
  }
  if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE &&
      glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
    kX = false;

  // Toggle bathroom tap water (key 4)
  static bool k4 = false;
  if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !k4) {
    tapOn = !tapOn;
    cout << "Bathroom Tap: " << (tapOn ? "ON (water flowing)" : "OFF") << "\n";
    k4 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE)
    k4 = false;

  // Toggle AC (key 8)
  static bool k8 = false;
  if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS && !k8) {
    acOn = !acOn;
    cout << "AC: " << (acOn ? "ON (cooling)" : "OFF") << "\n";
    k8 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_8) == GLFW_RELEASE)
    k8 = false;

  // Toggle table lamp (key 0)
  static bool k0 = false;
  if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS && !k0) {
    tableLampOn = !tableLampOn;
    cout << "Table Lamp: " << (tableLampOn ? "ON (warm glow)" : "OFF") << "\n";
    k0 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_0) == GLFW_RELEASE)
    k0 = false;

  // ── ROOM TELEPORT HOTKEYS ("Room dekhai... akdom baire dekhai") ──
  // Helper to instantly snap the camera position & angle
  auto jumpCam = [](glm::vec3 pos, float yaw, float pitch) {
    camera.Position = pos;
    camera.Yaw = yaw;
    camera.Pitch = pitch;
    camera.ProcessKeyboard(FORWARD, 0.0f); // flush vectors
  };

  if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) jumpCam(glm::vec3(-4.0f, 1.6f, 4.0f), -90.0f, 0.0f);   // GF TV Room
  if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) jumpCam(glm::vec3(-5.0f, 1.6f, -14.0f), -90.0f, 0.0f); // GF Bedroom
  if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) jumpCam(glm::vec3(5.0f, 1.6f, -15.0f), 0.0f, 0.0f);    // GF Kitchen
  if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS) jumpCam(glm::vec3(4.0f, 1.6f, 4.0f), -90.0f, 0.0f);    // GF Dining
  if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) jumpCam(glm::vec3(-1.0f, 5.6f, -10.0f), 0.0f, -5.0f);  // TF Bedroom
  if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS) jumpCam(glm::vec3(-1.0f, 5.6f, 2.0f), -90.0f, -5.0f);  // TF Bathroom
  if (glfwGetKey(window, GLFW_KEY_F7) == GLFW_PRESS) jumpCam(glm::vec3(-23.0f, 12.0f, 25.0f), -50.0f, -15.0f); // Akdom Baire (Full Outside)

  // Toggle autonomous car drive
  static bool kV = false;
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !kV) {
    carAuto = !carAuto;
    kV = true;
  }
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE)
    kV = false;

  // Lift control (L key)
  static bool kL = false;
  if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !kL) {
    liftUp = !liftUp;
    kL = true;
  }
  if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE)
    kL = false;

  static bool kG = false, k1 = false, k2 = false, k3 = false, k5 = false,
              k6 = false, k7 = false, k9 = false, kT = false, kB = false,
              kC = false, kF = false, kR = false, kZ = false;

  auto snapTo = [&](glm::vec3 pos, float yaw, float pitch) {
    camera.Position = pos;
    camera.Yaw = yaw;
    camera.Pitch = pitch;
    camera.Roll = 0.0f;
    camera.ProcessKeyboard(FORWARD, 0.0f); // forces vector refresh
  };

  // G – fan toggle
  if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !kG) {
    fanOn = !fanOn;
    kG = true;
  }
  if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE)
    kG = false;

  // 1 – directional light
  if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !k1) {
    directionLightOn = !directionLightOn;
    k1 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE)
    k1 = false;

  // 2 – point lights
  if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !k2) {
    point1On = !point1On;
    point2On = point1On;
    if (point1On) {
      pointlight1.turnOn();
      pointlight2.turnOn();
    } else {
      pointlight1.turnOff();
      pointlight2.turnOff();
    }
    k2 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE)
    k2 = false;

  // 3 – spot light
  if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !k3) {
    spotLightOn = !spotLightOn;
    cout << "Spot light: " << (spotLightOn ? "ON" : "OFF") << "\n";
    k3 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE)
    k3 = false;

  // 5 – ambient
  if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && !k5) {
    ambientOn = !ambientOn;
    k5 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE)
    k5 = false;

  // 6 – diffuse
  if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS && !k6) {
    diffuseOn = !diffuseOn;
    k6 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_6) == GLFW_RELEASE)
    k6 = false;

  // 7 – specular
  if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS && !k7) {
    specularOn = !specularOn;
    cout << "Specular light: " << (specularOn ? "ON" : "OFF") << "\n";
    k7 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_7) == GLFW_RELEASE)
    k7 = false;

  // 9 – split viewport
  if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS && !k9) {
    splitViewport = !splitViewport;
    cout << "Viewport: " << (splitViewport ? "4-SPLIT" : "FULL SCREEN") << "\n";
    k9 = true;
  }
  if (glfwGetKey(window, GLFW_KEY_9) == GLFW_RELEASE)
    k9 = false;

  // T – cycle texture mode
  if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !kT) {
    textureMode = (textureMode + 1) % 4;
    const char *modeNames[] = {
        "0 = No Texture", "1 = Simple Texture (pure texture)",
        "2 = Vertex-Blended Texture", "3 = Fragment-Blended Texture"};
    cout << "Texture mode: " << modeNames[textureMode] << "\n";
    kT = true;
  }
  if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
    kT = false;

  // ── Quick room views ─────────────────────────────────────────────────────
  // B = TV room, C = Dining room, F = Bedroom (ground), R = Top floor view
  if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !kB) {
    snapTo(glm::vec3(-6.0f, 1.8f, -6.0f), -90.0f,
           0.0f); // look toward TV wall (z-)
    kB = true;
  }
  if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    kB = false;

  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !kC) {
    snapTo(glm::vec3(-4.0f, 1.8f, 2.5f), 90.0f,
           0.0f); // look toward dining wall (z+)
    kC = true;
  }
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE)
    kC = false;

  if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !kF) {
    snapTo(glm::vec3(6.8f, 1.8f, -10.2f), 180.0f,
           0.0f); // face toward left bedroom area
    kF = true;
  }
  if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
    kF = false;

  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !kR) {
    snapTo(glm::vec3(0.0f, 8.2f, -5.0f), -90.0f,
           -10.0f); // elevated top-floor view
    kR = true;
  }
  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE)
    kR = false;

  // Z = Bedroom (ground) quick view (same as F)
  if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS && !kZ) {
    snapTo(glm::vec3(6.8f, 1.8f, -10.2f), 180.0f, 0.0f);
    kZ = true;
  }
  if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE)
    kZ = false;

  // ── Interactive car controls (arrow keys) + AUTONOMOUS ───────────────────
  if (carAuto) {
    carSpeed = 8.0f;
    // Auto-pilot around the block counter-clockwise
    float rad = glm::radians(carAngle);
    carX += carSpeed * cosf(rad) * dt;
    carZ += carSpeed * sinf(rad) * dt;

    // Rectangular path logic updated for the new symmetric layout
    if (carAngle == 90.f && carZ > 25.2f) {
      carZ = 25.2f;
      carAngle = 0.f;
    } // Going down left road, turn to front road
    else if (carAngle == 0.f && carX > 25.2f) {
      carX = 25.2f;
      carAngle = -90.f;
    } // Going across front road, turn to right road
    else if (carAngle == -90.f && carZ < -35.2f) {
      carZ = -35.2f;
      carAngle = 180.f;
    } // Going up right road, turn to back road
    else if (carAngle == 180.f && carX < -25.2f) {
      carX = -25.2f;
      carAngle = 90.f;
    } // Going across back road, turn to left road

    // Break out of auto if manual keys are pressed
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
      carAuto = false;
    }
  } else {
    float carAccel = 6.f * dt;
    float carTurn = 90.f * dt;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
      carSpeed += carAccel;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
      carSpeed -= carAccel;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
      carAngle -= carTurn;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
      carAngle += carTurn;

    carSpeed *= 0.97f; // Friction
    // Clamp speed
    if (carSpeed > 12.f)
      carSpeed = 12.f;
    if (carSpeed < -6.f)
      carSpeed = -6.f;

    // Move car
    float rad = glm::radians(carAngle);
    carX += carSpeed * cosf(rad) * dt;
    carZ += carSpeed * sinf(rad) * dt;
  }
}

void framebuffer_size_callback(GLFWwindow *, int w, int h) {
  glViewport(0, 0, w, h);
}
void scroll_callback(GLFWwindow *, double, double yoffset) {
  camera.ProcessMouseScroll((float)yoffset);
}