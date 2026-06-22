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

---

## Etapa 1.1 — Ajustes: avatar do jogador, escala e alcance de visão

**Prompt (resumo da instrução dada à IA):**

> Com base no feedback de teste: diminuir a escala do pato; aumentar o alcance
> de visão (far plane estava em apenas 10 unidades, cortando o chão); e adicionar
> um avatar visível para o jogador, pois na câmera de 3ª pessoa não havia nenhum
> personagem físico para ver. Usar o modelo do coelho (`bunny.obj`) como
> placeholder do jogador, texturizado (3ª imagem de textura) e posicionado sobre
> o chão, girando para a direção em que o jogador olha; desenhá-lo apenas na
> câmera de 3ª pessoa.

**Arquivos afetados:** `src/main.cpp`, `src/shader_fragment.glsl`.

---

## Etapa 2 — Patos voando: instâncias + curva de Bézier cúbica

**Prompt (resumo da instrução dada à IA):**

> Implementar os patos voando como múltiplas INSTÂNCIAS do mesmo modelo (mesmo
> VBO, Model matrices diferentes). Parte dos patos deve voar em linha reta
> atravessando o mapa, e parte deve voar de forma suave ao longo de uma curva de
> Bézier cúbica (com 4 pontos de controle), orientando-se na direção do
> movimento (tangente da curva). Toda a movimentação baseada em delta-time.
> Depois, a partir do feedback de teste, reduzir a velocidade dos patos.

**Arquivos afetados:** `src/main.cpp`.

---

## Etapa 2.1 — Céu azul e curvas de Bézier mais visíveis

**Prompt (resumo da instrução dada à IA):**

> Adicionar um céu azul (cor de fundo). Adicionar mais patos e tornar algumas
> curvas de Bézier bem mais acentuadas (pontos de controle exagerados: arcos
> altos, "S", "U", grandes oscilações laterais), pois as curvas estavam suaves
> demais e a trajetória curva não era perceptível visualmente.

**Arquivos afetados:** `src/main.cpp`.

---

## Etapa 2.2 — Campo de visão maior e pato em zig-zag (Bézier composta)

**Prompt (resumo da instrução dada à IA):**

> Aumentar o campo de visão total (FOV). Adicionar um pato com trajetória em
> zig-zag muito agudo. Como uma Bézier cúbica simples (4 pontos) só dobra uma
> vez, implementar uma Bézier composta (vários segmentos cúbicos encadeados) com
> cantos agudos para obter o zig-zag.

**Arquivos afetados:** `src/main.cpp`.

---

## Etapa 3 — Tiro, mira e placar (interseção raio-esfera)

**Prompt (resumo da instrução dada à IA):**

> Implementar o tiro: cruzeta no centro da tela, e o clique esquerdo do mouse
> lança um raio a partir do olho do jogador na direção do olhar. Testar
> interseção raio-esfera com a esfera envolvente de cada pato vivo (teste de
> interseção com propósito); o pato mais próximo atingido é abatido, some, o
> placar incrementa e o pato reaparece após alguns segundos. Mostrar o placar na
> tela. Corrigir bug: a avaliação da curva de Bézier produzia pontos com w != 1
> (erro de ponto flutuante), o que fazia o dotproduct() de matrices.h abortar o
> programa ao atirar — forçar w=1 nas posições dos patos.

**Arquivos afetados:** `src/main.cpp`.

---

## Etapa 4 — Colisões do jogador (limites do mapa + obstáculos)

**Prompt (resumo da instrução dada à IA):**

> Implementar colisões do jogador: ele não pode sair dos limites do terreno e
> não pode atravessar obstáculos. Como ainda não há um modelo de árvore em .obj
> (o asset veio como COLLADA .dae), usar rochas (instâncias da esfera,
> texturizadas com projeção esférica) como obstáculos por enquanto. A colisão
> com os obstáculos é um teste de interseção círculo-círculo no plano XZ que
> empurra o jogador para fora da rocha.

**Arquivos afetados:** `src/main.cpp`, `src/shader_fragment.glsl`.

---

## Etapa 4.1 — Pedras naturais, muralha na borda e tiro bloqueado por pedra

**Prompt (resumo da instrução dada à IA):**

