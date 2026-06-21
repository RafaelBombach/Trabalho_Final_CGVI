# Registro de uso de IA (Duck Hunt 3D)

Este arquivo registra os prompts utilizados para gerar/editar código com
ferramentas de IA neste trabalho, conforme exigido pelo enunciado.

- Os commits cujo código foi gerado/editado com auxílio de IA têm o prefixo
  `PROMPT:` na mensagem do commit, para fácil localização.
- Trechos de código com origem específica (não-IA) estão marcados no código
  fonte com comentários contendo a palavra `FONTE`.
- A ferramenta de IA utilizada foi a Claude Code (modelo Claude Opus), operando
  diretamente sobre o repositório a partir das instruções da dupla.

---

## Etapa 1 — Fundação: câmera FPS, chão e pato

**Prompt (resumo da instrução dada à IA):**

> A partir do código base do Laboratório 5 da disciplina, transformar o projeto
> na fundação do jogo "Duck Hunt 3D" (um FPS): remover os modelos de
> demonstração (esfera, coelho) e, no lugar, carregar um plano texturizado como
> chão e o modelo do pato (`data/pato.obj`). Implementar uma câmera de 1ª pessoa
> livre, controlada pelo mouse (mouse-look, cursor capturado) e por WASD para
> andar sobre o chão, com toda a movimentação baseada em delta-time. Adicionar
> também uma câmera de 3ª pessoa (look-at) que orbita o jogador, alternável com
> a tecla C. Implementar iluminação de Blinn-Phong no fragment shader para o
> chão e o pato. Não utilizar funções proibidas pelo enunciado (glm::lookAt,
> glm::perspective, etc.) — usar apenas as funções de `matrices.h`.

**Arquivos afetados:** `src/main.cpp`, `src/shader_fragment.glsl`,
`data/pato.*`.
