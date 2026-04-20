#ifndef SPHERE_H
#define SPHERE_H

#include "shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#define PI 3.1416

using namespace std;

const int MIN_SECTOR_COUNT = 3;
const int MIN_STACK_COUNT = 2;

class Sphere {
public:
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  float shininess;

  // Constructor
  Sphere(float radius = 1.0f, int sectorCount = 36, int stackCount = 18,
         glm::vec3 amb = glm::vec3(1.0f, 0.0f, 0.0f),
         glm::vec3 diff = glm::vec3(1.0f, 0.0f, 0.0f),
         glm::vec3 spec = glm::vec3(0.5f, 0.5f, 0.5f), float shiny = 32.0f)
      : verticesStride(32) {
    set(radius, sectorCount, stackCount, amb, diff, spec, shiny);
    buildCoordinatesAndIndices();
    buildVertices();

    // VAO for sphere without texture
    glGenVertexArrays(1, &sphereVAO_NoTexture);
    glBindVertexArray(sphereVAO_NoTexture);

    unsigned int sphereVBO;
    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, getVertexSizeWithoutTexture(),
                 getVerticesWithoutTexture(), GL_STATIC_DRAW);

    unsigned int sphereEBO;
    glGenBuffers(1, &sphereEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, getIndexSize(), getIndices(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // Position
    glEnableVertexAttribArray(1); // Normal

    int stride = getVerticesStride();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)(sizeof(float) * 3));

    glBindVertexArray(0);

    // VAO for sphere with texture
    glGenVertexArrays(1, &sphereVAO_WithTexture);
    glBindVertexArray(sphereVAO_WithTexture);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, getVertexSize(), getVertices(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, getIndexSize(), getIndices(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // Position
    glEnableVertexAttribArray(1); // Normal
    glEnableVertexAttribArray(2); // Texture coordinates

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)(sizeof(float) * 3));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          (void *)(sizeof(float) * 6));

    glBindVertexArray(0);
  }

  ~Sphere() {}

  // Setters
  void set(float radius, int sectors, int stacks, glm::vec3 amb, glm::vec3 diff,
           glm::vec3 spec, float shiny) {
    if (radius > 0)
      this->radius = radius;
    this->sectorCount = sectors;
    if (sectors < MIN_SECTOR_COUNT)
      this->sectorCount = MIN_SECTOR_COUNT;
    this->stackCount = stacks;
    if (stacks < MIN_STACK_COUNT)
      this->stackCount = MIN_STACK_COUNT;
    this->ambient = amb;
    this->diffuse = diff;
    this->specular = spec;
    this->shininess = shiny;
  }

  void setRadius(float radius) {
    if (radius != this->radius)
      set(radius, sectorCount, stackCount, ambient, diffuse, specular,
          shininess);
  }

  void setSectorCount(int sectors) {
    if (sectors != this->sectorCount)
      set(radius, sectors, stackCount, ambient, diffuse, specular, shininess);
  }

  void setStackCount(int stacks) {
    if (stacks != this->stackCount)
      set(radius, sectorCount, stacks, ambient, diffuse, specular, shininess);
  }

  unsigned int getVertexCount() const {
    return (unsigned int)coordinates.size() / 3;
  }

  unsigned int getVertexSizeWithoutTexture() const {
    return (unsigned int)verticesWithoutTexture.size() * sizeof(float);
  }

  unsigned int getVertexSize() const {
    return (unsigned int)vertices.size() * sizeof(float);
  }

  int getVerticesStride() const { return verticesStride; }

  const float *getVertices() const { return vertices.data(); }

  const float *getVerticesWithoutTexture() const {
    return verticesWithoutTexture.data();
  }

  unsigned int getIndexSize() const {
    return (unsigned int)indices.size() * sizeof(unsigned int);
  }

  const unsigned int *getIndices() const { return indices.data(); }

  unsigned int getIndexCount() const { return (unsigned int)indices.size(); }

  // Draw sphere without texture
  void drawSphere(Shader &shader, glm::mat4 model) const {
    shader.use();
    shader.setVec3("material.ambient", ambient);
    shader.setVec3("material.diffuse", diffuse);
    shader.setVec3("material.specular", specular);
    shader.setFloat("material.shininess", shininess);
    shader.setMat4("model", model);

    glBindVertexArray(sphereVAO_NoTexture);
    glDrawElements(GL_TRIANGLES, getIndexCount(), GL_UNSIGNED_INT, (void *)0);
    glBindVertexArray(0);
  }

  // Draw sphere with texture
  void drawSphereTexture(Shader &shader, glm::mat4 model,
                         unsigned int diffuseMap, unsigned int specularMap,
                         glm::vec3 diffuseColor = glm::vec3(1.0f)) const {
    shader.use();

    // texture slot bind করো
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    // texture1 uniform-এ slot 0 বলো
    shader.setInt("texture1", 0);

    // material.diffuse হিসেবে আসল রঙ পাঠাও (Mode 3-এর জন্য দরকার)
    shader.setVec3("material.diffuse", diffuseColor); // ✅
    shader.setFloat("material.shininess", 32.0f);

    glBindVertexArray(sphereVAO_WithTexture);
    shader.setMat4("model", model);
    glDrawElements(GL_TRIANGLES, getIndexCount(), GL_UNSIGNED_INT, (void *)0);
    glBindVertexArray(0);
  }

