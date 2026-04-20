#ifndef CYLINDER_BASE_H
#define CYLINDER_BASE_H

#include "shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#define PI 3.1416

using namespace std;

// const int MIN_SECTOR_COUNT1 = 3;
// const int MIN_STACK_COUNT1 = 2;

class Cylinder_base {
public:
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  float shininess;

  // Constructor
  Cylinder_base(float radius = 1.0f, int sectorCount = 36, int stackCount = 18,
                glm::vec3 amb = glm::vec3(1.0f, 0.0f, 0.0f),
                glm::vec3 diff = glm::vec3(1.0f, 0.0f, 0.0f),
                glm::vec3 spec = glm::vec3(0.5f, 0.5f, 0.5f),
                float shiny = 32.0f)
      : verticesStride(32) {
    set(radius, sectorCount, stackCount, amb, diff, spec, shiny);
    buildCoordinatesAndIndices();
    buildVertices();

    // VAO for cylinder without texture coordinates
    glGenVertexArrays(1, &sphereVAO_NoTexture);
    glBindVertexArray(sphereVAO_NoTexture);

    unsigned int sphereVBO;
    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO); // for vertex data
    glBufferData(GL_ARRAY_BUFFER, this->getVertexSizeWithoutTexture(),
                 this->getVerticesWithoutTexture(), GL_STATIC_DRAW);

    unsigned int sphereEBO;
    glGenBuffers(1, &sphereEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO); // for index data
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->getIndexSize(),
                 this->getIndices(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // Position
    glEnableVertexAttribArray(1); // Normal

    int stride = this->getVerticesStride(); // should be 24 bytes now (24 bytes
                                            // for position + normal)
    glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, stride,
                          (void *)(sizeof(float) * 3));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // VAO for cylinder with texture coordinates
    glGenVertexArrays(1, &sphereVAO_WithTexture);
    glBindVertexArray(sphereVAO_WithTexture);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, this->getVertexSize(), this->getVertices(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->getIndexSize(),
                 this->getIndices(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // Position
    glEnableVertexAttribArray(1); // Normal
    glEnableVertexAttribArray(2); // Texture coordinates

    glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, stride,
                          (void *)(sizeof(float) * 3));
    glVertexAttribPointer(2, 2, GL_FLOAT, false, stride,
                          (void *)(sizeof(float) * 6));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  ~Cylinder_base() {}

  // Setters
  void set(float radius, int sectors, int stacks, glm::vec3 amb, glm::vec3 diff,
           glm::vec3 spec, float shiny) {
    if (radius > 0)
      this->radius = radius;
    this->sectorCount = sectors;
    if (sectors < MIN_SECTOR_COUNT1)
      this->sectorCount = MIN_SECTOR_COUNT1;
    this->stackCount = stacks;
    if (stacks < MIN_STACK_COUNT1)
      this->stackCount = MIN_STACK_COUNT1;
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

  // Drawing with lighting and texture
  void drawSphere(Shader &lightingShader, glm::mat4 model) const {
    lightingShader.use();
    lightingShader.setVec3("material.ambient", this->ambient);
    lightingShader.setVec3("material.diffuse", this->diffuse);
    lightingShader.setVec3("material.specular", this->specular);
    lightingShader.setFloat("material.shininess", this->shininess);
    lightingShader.setMat4("model", model);

    glBindVertexArray(sphereVAO_NoTexture);
    glDrawElements(GL_TRIANGLES, this->getIndexCount(), GL_UNSIGNED_INT,
                   (void *)0);
    glBindVertexArray(0);
  }

  // Drawing with texture
  void drawSphereTexture(Shader &lightingShaderWithTexture, glm::mat4 model,
                         unsigned int diffuseMap,
                         unsigned int specularMap) const {
    lightingShaderWithTexture.use();
    lightingShaderWithTexture.setInt("material.diffuse", 0);
    lightingShaderWithTexture.setInt("material.specular", 1);
    lightingShaderWithTexture.setFloat("material.shininess", 32.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    glBindVertexArray(sphereVAO_WithTexture);
    lightingShaderWithTexture.setMat4("model", model);
    glDrawElements(GL_TRIANGLES, this->getIndexCount(), GL_UNSIGNED_INT,
                   (void *)0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
  }

private:
  void buildCoordinatesAndIndices() {
    float x, y, z, nx, ny, nz;
    float sectorStep = 2 * PI / sectorCount;
    float sectorAngle;

    // Build vertices for stacks
    for (int i = 0; i <= stackCount; ++i) {
      y = -0.5f + (float)i / stackCount; // Stack height from -0.5 to 0.5

      for (int j = 0; j <= sectorCount; ++j) {
        sectorAngle = j * sectorStep;
        x = radius * cos(sectorAngle);
        z = radius * sin(sectorAngle);

        // Vertex position
        coordinates.push_back(x);
        coordinates.push_back(y);
        coordinates.push_back(z);

        // Normal
        nx = cos(sectorAngle);
        ny = 0;
        nz = sin(sectorAngle);
        normals.push_back(nx);
        normals.push_back(ny);
        normals.push_back(nz);

        // Texture coordinates
        texCoords.push_back((float)j / sectorCount);
        texCoords.push_back((float)i / stackCount);
      }
    }

    // Indices for the sides
    for (int i = 0; i < stackCount; ++i) {
      int k1 = i * (sectorCount + 1); // Beginning of current stack
      int k2 = k1 + sectorCount + 1;  // Beginning of next stack

      for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
        // Two triangles for each sector
        indices.push_back(k1);
        indices.push_back(k2);
        indices.push_back(k1 + 1);

        indices.push_back(k1 + 1);
        indices.push_back(k2);
        indices.push_back(k2 + 1);
      }
    }

    // Indices for the bottom circle
    int centerIndexBottom =
        coordinates.size() / 3; // Center point of the bottom circle
    coordinates.push_back(0);
    coordinates.push_back(-0.5f);
    coordinates.push_back(0);
    normals.push_back(0);
    normals.push_back(-1);
    normals.push_back(0);
    texCoords.push_back(0.5f);
    texCoords.push_back(0.5f);

    for (int i = 0; i < sectorCount; ++i) {
      indices.push_back(centerIndexBottom);
      indices.push_back(i);
      indices.push_back((i + 1) % sectorCount);
    }

    // Indices for the top circle
    int centerIndexTop =
        coordinates.size() / 3; // Center point of the top circle
    coordinates.push_back(0);
    coordinates.push_back(0.5f);
    coordinates.push_back(0);
    normals.push_back(0);
    normals.push_back(1);
    normals.push_back(0);
    texCoords.push_back(0.5f);
    texCoords.push_back(0.5f);

    int baseIndex = (stackCount) * (sectorCount + 1);
    for (int i = 0; i < sectorCount; ++i) {
      indices.push_back(centerIndexTop);
      indices.push_back(baseIndex + (i + 1) % sectorCount);
      indices.push_back(baseIndex + i);
    }
  }

  void buildVertices() {
    size_t i, j;
    size_t count = coordinates.size();
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

      // Vertices without texture
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

#endif // CYLINDER_H
