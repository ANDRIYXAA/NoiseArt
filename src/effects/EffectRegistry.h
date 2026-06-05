// ============================================================================
// NoiseArt — Реєстр ефектів (Заголовок)
// ============================================================================
// EffectRegistry — центральне місце, де зареєстровані ВСІ ефекти.
// Завдяки цьому:
// - UI автоматично показує всі доступні ефекти
// - Додати новий ефект = 1 рядок коду (registry.registerEffect<MyEffect>())
// - Не потрібно змінювати код UI кожного разу
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <functional>  // std::function — обгортка для будь-якої функції
#include <memory>      // std::unique_ptr

#include "Effect.h"

namespace NoiseArt {

// ============================================================================
// EffectInfo — інформація про один зареєстрований ефект
// ============================================================================
// Ця структура зберігає ВСЕ, що потрібно знати про ефект,
// щоб показати його в меню та створити новий екземпляр.
struct EffectInfo {
    std::string name;       // "Noise", "Bloom", "Blur" тощо
    std::string category;   // "Generate", "Color", "Blur" — для групування в меню

    // Фабрична функція — створює НОВИЙ екземпляр ефекту.
    // std::function — це обгортка, яка може зберігати будь-яку функцію
    // (лямбду, звичайну функцію, метод класу).
    // Тут вона зберігає лямбду: []() { return std::make_unique<NoiseEffect>(); }
    std::function<std::unique_ptr<Effect>()> create;
};

// ============================================================================
// EffectRegistry — реєстр (каталог) всіх ефектів
// ============================================================================
class EffectRegistry {
public:
    // ===== Реєстрація =====

    // registerEffect<T>() — шаблонна функція (template).
    // T — це тип ефекту (NoiseEffect, BlurEffect тощо).
    // Компілятор автоматично створить окрему версію функції для кожного T.
    //
    // Як це працює:
    // 1. Створюємо тимчасовий об'єкт T, щоб дізнатись його ім'я та категорію
    // 2. Зберігаємо лямбду, яка вміє створювати нові T
    // 3. Додаємо інформацію в список
    template<typename T>
    void registerEffect() {
        EffectInfo info;
        T temp;  // Тимчасовий об'єкт для отримання метаданих
        info.name = temp.getName();
        info.category = temp.getCategory();
        // Лямбда — анонімна функція, яка "захоплює" тип T
        // і створює новий об'єкт кожного разу, коли її викликають
        info.create = []() -> std::unique_ptr<Effect> {
            return std::make_unique<T>();
        };
        m_effects.push_back(std::move(info));
    }

    // ===== Отримання інформації =====

    /// Повертає список ВСІХ зареєстрованих ефектів
    const std::vector<EffectInfo>& getAll() const { return m_effects; }

    /// Повертає список категорій (без дублікатів)
    std::vector<std::string> getCategories() const;

    /// Повертає ефекти певної категорії
    std::vector<const EffectInfo*> getByCategory(const std::string& category) const;

    /// Створює новий екземпляр ефекту за його назвою
    std::unique_ptr<Effect> createEffect(const std::string& name) const;

private:
    std::vector<EffectInfo> m_effects;
};

} // namespace NoiseArt