private:
  void buildCoordinatesAndIndices() {
    float x, y, z, xy;                           // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius; // normalized vertex normal
    float s, t;                                  // vertex texture coordinate

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i) {
      stackAngle = PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
      xy = radius * cosf(stackAngle);      // r * cos(u)
      z = radius * sinf(stackAngle);       // r * sin(u)

      for (int j = 0; j <= sectorCount; ++j) {
        sectorAngle = j * sectorStep; // starting from 0 to 2pi

        // vertex position (x, y, z)
        x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
        y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
        coordinates.push_back(x);
        coordinates.push_back(y);
        coordinates.push_back(z);

        // normalized vertex normal (nx, ny, nz)
        nx = x * lengthInv;
        ny = y * lengthInv;
        nz = z * lengthInv;
        normals.push_back(nx);
        normals.push_back(ny);
        normals.push_back(nz);

        // vertex texture coordinate (s, t)
        s = (float)j / sectorCount;
        t = (float)i / stackCount;
        texCoords.push_back(s);
        texCoords.push_back(t);
      }
    }

    // indices
    int k1, k2;
    for (int i = 0; i < stackCount; ++i) {
      k1 = i * (sectorCount + 1); // beginning of current stack
      k2 = k1 + sectorCount + 1;  // beginning of next stack

      for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
        // two triangles per sector
        indices.push_back(k1);
        indices.push_back(k2);
        indices.push_back(k1 + 1);

        indices.push_back(k1 + 1);
        indices.push_back(k2);
        indices.push_back(k2 + 1);
      }
    }
  }

  void buildVertices() {
    int i, j;
    int count = (int)coordinates.size();
    for (i = 0, j = 0; i < count; i += 3, j += 2) {
      // Position
      vertices.push_back(coordinates[i]);
      vertices.push_back(coordinates[i + 1]);
      vertices.push_back(coordinates[i + 2]);

      // Normal
      vertices.push_back(normals[i]);
      vertices.push_back(normals[i + 1]);
      vertices.push_back(normals[i + 2]);

      // Texture coordinates
      vertices.push_back(texCoords[j]);
      vertices.push_back(texCoords[j + 1]);

      // For vertices without texture coordinates (for non-textured rendering)
      verticesWithoutTexture.push_back(coordinates[i]);
      verticesWithoutTexture.push_back(coordinates[i + 1]);
      verticesWithoutTexture.push_back(coordinates[i + 2]);

      verticesWithoutTexture.push_back(normals[i]);
      verticesWithoutTexture.push_back(normals[i + 1]);
      verticesWithoutTexture.push_back(normals[i + 2]);
    }
  }

  unsigned int sphereVAO_NoTexture;
  unsigned int sphereVAO_WithTexture;
  float radius;
  int sectorCount;
  int stackCount;
  vector<float> vertices;
  vector<float> verticesWithoutTexture;
  vector<float> coordinates;
  vector<float> normals;
  vector<float> texCoords;
  vector<unsigned int> indices;
  const int verticesStride;
};

#endif // SPHERE_H
