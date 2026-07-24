#include "formcomponents.hpp"

#include "../files/esm/esmwriter.hpp"

namespace openck {

void FormComponents::saveAll(ESMWriter& esm) const
{
    for (const auto& c : m_components)
    {
        if (c) c->save(esm);
    }
}

} // namespace openck
