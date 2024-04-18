#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <GL/gl.h>

#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <glm/mat4x4.hpp> // For glm::mat4
#include <glm/mat3x3.hpp> // For glm::mat4

class ShaderProgram {
    public:
        ShaderProgram(const std::string& vsh, const std::string& fsh);
        ~ShaderProgram();
        void use();
        void unuse();
        void uniform(const std::string& name, GLint i);
        void uniform(const std::string& name, glm::mat4& m);
        void uniform(const std::string& name, glm::mat3& m);
        void attrib(const std::string& name, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);

    private:
        GLuint createShader(const std::string& code, GLenum type);
        GLuint createProgram(GLuint vsh, GLuint fsh);
        GLuint createShaderProgram(const std::string& vsh_path, const std::string& fsh_path);
        std::string readShader(const std::string& path);
        GLint getUniformLocation(const std::string& name);
        GLint getAttribLocation(const std::string& name);

        GLuint shader_id = 0;
        std::vector<GLint> enabled_attributes;
};

class VAO {
    public:
        VAO(const float* vbo, size_t vbo_size, const unsigned short* ibo, size_t ibo_size);
        ~VAO();
        void bind();
        void unbind();

    private:
        GLuint vertexBuffer = 0;
        GLuint indexBuffer = 0;
};
