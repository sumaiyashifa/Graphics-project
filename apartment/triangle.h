//
//  triangle.h
//  test
//
//  Created by Nazirul Hasan on 4/10/23.
//

#ifndef triangle_h
#define triangle_h

#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"

using namespace std;

class Triangle {
public:
    // Material properties
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    // Texture properties
    float TXmin = 0.0f;
    float TXmax = 1.0f;
    float TYmin = 0.0f;
    float TYmax = 1.0f;
    unsigned int diffuseMap;
    unsigned int specularMap;

    // Common property
    float shininess;

    // Constructors
    Triangle() {
        setUpTriangleVertexDataAndConfigureVertexAttribute();
    }

    Triangle(glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, float shiny)
        : ambient(amb), diffuse(diff), specular(spec), shininess(shiny) {
        setUpTriangleVertexDataAndConfigureVertexAttribute();
    }

    Triangle(unsigned int dMap, unsigned int sMap, float shiny, float textureXmin, float textureYmin, float textureXmax, float textureYmax)
        : diffuseMap(dMap), specularMap(sMap), shininess(shiny),
        TXmin(textureXmin), TYmin(textureYmin), TXmax(textureXmax), TYmax(textureYmax) {
        setUpTriangleVertexDataAndConfigureVertexAttribute();
    }

    // Destructor
    ~Triangle() {
        glDeleteVertexArrays(1, &triangleVAO);
        glDeleteVertexArrays(1, &lightTriangleVAO);
        glDeleteVertexArrays(1, &lightTexTriangleVAO);
        glDeleteBuffers(1, &triangleVBO);
        glDeleteBuffers(1, &triangleEBO);
    }

    // Functions to draw with texture or material properties
    void drawTriangleWithTexture(Shader& lightingShaderWithTexture, glm::mat4 model = glm::mat4(1.0f)) {
        lightingShaderWithTexture.use();

        lightingShaderWithTexture.setInt("material.diffuse", 0);
        lightingShaderWithTexture.setInt("material.specular", 1);
        lightingShaderWithTexture.setFloat("material.shininess", this->shininess);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->specularMap);

        lightingShaderWithTexture.setMat4("model", model);

        glBindVertexArray(lightTexTriangleVAO);
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0); // Updated for 5 sides
    }

    void drawTriangleWithMaterialisticProperty(Shader& lightingShader, glm::mat4 model = glm::mat4(1.0f)) {
        lightingShader.use();

        lightingShader.setVec3("material.ambient", this->ambient);
        lightingShader.setVec3("material.diffuse", this->diffuse);
        lightingShader.setVec3("material.specular", this->specular);
        lightingShader.setFloat("material.shininess", this->shininess);

        lightingShader.setMat4("model", model);

        glBindVertexArray(lightTriangleVAO);
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0); // Updated for 5 sides
    }

    void drawTriangle(Shader& shader, glm::mat4 model = glm::mat4(1.0f), float r = 1.0f, float g = 1.0f, float b = 1.0f) {
        shader.use();

        shader.setVec3("color", glm::vec3(r, g, b));
        shader.setMat4("model", model);

        glBindVertexArray(triangleVAO);
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0); // Updated for 5 sides
    }

    // Setters for material and texture properties
    void setMaterialisticProperty(glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, float shiny) {
        this->ambient = amb;
        this->diffuse = diff;
        this->specular = spec;
        this->shininess = shiny;
    }

    void setTextureProperty(unsigned int dMap, unsigned int sMap, float shiny) {
        this->diffuseMap = dMap;
        this->specularMap = sMap;
        this->shininess = shiny;
    }

private:
    unsigned int triangleVAO;
    unsigned int lightTriangleVAO;
    unsigned int lightTexTriangleVAO;
    unsigned int triangleVBO;
    unsigned int triangleEBO;

    void setUpTriangleVertexDataAndConfigureVertexAttribute() {
        // Vertex data for the triangle prism
        float triangle_vertices[] = {
            // positions        // normals         // texture coordinates
            // Front triangle
            0.0f, 0.0f, 0.0f,   0.0f, 0.0f, -1.0f,  0.0f, 0.0f, // Vertex 0
            1.0f, 0.0f, 0.0f,   0.0f, 0.0f, -1.0f,  1.0f, 0.0f, // Vertex 1
            0.0f, 1.0f, 0.0f,   0.0f, 0.0f, -1.0f,  0.0f, 1.0f, // Vertex 2

            // Back triangle
            0.0f, 0.0f, 1.0f,   0.0f, 0.0f,  1.0f,  0.0f, 0.0f, // Vertex 3
            1.0f, 0.0f, 1.0f,   0.0f, 0.0f,  1.0f,  1.0f, 0.0f, // Vertex 4
            0.0f, 1.0f, 1.0f,   0.0f, 0.0f,  1.0f,  0.0f, 1.0f, // Vertex 5

            // Bottom rectangle (base of prism)
            0.0f, 0.0f, 0.0f,   0.0f, -1.0f, 0.0f,  0.0f, 0.0f, // Vertex 6
            1.0f, 0.0f, 0.0f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f, // Vertex 7
            1.0f, 0.0f, 1.0f,   0.0f, -1.0f, 0.0f,  1.0f, 1.0f, // Vertex 8
            0.0f, 0.0f, 1.0f,   0.0f, -1.0f, 0.0f,  0.0f, 1.0f, // Vertex 9

            // Side rectangle 1
            1.0f, 0.0f, 0.0f,   1.0f, 1.0f,  0.0f,  0.0f, 0.0f, // Vertex 10
            1.0f, 0.0f, 1.0f,   1.0f, 1.0f,  0.0f,  1.0f, 0.0f, // Vertex 11
            0.0f, 1.0f, 1.0f,   1.0f, 1.0f,  0.0f,  1.0f, 1.0f, // Vertex 12
            0.0f, 1.0f, 0.0f,   1.0f, 1.0f,  0.0f,  0.0f, 1.0f, // Vertex 13

            // Side rectangle 2
            0.0f, 0.0f, 0.0f,  -1.0f, 0.0f,  0.0f,  0.0f, 0.0f, // Vertex 14
            0.0f, 0.0f, 1.0f,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, // Vertex 15
            0.0f, 1.0f, 1.0f,  -1.0f, 0.0f,  0.0f,  1.0f, 1.0f, // Vertex 16
            0.0f, 1.0f, 0.0f,  -1.0f, 0.0f,  0.0f,  0.0f, 1.0f, // Vertex 17
        };

        // Indices for drawing the triangle prism
        unsigned int triangle_indices[] = {
            // Front triangle
            0, 1, 2,
            // Back triangle
            3, 4, 5,
            // Bottom rectangle
            6, 7, 8,
            8, 9, 6,
            // Side rectangle 1
            10, 11, 12,
            12, 13, 10,
            // Side rectangle 2
            14, 15, 16,
            16, 17, 14
        };

        glGenVertexArrays(1, &triangleVAO);
        glGenVertexArrays(1, &lightTriangleVAO);
        glGenVertexArrays(1, &lightTexTriangleVAO);
        glGenBuffers(1, &triangleVBO);
        glGenBuffers(1, &triangleEBO);


        glBindVertexArray(lightTexTriangleVAO);

        glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle_indices), triangle_indices, GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // vertex normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)12);
        glEnableVertexAttribArray(1);

        // texture coordinate attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)24);
        glEnableVertexAttribArray(2);


        glBindVertexArray(lightTriangleVAO);

        glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleEBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)12);
        glEnableVertexAttribArray(1);


        glBindVertexArray(triangleVAO);

        glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleEBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
};

#endif /* triangle_h */
