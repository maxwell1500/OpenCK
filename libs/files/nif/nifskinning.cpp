#include "nifskinning.hpp"

namespace Nif {

void blendSkinnedLocal(const float* restPos, const float* restNrm, int vertexCount,
                       const float (*palettes)[16], const float (*normalPalettes)[9],
                       int paletteCount,
                       const SkinVertexWeight* weights, int weightCount,
                       float* outPos, float* outNrm)
{
    if (!restPos || !restNrm || vertexCount <= 0 || !outPos || !outNrm)
        return;

    // Accumulate weighted palette transforms per vertex.
    // VLA-free: fixed-size spill via heap buffers sized by vertexCount.
    float* accPos = new float[static_cast<size_t>(vertexCount) * 3]();
    float* accNrm = new float[static_cast<size_t>(vertexCount) * 3]();
    float* accW = new float[static_cast<size_t>(vertexCount)]();

    if (palettes && normalPalettes && paletteCount > 0 && weights && weightCount > 0)
    {
        for (int i = 0; i < weightCount; ++i)
        {
            const SkinVertexWeight& w = weights[i];
            if (w.weight <= 0.0f)
                continue;
            const int v = static_cast<int>(w.vertex);
            const int b = static_cast<int>(w.bone);
            if (v < 0 || v >= vertexCount || b < 0 || b >= paletteCount)
                continue;

            float p[3];
            transformPointRowMajor(palettes[b], restPos + v * 3, p);
            accPos[v * 3 + 0] += p[0] * w.weight;
            accPos[v * 3 + 1] += p[1] * w.weight;
            accPos[v * 3 + 2] += p[2] * w.weight;

            float n[3];
            transformDirectionRowMajor(normalPalettes[b], restNrm + v * 3, n);
            accNrm[v * 3 + 0] += n[0] * w.weight;
            accNrm[v * 3 + 1] += n[1] * w.weight;
            accNrm[v * 3 + 2] += n[2] * w.weight;

            accW[v] += w.weight;
        }
    }

    for (int v = 0; v < vertexCount; ++v)
    {
        if (accW[v] > 0.0f)
        {
            outPos[v * 3 + 0] = accPos[v * 3 + 0] / accW[v];
            outPos[v * 3 + 1] = accPos[v * 3 + 1] / accW[v];
            outPos[v * 3 + 2] = accPos[v * 3 + 2] / accW[v];
            outNrm[v * 3 + 0] = accNrm[v * 3 + 0] / accW[v];
            outNrm[v * 3 + 1] = accNrm[v * 3 + 1] / accW[v];
            outNrm[v * 3 + 2] = accNrm[v * 3 + 2] / accW[v];
        }
        else
        {
            outPos[v * 3 + 0] = restPos[v * 3 + 0];
            outPos[v * 3 + 1] = restPos[v * 3 + 1];
            outPos[v * 3 + 2] = restPos[v * 3 + 2];
            outNrm[v * 3 + 0] = restNrm[v * 3 + 0];
            outNrm[v * 3 + 1] = restNrm[v * 3 + 1];
            outNrm[v * 3 + 2] = restNrm[v * 3 + 2];
        }
    }

    delete[] accPos;
    delete[] accNrm;
    delete[] accW;
}

void packGpuSkinInfluences(int vertexCount, const SkinVertexWeight* weights,
                           int weightCount, int boneCount,
                           QVector<GpuSkinInfluence>& out)
{
    out.clear();
    if (vertexCount <= 0)
        return;
    out.resize(vertexCount);

    if (!weights || weightCount <= 0 || boneCount <= 0)
        return;

    for (int i = 0; i < weightCount; ++i) {
        const SkinVertexWeight& w = weights[i];
        if (w.weight <= 0.0f) continue;
        if (w.vertex >= static_cast<quint32>(vertexCount)) continue;
        if (w.bone >= static_cast<quint32>(boneCount)) continue;
        GpuSkinInfluence& v = out[static_cast<int>(w.vertex)];
        for (int k = 0; k < 8; ++k) {
            if (v.weights[k] == 0.0f) {
                v.indices[k] = static_cast<float>(w.bone);
                v.weights[k] = w.weight;
                break;
            }
        }
    }

    for (GpuSkinInfluence& v : out) {
        float total = 0.0f;
        for (int k = 0; k < 8; ++k) total += v.weights[k];
        if (total <= 0.0f) continue;
        for (int k = 0; k < 8; ++k) v.weights[k] /= total;
    }
}

} // namespace Nif
