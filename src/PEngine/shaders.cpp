//#include <iostream>
#include <fstream>
#include <sstream>

#include "shaders.h"

ShaderProgram::ShaderProgram(const std::string& vsh, const std::string& fsh)
{
    shader_id = createShaderProgram(vsh, fsh);
}

ShaderProgram::~ShaderProgram()
{
    unuse();

    if (shader_id != 0)
        glDeleteProgram(shader_id);
}

void ShaderProgram::use()
{
    glUseProgram(shader_id);
}

void ShaderProgram::uniform(const std::string& name, GLint i)
{
    GLint u_id = getUniformLocation(name);
    glUniform1i(u_id, i);
}

void ShaderProgram::uniform(const std::string& name, GLfloat f)
{
    GLint u_id = getUniformLocation(name);
    glUniform1f(u_id, f);
}

void ShaderProgram::uniform(const std::string& name, const glm::mat4& m)
{
    GLint u_id = getUniformLocation(name);
    glUniformMatrix4fv(u_id, 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderProgram::uniform(const std::string& name, const glm::mat3& m)
{
    GLint u_id = getUniformLocation(name);
    glUniformMatrix3fv(u_id, 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderProgram::uniform(const std::string& name, const glm::vec3& v)
{
    GLint u_id = getUniformLocation(name);
    glUniform3fv(u_id, 1, glm::value_ptr(v));
}

void ShaderProgram::uniform(const std::string& name, const glm::vec4& v)
{
    GLint u_id = getUniformLocation(name);
    glUniform4fv(u_id, 1, glm::value_ptr(v));
}

void ShaderProgram::attrib(const std::string& name, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    GLint a_id = getAttribLocation(name);
    enabled_attributes.push_back(a_id);
    glEnableVertexAttribArray(a_id);
    glVertexAttribPointer(a_id, size, type, normalized, stride, pointer);
}

void ShaderProgram::unuse() {
    for (GLint a_id: enabled_attributes)
        glDisableVertexAttribArray(a_id);

    glUseProgram(0);
}

/*
 * Private interface
 */

GLint ShaderProgram::getUniformLocation(const std::string& name) {
    return glGetUniformLocation(shader_id, name.c_str());
}

GLint ShaderProgram::getAttribLocation(const std::string& name) {
    return glGetAttribLocation(shader_id, name.c_str());
}

GLuint ShaderProgram::createShader(const std::string& code, GLenum type)
{
    GLuint result = glCreateShader(type);

    const char* code_str = code.c_str();

    glShaderSource(result, 1, &code_str, NULL);
    glCompileShader(result);

    GLint compiled;
    glGetShaderiv(result, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0)
        {
            char* infoLog = (char*)alloca(infoLen);
            glGetShaderInfoLog(result, infoLen, NULL, infoLog);
            puts("Shader compilation error");
            puts(infoLog);
        }
        glDeleteShader(result);
        return 0;
    }

    return result;
}

GLuint ShaderProgram::createProgram(GLuint vsh, GLuint fsh)
{
    GLuint result = glCreateProgram();

    glAttachShader(result, vsh);
    glAttachShader(result, fsh);

    glLinkProgram(result);

    GLint linked;
    glGetProgramiv(result, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0)
        {
            char* infoLog = (char*)alloca(infoLen);
            glGetProgramInfoLog(result, infoLen, NULL, infoLog);
            puts("Shader program linking error");
            puts(infoLog);
        }
        glDeleteProgram(result);
        return 0;
    }

    return result;
}

GLuint ShaderProgram::createShaderProgram(const std::string& vsh_path, const std::string& fsh_path)
{
    GLuint shaderProgram = 0;

    std::string vsh = readShader(vsh_path);
    std::string fsh = readShader(fsh_path);

    GLuint vertexShader, fragmentShader;

    vertexShader = createShader(vsh, GL_VERTEX_SHADER);
    fragmentShader = createShader(fsh, GL_FRAGMENT_SHADER);

    shaderProgram = createProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

std::string ShaderProgram::readShader(const std::string& path)
{
    std::ifstream t("../data/shaders/" + path);
    std::stringstream buffer;
    buffer << t.rdbuf();
    //std::cout << buffer.str() << std::endl;
    return buffer.str();
}

VAO::VAO(const float* vbo, size_t vbo_size, const unsigned short* ibo, size_t ibo_size)
{
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &indexBuffer);

    bind();

    glBufferData(GL_ARRAY_BUFFER, vbo_size, vbo, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibo_size, ibo, GL_STATIC_DRAW);

    unbind();
}

VAO::~VAO()
{
    unbind();
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
}

void VAO::bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
}

void VAO::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
