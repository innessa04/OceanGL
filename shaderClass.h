#pragma once
#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

std::string get_file_contents(const char* filename);

class Shader
{
public:
    GLuint ID;
    Shader(const char* vertexFile, const char* fragmentFile);

    void Activate();
    void Delete();
    void use();


    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;


private:
    void compileErrors(unsigned int shader, const char* type);
};


#endif