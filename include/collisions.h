#ifndef COLLISIONS_H
#define COLLISIONS_H

// Testes de colisão / interseção do jogo Duck Hunt 3D.
// (O enunciado do trabalho exige que os testes de colisão fiquem em um arquivo
//  separado chamado "collisions.cpp".)

#include <glm/vec4.hpp>

// Interseção RAIO-ESFERA.
// Lança um raio com origem 'O' e direção 'D' (deve estar normalizada) e testa a
// interseção com a esfera de centro 'C' e raio 'radius'. Em caso de interseção
// à frente da origem, retorna true e escreve em '*t_out' o parâmetro 't' da
// primeira interseção (distância ao longo do raio). Usado pelo tiro (picking de
// patos) e pelo bloqueio do tiro por pedras/árvores.
bool RaySphereIntersect(const glm::vec4& O, const glm::vec4& D,
                        const glm::vec4& C, float radius, float* t_out);

// Colisão CÍRCULO-CÍRCULO no plano XZ.
// Testa se o jogador (posição px,pz e raio pr) penetra um obstáculo circular
// (centro cx,cz e raio cr). Se houver sobreposição, empurra o jogador para fora
// (ajustando *px e *pz) e retorna true. Usado para impedir o jogador de
// atravessar pedras e troncos de árvores.
bool ResolveCircleCollisionXZ(float* px, float* pz, float pr,
                              float cx, float cz, float cr);

#endif // COLLISIONS_H
