#ifndef FORM_COMPONENTS_HPP
#define FORM_COMPONENTS_HPP

// FormComponents — the simple container that record structs use to
// own their list of components. Each record struct that participates
// in the component architecture embeds a FormComponents member:
//
//   struct StatRecord {
//       openck::FormComponents components;
//       QString editorId;
//       ...
//       void load(ESMReader& esm, bool base);
//   };
//
// The FormComponents helper does four things:
//   * add<T>(...) appends a fresh component of the given type
//   * findByName("TESModel") returns the first component whose
//     className() matches the given string, or nullptr
//   * iterate() yields const access to all components for save/clone
//   * the container is copy-constructible and assignable, so cloning
//     a record is one line
//
// We don't expose a typed find<T>() helper because every concrete
// Component has a className() method that already returns the
// matching string. The cost of writing `findByName(T::className())`
// instead of `find<T>()` is one string literal; the benefit is that
// the typed version doesn't need every component to declare a
// `classNameStatic()` companion.

#include "component.hpp"

#include <QString>

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

class ESMReader;
class ESMWriter;

namespace openck {

class FormComponents
{
public:
    FormComponents() = default;
    FormComponents(const FormComponents& other)
    {
        for (const auto& c : other.m_components)
        {
            m_components.push_back(c->clone());
        }
    }
    FormComponents& operator=(const FormComponents& other)
    {
        if (this != &other)
        {
            FormComponents copy(other);
            m_components.swap(copy.m_components);
        }
        return *this;
    }
    FormComponents(FormComponents&&) noexcept = default;
    FormComponents& operator=(FormComponents&&) noexcept = default;

    // Append a fresh component of type T, forwarding args to T's
    // constructor. The component is owned by this container.
    template <typename T, typename... Args>
    T* add(Args&&... args)
    {
        static_assert(std::is_base_of_v<Component, T>,
            "FormComponents::add<T> requires T to derive from Component");
        auto c = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = c.get();
        m_components.push_back(std::move(c));
        return raw;
    }

    // Returns the first component whose className() matches the
    // given class, or nullptr. Searches linearly because record
    // types typically have <20 components — keeping it simple.
    Component* findByName(const QString& className)
    {
        auto it = std::find_if(m_components.begin(), m_components.end(),
            [&](const std::unique_ptr<Component>& c) {
                return c->className() == className;
            });
        return it != m_components.end() ? it->get() : nullptr;
    }
    const Component* findByName(const QString& className) const
    {
        return const_cast<FormComponents*>(this)->findByName(className);
    }

    const std::vector<std::unique_ptr<Component>>& all() const { return m_components; }
    std::vector<std::unique_ptr<Component>>& all() { return m_components; }

    void clear() { m_components.clear(); }

    bool empty() const { return m_components.empty(); }
    std::size_t size() const { return m_components.size(); }

    inline bool operator==(const FormComponents& other) const
    {
        if (m_components.size() != other.m_components.size()) return false;
        for (size_t i = 0; i < m_components.size(); ++i)
        {
            if (!m_components[i]->isEqualTo(other.m_components[i].get())) return false;
        }
        return true;
    }
    inline bool operator!=(const FormComponents& other) const { return !(*this == other); }

    // Save every component's subrecords in order. Used by record
    // save() implementations that prefer to delegate to the
    // component layer.
    inline void saveAll(ESMWriter& esm) const
    {
        for (const auto& c : m_components)
        {
            if (c) c->save(esm);
        }
    }

private:
    std::vector<std::unique_ptr<Component>> m_components;
};

} // namespace openck

#endif // FORM_COMPONENTS_HPP
