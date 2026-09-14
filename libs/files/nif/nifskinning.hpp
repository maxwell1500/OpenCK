#ifndef NIFSKINNING_HPP
#define NIFSKINNING_HPP

#include "nifparser.hpp"

namespace Nif {

// CPU per-vertex skinning math (REMAINING.md §8.1). GUI-free on purpose: the
// viewport and the headless tests share this exact code path, so the numbers
// the tests pin are the numbers the renderer blends.
//
// Conventions:
// - Positions/normals are flat float triples, vertices numbered 0..N-1.
// - Palettes are row-major 4x4 (16 floats) and row-major 3x3 (9 floats).
//   Callers holding column-major matrices (e.g. QMatrix4x4::constData())
//   must transpose into these buffers first.
// - Weights reference shape-local vertices and palette indices; out-of-range
//   vertices/bones and non-positive weights are ignored.
// - Each vertex normalizes by its own total weight, so sparse or
//   non-normalized weight lists still behave. Vertices with no usable weight
//   keep their rest pose (the caller applies the owner transform on top,
//   matching the rigid path).

/// Blends rest-pose positions/normals through bone palettes.
/// - restPos/restNrm: 3*vertexCount input floats.
/// - palettes/normalPalettes: paletteCount row-major matrices.
/// - weights/weightCount: sparse influences.
/// - outPos/outNrm: 3*vertexCount outputs (may alias rest buffers).
void blendSkinnedLocal(const float* restPos, const float* restNrm, int vertexCount,
                       const float (*palettes)[16], const float (*normalPalettes)[9],
                       int paletteCount,
                       const SkinVertexWeight* weights, int weightCount,
                       float* outPos, float* outNrm);

/// Transforms a point by a row-major 4x4 matrix.
inline void transformPointRowMajor(const float m[16], const float in[3], float out[3])
{
    out[0] = m[0] * in[0] + m[1] * in[1] + m[2] * in[2] + m[3];
    out[1] = m[4] * in[0] + m[5] * in[1] + m[6] * in[2] + m[7];
    out[2] = m[8] * in[0] + m[9] * in[1] + m[10] * in[2] + m[11];
}

/// Transforms a direction by a row-major 3x3 matrix.
inline void transformDirectionRowMajor(const float m[9], const float in[3], float out[3])
{
    out[0] = m[0] * in[0] + m[1] * in[1] + m[2] * in[2];
    out[1] = m[3] * in[0] + m[4] * in[1] + m[5] * in[2];
    out[2] = m[6] * in[0] + m[7] * in[1] + m[8] * in[2];
}

} // namespace Nif

#endif // NIFSKINNING_HPP
