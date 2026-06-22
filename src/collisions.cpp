// Implementação dos testes de colisão / interseção do Duck Hunt 3D.
// (Arquivo separado conforme exigido pelo enunciado do trabalho.)

#include "collisions.h"
#include <cmath>

// Interseção raio-esfera. Com a direção D normalizada, a interseção do raio
// P(t) = O + t*D com a esfera |P - C|^2 = r^2 se reduz à equação de 2o grau
//   t^2 + 2b t + c = 0,   onde b = (O-C) . D   e   c = (O-C).(O-C) - r^2.
// As raízes são t = -b +/- sqrt(b^2 - c). Retornamos a primeira que esteja à
// frente da origem (t > 0).
bool RaySphereIntersect(const glm::vec4& O, const glm::vec4& D,
                        const glm::vec4& C, float radius, float* t_out)
{
    // Vetor da origem do raio até o centro da esfera (apenas componentes x,y,z).
    float ocx = O.x - C.x;
    float ocy = O.y - C.y;
    float ocz = O.z - C.z;

    float b = ocx*D.x + ocy*D.y + ocz*D.z;
    float c = ocx*ocx + ocy*ocy + ocz*ocz - radius*radius;

    float disc = b*b - c;
    if (disc < 0.0f)
        return false; // o raio não toca a esfera

    float sq = std::sqrt(disc);
    float t0 = -b - sq;
    float t1 = -b + sq;

    float t = (t0 > 0.0f) ? t0 : t1; // primeira interseção à frente
    if (t <= 0.0f)
        return false; // a esfera está atrás da origem

    if (t_out != nullptr)
        *t_out = t;
    return true;
}

// Colisão círculo-círculo no plano XZ. Se a distância entre o jogador e o centro
// do obstáculo for menor que a soma dos raios, há sobreposição: empurramos o
// jogador para fora, ao longo da direção que liga o centro do obstáculo ao
// jogador, até que eles fiquem apenas tangentes.
bool ResolveCircleCollisionXZ(float* px, float* pz, float pr,
                              float cx, float cz, float cr)
{
    float dx = *px - cx;
    float dz = *pz - cz;
    float dist = std::sqrt(dx*dx + dz*dz);
    float min_dist = pr + cr;

    if (dist < min_dist && dist > 1e-4f)
    {
        float push = (min_dist - dist) / dist;
        *px += dx * push;
        *pz += dz * push;
        return true;
    }
    return false;
}
