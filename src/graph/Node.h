// ============================================================================
// NoiseArt — Node (абстрактна нода графа ефектів/шейдерів)
// ============================================================================
// Кожна нода видає одну GL-текстуру у evaluate(). Має власні GPU-ресурси
// (ліниві — реально задіюються у Phase 2, коли з'явиться топологічний евал).
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstddef>
#include <cstdint>
#include "graph/Socket.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"

namespace NoiseArt {

class QuadRenderer;  // спільний фулскрін-квад (Phase 2)

// Контекст одного проходу обчислення графа.
struct NodeEvalContext {
    int           width            = 512;     // роздільність рендеру (RES)
    int           height           = 512;
    float         layerW           = 400.0f;  // розмір шару на полотні (для u_resolution шейдера)
    float         layerH           = 300.0f;
    float         time             = 0.0f;
    uint64_t      editGen          = 0;
    bool          interacting      = false;
    bool          lowResInteract   = false;
    bool          cache            = true;
    bool          throttle         = false;
    float         throttleInterval = 1.0f / 30.0f;
    unsigned int  sourceTex        = 0;        // вхідна текстура шару (для InputNode)
    QuadRenderer* quad             = nullptr;  // спільний фулскрін-квад
};

class Node {
public:
    explicit Node(int id) : m_id(id) {}
    virtual ~Node() = default;

    int id() const { return m_id; }

    // --- Метадані ---
    virtual std::string getTypeName() const = 0;
    virtual std::string getCategory() const { return "Node"; }

    // --- Сокети (структуру задає конструктор нащадка; id призначає NodeGraph) ---
    std::vector<Socket>&       inputs()        { return m_inputs; }
    std::vector<Socket>&       outputs()       { return m_outputs; }
    const std::vector<Socket>& inputs()  const { return m_inputs; }
    const std::vector<Socket>& outputs() const { return m_outputs; }

    // --- UI параметрів у тілі ноди (задіє Phase 3) ---
    virtual bool renderParamsUI() { return false; }

    // --- Обчислення → GL-текстура результату. inputTex відповідає inputs() за
    //     порядком (0, якщо вхід не під'єднано). ---
    virtual unsigned int evaluate(NodeEvalContext& ctx, const std::vector<unsigned int>& inputTex) = 0;

    // --- Кеш / ідентичність ---
    virtual std::size_t paramHash() const { return 0; }

    // --- Чи залежить нода від часу? Графи з анімацією рендеряться щокадру (з тротлінгом). ---
    virtual bool isAnimated() const { return false; }

    // --- Чи paramHash() ПОВНІСТЮ описує стан ноди? Якщо ні (напр. CpuEffectNode, чиї
    //     параметри живуть у Effect без хешу), кеш додатково інвалідовується editGen.
    //     Безпечний типовий варіант — false (перераховувати на будь-яку правку). ---
    virtual bool cacheKeyComplete() const { return false; }

    // --- Глибока копія (зберігає id ноди та id її сокетів) ---
    virtual std::unique_ptr<Node> clone() const = 0;

    // Позиція у редакторі (imgui-node-editor / серіалізація)
    float m_editorX = 0.0f;
    float m_editorY = 0.0f;

    // Кеш рендеру (задіє Phase 2)
    std::size_t  m_cacheKey   = 0;
    bool         m_cacheValid = false;
    unsigned int m_cacheTex   = 0;

protected:
    // Хелпери для конструкторів нащадків
    void addInput (const std::string& name, SocketType t) { m_inputs.push_back ({ -1, name, t, true,  m_id }); }
    void addOutput(const std::string& name, SocketType t) { m_outputs.push_back({ -1, name, t, false, m_id }); }

    // Копіює базовий стан (сокети з їх id + позицію) — для clone() нащадків.
    // GPU-ресурси й кеш НЕ копіюються (свіжі).
    void copyBaseFrom(const Node& o) {
        m_inputs  = o.m_inputs;
        m_outputs = o.m_outputs;
        m_editorX = o.m_editorX;
        m_editorY = o.m_editorY;
    }

    int                 m_id = -1;
    std::vector<Socket> m_inputs;
    std::vector<Socket> m_outputs;
    Framebuffer         m_fbo;     // власний FBO ноди (Phase 2)
    Texture             m_outTex;  // вихідна текстура ноди (Phase 2)
};

} // namespace NoiseArt
