#pragma once
#include "disable_all_warnings.h"
#include <vector>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
DISABLE_WARNINGS_POP()
#include <filesystem>

class Screen {
public:
    Screen(const glm::ivec2& resolution);

    void clear(const glm::vec3& color);
    void setPixel(int x, int y, const glm::vec3& color);

    void writeBitmapToFile(const std::filesystem::path& filePath);
    void draw();

private:
    glm::ivec2 m_resolution;
    std::vector<glm::vec3> m_textureData;

    uint32_t m_texture;
};