> Ajustar as pedras: deixá-las menores e parcialmente enterradas no solo (centro
> abaixo do chão) para parecerem rochas naturais e não bolas. Fazer o tiro ser
> bloqueado por pedras: se uma pedra estiver antes do pato no caminho do raio, o
> pato atrás dela não deve ser atingido (testar interseção raio-pedra e comparar
> distâncias). Adicionar uma muralha de pedras ao redor da borda do mapa, no
> lugar da parede invisível (mantendo um limite de segurança na borda).

**Arquivos afetados:** `src/main.cpp`.

---

## Etapa 5 — Grama com canal de opacidade (alpha discard)

**Prompt (resumo da instrução dada à IA):**

> Adicionar grama espalhada pelo chão usando o modelo Gras.obj (tufo de vários
> retângulos cruzados), texturizada com a imagem difusa e recortada pelo canal
> de opacidade. Usar alpha test (discard no fragment shader) para recortar o
> formato das folhas, e desligar o backface culling ao desenhar a grama para que
> os quads sejam visíveis dos dois lados. Espalhar centenas de tufos com
> posição/escala/rotação pseudo-aleatórias (semente fixa). Corrigir a
> orientação do modelo (vinha deitado, pois o asset é Z-up do 3ds Max): aplicar
> rotação de correção de -90° em X e calcular automaticamente a altura da base a
> partir da bounding box rotacionada para plantar o tufo sobre o chão.

**Arquivos afetados:** `src/main.cpp`, `src/shader_fragment.glsl`,
`data/Gras.*`, `data/graeser_mischung*.{png,jpg}`.

---

## Etapa 6 — Árvores (tronco + folhas com alpha) e obstáculos

**Prompt (resumo da instrução dada à IA):**

> Integrar a árvore (sugar maple) que a dupla converteu de COLLADA para OBJ. O
> modelo tem 2071 shapes e vários materiais (casca, folha, além de chão e grama
> embutidos que NÃO queremos). Escrever um script Python (tools/split_tree.py)
> que separa o modelo em dois OBJ limpos: tronco (material de casca) e folhas
> (material de folha), descartando o chão/grama e re-indexando vértices. Carregar
> os dois modelos, texturizar o tronco com a casca e as folhas com a textura RGBA
> (recorte por alpha discard). Corrigir orientação (Z-up do 3ds Max) com rotação
> de -90° em X e base automática. As árvores são obstáculos: o tronco bloqueia o
> jogador (colisão XZ) e a copa (aproximada por uma esfera) bloqueia tiros.
> Adaptar LoadTextureImage para carregar texturas com canal alpha (RGBA).

**Arquivos afetados:** `src/main.cpp`, `src/shader_fragment.glsl`,
`tools/split_tree.py`, `data/tree_trunk.obj`, `data/tree_leaves.obj`,
`data/sugar_maple_bark.jpg`, `data/sugar_maple_leaf.png`, `data/FONTES.txt`.

---

## Etapa 6.1 — Folhas das árvores: filtragem anisotrópica e limiar de alpha

**Prompt (resumo da instrução dada à IA):**

> As folhas, vistas de perto e de baixo (ângulo rasante), ficavam quase
> transparentes (dissolviam em fios), pois o filtro trilinear borra o alpha em
> ângulos rasantes. Habilitar filtragem anisotrópica em todas as texturas (token
> da extensão EXT_texture_filter_anisotropic) para preservar o detalhe nesses
> ângulos, e baixar o limiar do alpha discard das folhas (0.5 -> 0.3) para
> manter mais área de folha.

**Arquivos afetados:** `src/main.cpp`, `src/shader_fragment.glsl`.

---

## Etapa 6.2 — Copa cheia: núcleo opaco e alpha nítido nas folhas

**Prompt (resumo da instrução dada à IA):**

> A correção anterior piorou: vista de baixo, a copa quase sumia. Causa: mipmap
> + anisotropia em textura de recorte, em ângulo rasante, misturam folha+céu e
> derrubam o alpha. Correção: nas texturas com alpha (folhagem), usar filtro
> LINEAR sem mipmap e sem anisotropia (alpha nítido); manter mipmap/anisotropia
> apenas nas texturas opacas. Além disso, adicionar uma copa opaca interna
> (elipsoide verde, um pouco menor que a extensão das folhas) por baixo das
> folhas, para que não se veja o céu através/por baixo da árvore.

**Arquivos afetados:** `src/main.cpp`, `src/shader_fragment.glsl`.
