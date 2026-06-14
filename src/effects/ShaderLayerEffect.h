#pragma once

#include "effects/Effect.h"
#include "renderer/Shader.h"
#include <glad/glad.h>
#include <memory>

namespace NoiseArt {

class ShaderLayerEffect : public Effect {
public:
    ShaderLayerEffect();
    ~ShaderLayerEffect() override;

    std::string getName() const override { return "Live Shader"; }
    std::string getCategory() const override { return "Backgrounds"; }

    void apply(const Image& input, Image& output) override;

    bool isShader() const override { return true; }
    void renderShader(int width, int height, float time) override;

    bool renderUI() override;

    std::unique_ptr<Effect> clone() const override;

    enum class ShaderType {
        Plasma,
        Voronoi,
        Perlin
    };

private:
    void initGL();
    void compileCurrentShader();
    const char* getFragmentSource() const;

    ShaderType m_type = ShaderType::Plasma;
    std::unique_ptr<Shader> m_shader;
    
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    
    // Shader params
    float m_scale = 5.0f;
    float m_speed = 1.0f;
    float m_color[3] = {1.0f, 0.5f, 0.2f};
};

} // namespace NoiseArt
