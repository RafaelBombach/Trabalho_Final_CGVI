//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     CÓDIGO BASE PARA O TRABALHO FINAL
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Biblioteca de áudio (apenas as declarações aqui; a implementação está em
// src/miniaudio_impl.cpp). FONTE: miniaudio (https://github.com/mackron/miniaudio).
#include "miniaudio.h"

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "collisions.h" // testes de colisão/interseção (implementados em collisions.cpp)

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename, bool with_alpha = false); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// ===================== Estado do jogo Duck Hunt 3D =====================
// (Daqui para baixo, código do trabalho final; o que estava acima é em
//  grande parte o código base do Laboratório 5 da disciplina.)

// Tipos de câmera implementados. A câmera de 1a pessoa é uma câmera LIVRE
// (a posição é controlada pelo jogador). A de 3a pessoa é uma câmera LOOK-AT
// que orbita o jogador.
enum CameraMode { CAMERA_FIRST_PERSON, CAMERA_THIRD_PERSON };
CameraMode g_CameraMode = CAMERA_FIRST_PERSON;

// Posição do jogador no mundo (sobre o plano do chão, em XZ). A altura dos
// "olhos" (câmera de 1a pessoa) é somada a esta posição no loop de render.
glm::vec4 g_PlayerPosition = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

// Estado das teclas de movimento (WASD). Atualizado em KeyCallback() e lido
// no loop principal para mover o jogador de forma contínua e baseada no tempo.
bool g_KeyW = false;
bool g_KeyA = false;
bool g_KeyS = false;
bool g_KeyD = false;

// Variáveis de animação baseada no tempo (delta-time). Garantem que a
// movimentação ocorra na mesma velocidade independente do FPS da máquina.
float g_DeltaTime    = 0.0f; // Tempo (s) entre o quadro atual e o anterior
float g_LastFrameTime = 0.0f; // Instante (s) em que o quadro anterior começou

// ---- Patos (instâncias) ----
// Cada pato é uma INSTÂNCIA do mesmo modelo (mesmo VBO), desenhada com uma
// Model matrix diferente. Há dois tipos de trajetória de voo:
//   PATH_LINEAR - voo em linha reta atravessando o mapa.
//   PATH_BEZIER - voo suave ao longo de uma curva de Bézier cúbica.
//   PATH_BEZIER_CHAIN - Bézier composta (vários segmentos cúbicos encadeados),
//                       permitindo trajetórias em zig-zag agudo.
enum DuckPath { PATH_LINEAR, PATH_BEZIER, PATH_BEZIER_CHAIN };
struct Duck
{
    DuckPath  path;
    // Linear: começa em 'start' e anda na direção 'dir' por 'range' unidades.
    glm::vec4 start;
    glm::vec4 dir;
    float     range;
    // Bézier simples: 4 pontos de controle da curva cúbica.
    glm::vec4 b0, b1, b2, b3;
    // Bézier composta: lista de pontos de controle (3*nsegmentos + 1 pontos).
    std::vector<glm::vec4> chain;
    // Comum:
    float param;     // parâmetro de avanço, acumulado por delta-time
    float speed;     // velocidade do avanço do parâmetro
    float scale;     // escala do modelo
    glm::vec4 worldPos = glm::vec4(0.0f,0.0f,0.0f,1.0f); // posição atual no mundo (p/ o tiro)
    float respawnAt = 0.0f; // instante (s) em que o pato reaparece após cair

    // Estado do pato: voando (alvo válido), caindo (abatido, despencando) ou
    // morto (escondido até reaparecer).
    int   state = 0; // 0 = voando, 1 = caindo, 2 = morto
    glm::vec4 fwd   = glm::vec4(0.0f,0.0f,1.0f,0.0f); // direção horizontal do voo (p/ a queda)
    float lastYaw = 0.0f;     // yaw no instante do abate (mantém a direção ao cair)
    glm::vec4 fallPos = glm::vec4(0.0f,0.0f,0.0f,1.0f); // posição durante a queda
    glm::vec4 fallVel = glm::vec4(0.0f,0.0f,0.0f,0.0f); // velocidade durante a queda
    float fallSpin = 0.0f;    // ângulo de tombamento acumulado durante a queda
};
// Estados possíveis de um pato.
#define DUCK_FLYING  0
#define DUCK_FALLING 1
#define DUCK_DEAD    2
std::vector<Duck> g_Ducks;

// Ajuste de orientação do modelo do pato, para o bico apontar na direção do
// voo. Valor calibrado em tempo real com as teclas '[' e ']'.
float g_DuckYawOffset = 6.8941f; // 395 graus (equivalente a 35 graus)

// Origem e direção (normalizada) do "tiro", atualizadas a cada quadro: o tiro
// parte do olho do jogador na direção em que ele está olhando (centro da mira).
glm::vec4 g_ShotOrigin    = glm::vec4(0.0f,0.0f,0.0f,1.0f);
glm::vec4 g_ShotDirection = glm::vec4(0.0f,0.0f,1.0f,0.0f);

// Placar (número de patos abatidos).
int g_Score = 0;

// ---- Áudio (miniaudio) ----
// Engine de áudio de alto nível. Usamos ma_engine_play_sound() para tocar
// efeitos "fire-and-forget" (que podem se sobrepor), como tiro e quack.
ma_engine g_AudioEngine;
bool      g_AudioReady = false;

// Toca um arquivo de som (se o áudio foi inicializado com sucesso).
void PlayGameSound(const char* filename)
{
    if (g_AudioReady)
        ma_engine_play_sound(&g_AudioEngine, filename, NULL);
}

// ---- Obstáculos (rochas) ----
// Objetos com os quais o jogador colide (não pode atravessar). Cada um é uma
// instância da esfera, com posição no chão e um raio de colisão (em XZ).
struct Obstacle
{
    glm::vec4 pos;   // posição no plano (x, z); y não é usado aqui
    float     scale; // escala (raio) do modelo da esfera
    float     radius;// raio de colisão no plano XZ
    float     drawY; // altura do CENTRO da esfera ao desenhar (negativa = enterrada)
};
std::vector<Obstacle> g_Obstacles;

// Metade do tamanho do chão (o terreno vai de -50 a +50 em X e Z). O jogador
// é mantido dentro destes limites (colisão com a borda do mapa).
const float g_MapHalfSize = 49.0f;

// ---- Grama ----
// Cada tufo de grama é uma instância do modelo Gras.obj, espalhado pelo chão
// com posição, escala e rotação próprias.
struct GrassPatch
{
    float x, z;   // posição no chão
    float scale;  // escala do tufo
    float rot;    // rotação em torno de Y (variedade visual)
};
std::vector<GrassPatch> g_Grass;

// Nomes dos shapes que compõem o modelo de grama (todos desenhados por tufo).
const char* g_GrassShapes[] = {
    "Rectangle001", "Rectangle002", "Rectangle003",
    "Rectangle004", "Rectangle005", "Rectangle006",
};

// Rotação de correção aplicada ao modelo de grama (o asset foi exportado do
// 3ds Max, cujo eixo "para cima" é Z; giramos -90° em X para deixá-lo em pé no
// nosso mundo Y-up). A altura da base é calculada automaticamente a partir da
// bounding box rotacionada (ver loop de desenho), então mudar este ângulo já
// reposiciona o tufo sobre o chão.
const float g_GrassRotX = -1.5707963f; // -90 graus

// Bounding box (em coordenadas do modelo) do tufo de grama, obtida do Gras.obj.
const glm::vec3 g_GrassBBoxMin = glm::vec3(-5.27f, -4.11f, -0.87f);
const glm::vec3 g_GrassBBoxMax = glm::vec3( 9.85f, 10.77f,  5.70f);

// ---- Árvores ----
// Cada árvore é uma instância dos modelos tronco+folhas. A árvore é Z-up (3ds
// Max), então aplicamos -90° em X (como na grama). Serve também de obstáculo:
// o tronco bloqueia o jogador (colisão XZ) e a copa bloqueia tiros (esfera).
struct Tree
{
    float x, z;   // posição no chão
    float scale;  // escala do modelo
    float rot;    // rotação em torno de Y
};
std::vector<Tree> g_Trees;

const float g_TreeRotX = -1.5707963f; // -90 graus (Z-up -> Y-up)
const glm::vec3 g_TreeBBoxMin = glm::vec3(-122.44f, -115.7f,  -1.27f);
const glm::vec3 g_TreeBBoxMax = glm::vec3( 116.7f,  126.6f, 324.15f);

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// Avalia um ponto da curva de Bézier cúbica definida pelos pontos de controle
// p0..p3, no parâmetro t em [0,1].
//   B(t) = (1-t)^3 p0 + 3(1-t)^2 t p1 + 3(1-t) t^2 p2 + t^3 p3
glm::vec4 BezierPoint(glm::vec4 p0, glm::vec4 p1, glm::vec4 p2, glm::vec4 p3, float t)
{
    float u = 1.0f - t;
    return (u*u*u) * p0
         + (3.0f*u*u*t) * p1
         + (3.0f*u*t*t) * p2
         + (t*t*t) * p3;
}

// Vetor tangente (derivada) da curva de Bézier cúbica no parâmetro t. Usado
// para orientar o pato na direção do movimento.
//   B'(t) = 3(1-t)^2 (p1-p0) + 6(1-t)t (p2-p1) + 3t^2 (p3-p2)
glm::vec4 BezierTangent(glm::vec4 p0, glm::vec4 p1, glm::vec4 p2, glm::vec4 p3, float t)
{
    float u = 1.0f - t;
    return (3.0f*u*u) * (p1 - p0)
         + (6.0f*u*t) * (p2 - p1)
         + (3.0f*t*t) * (p3 - p2);
}

// Monta uma Bézier composta a partir de uma lista de pontos-âncora. Os dois
// pontos de controle internos de cada segmento são interpolados linearmente
// (1/3 e 2/3) entre as âncoras, o que produz cantos agudos -> trajetória em
// zig-zag. Retorna 3*nsegmentos + 1 pontos de controle.
std::vector<glm::vec4> BuildBezierChain(const std::vector<glm::vec4>& anchors)
{
    std::vector<glm::vec4> cp;
    for (size_t i = 0; i + 1 < anchors.size(); ++i)
    {
        glm::vec4 a = anchors[i];
        glm::vec4 b = anchors[i+1];
        if (i == 0)
            cp.push_back(a);
        cp.push_back(a + (b - a) * (1.0f/3.0f));
        cp.push_back(a + (b - a) * (2.0f/3.0f));
        cp.push_back(b);
    }
    return cp;
}

// Cria os patos da cena. Misturamos patos de voo linear e patos de voo por
// curva de Bézier cúbica, em diferentes altitudes e com fases distintas.
void SpawnDucks()
{
    g_Ducks.clear();

    // --- Patos de voo LINEAR (atravessam o mapa) ---
    for (int i = 0; i < 5; ++i)
    {
        Duck d;
        d.path  = PATH_LINEAR;
        float z = -30.0f + i * 18.0f;     // faixas de Z diferentes
        float y = 6.0f + (i % 2) * 3.0f;  // altitudes alternadas
        d.start = glm::vec4(-55.0f, y, z, 1.0f);
        d.dir   = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f); // voa no sentido +X
        d.range = 110.0f;                  // atravessa e reaparece do outro lado
        d.param = i * 25.0f;               // fase inicial (espalha os patos)
        d.speed = 4.0f + i * 0.6f;         // unidades por segundo
        d.scale = 3.0f;
        d.state = DUCK_FLYING;
        g_Ducks.push_back(d);
    }

    // --- Patos de voo por curva de BÉZIER cúbica ---
    // Pontos de controle EXAGERADOS de propósito (P1/P2 bem afastados da reta
    // P0->P3) para que a curvatura seja bem visível: arcos altos, "S" no plano
    // horizontal e grandes oscilações laterais.
    glm::vec4 curves[6][4] = {
        // Arco alto (sobe bastante no meio do trajeto)
        { glm::vec4(-45,  4,  0, 1), glm::vec4(-15, 28,  0, 1),
          glm::vec4( 15, 28,  0, 1), glm::vec4( 45,  4,  0, 1) },
        // "S" no plano horizontal (zig-zag acentuado)
        { glm::vec4(-45,  9,-45, 1), glm::vec4(-45,  9, 45, 1),
          glm::vec4( 45,  9,-45, 1), glm::vec4( 45,  9, 45, 1) },
        // Grandes oscilações laterais
        { glm::vec4(  0,  6,-48, 1), glm::vec4( 65, 14,-12, 1),
          glm::vec4(-65, 14, 12, 1), glm::vec4(  0,  6, 48, 1) },
        // Arco diagonal subindo e descendo forte
        { glm::vec4(-40,  3, 35, 1), glm::vec4(  5, 30, 10, 1),
          glm::vec4( -5, 30,-10, 1), glm::vec4( 40,  3,-35, 1) },
        // "U" profundo (mergulha e sobe)
        { glm::vec4(-30, 20,-25, 1), glm::vec4(-10,  2,-25, 1),
          glm::vec4( 10,  2, 25, 1), glm::vec4( 30, 20, 25, 1) },
        // Curva suave (mantemos uma mais discreta para variedade)
        { glm::vec4( 40,  8, 40, 1), glm::vec4( 10, 16, 10, 1),
          glm::vec4(-10, 16,-10, 1), glm::vec4(-40,  8,-40, 1) },
    };
    for (int i = 0; i < 6; ++i)
    {
        Duck d;
        d.path  = PATH_BEZIER;
        d.b0 = curves[i][0]; d.b1 = curves[i][1];
        d.b2 = curves[i][2]; d.b3 = curves[i][3];
        d.param = i * 0.17f;         // fase inicial (espalha ao longo das curvas)
        d.speed = 0.05f + i*0.01f;   // ciclos por segundo
        d.scale = 3.0f;
        d.state = DUCK_FLYING;
        g_Ducks.push_back(d);
    }

    // --- Pato de voo em ZIG-ZAG agudo (Bézier composta de vários segmentos) ---
    {
        std::vector<glm::vec4> anchors = {
            glm::vec4(-52, 11,   0, 1),
            glm::vec4(-32, 11,  24, 1),
            glm::vec4(-16, 11, -24, 1),
            glm::vec4(  0, 11,  24, 1),
            glm::vec4( 16, 11, -24, 1),
            glm::vec4( 32, 11,  24, 1),
            glm::vec4( 52, 11, -12, 1),
        };
        Duck d;
        d.path  = PATH_BEZIER_CHAIN;
        d.chain = BuildBezierChain(anchors);
        d.param = 0.0f;
        d.speed = 0.06f;
        d.scale = 3.5f;
        d.state = DUCK_FLYING;
        g_Ducks.push_back(d);
    }
}

// Cria as rochas (obstáculos): algumas espalhadas pelo mapa e uma muralha de
// pedras ao redor da borda. As pedras ficam parcialmente enterradas (centro
// abaixo do solo) para parecerem rochas naturais, e não esferas/bolas.
void SpawnObstacles()
{
    g_Obstacles.clear();

    // Função auxiliar que adiciona uma rocha. drawY negativo enterra ~30% do
    // raio no chão, deixando à mostra uma calota arredondada.
    auto addRock = [&](float x, float z, float scale)
    {
        Obstacle o;
        o.pos    = glm::vec4(x, 0.0f, z, 1.0f);
        o.scale  = scale;
        o.radius = scale;             // raio de colisão no plano XZ
        o.drawY  = -0.30f * scale;    // centro abaixo do solo (parcialmente enterrada)
        g_Obstacles.push_back(o);
    };

    // --- Rochas espalhadas pelo interior do mapa (menores que antes) ---
    addRock( 10.0f,  -8.0f, 2.0f);
    addRock(-14.0f,  12.0f, 2.4f);
    addRock( 22.0f,  20.0f, 1.6f);
    addRock(-25.0f, -18.0f, 2.6f);
    addRock(  5.0f,  28.0f, 2.1f);
    addRock(-30.0f,  30.0f, 1.8f);

    // --- Muralha de pedras ao redor da borda do mapa ---
    // As pedras se sobrepõem (diâmetro > espaçamento) para formar uma parede
    // contínua, sem frestas por onde o jogador passe.
    const float border = 46.0f;
    const float step   = 5.0f;
    for (float c = -48.0f; c <= 48.0f; c += step)
    {
        float s = 3.0f + 0.5f * sinf(c * 0.6f); // leve variação de tamanho
        addRock( border, c, s);   // borda +X
        addRock(-border, c, s);   // borda -X
        addRock( c,  border, s);  // borda +Z
        addRock( c, -border, s);  // borda -Z
    }
}

// Espalha tufos de grama pelo chão, com posição/escala/rotação pseudo-aleatórias
// (semente fixa para a cena ser sempre a mesma).
void SpawnGrass()
{
    g_Grass.clear();
    srand(1234);
    const int num_patches = 250;
    for (int i = 0; i < num_patches; ++i)
    {
        GrassPatch g;
        g.x     = -45.0f + (rand() / (float)RAND_MAX) * 90.0f; // [-45, 45]
        g.z     = -45.0f + (rand() / (float)RAND_MAX) * 90.0f;
        g.scale = 0.07f + (rand() / (float)RAND_MAX) * 0.06f;  // [0.07, 0.13]
        g.rot   = (rand() / (float)RAND_MAX) * 6.2831853f;     // [0, 2pi]
        g_Grass.push_back(g);
    }
}

// Cria as árvores em posições fixas pelo mapa (evitando o centro, onde o
// jogador começa).
void SpawnTrees()
{
    g_Trees.clear();
    float spots[][2] = {
        { 18.0f, 14.0f}, {-20.0f, 10.0f}, { 25.0f,-22.0f}, {-28.0f,-15.0f},
        { 12.0f, 30.0f}, {-15.0f, 35.0f}, { 35.0f,  8.0f}, {-35.0f, 25.0f},
        {  8.0f,-30.0f}, {-10.0f,-32.0f},
    };
    for (int i = 0; i < 10; ++i)
    {
        Tree t;
        t.x     = spots[i][0];
        t.z     = spots[i][1];
        t.scale = 0.035f + 0.010f * ((i % 3) / 2.0f); // leve variação de tamanho
        t.rot   = i * 0.7f;                            // rotações distintas
        g_Trees.push_back(t);
    }
}

// Dispara um tiro: lança um raio a partir de g_ShotOrigin na direção
// g_ShotDirection e testa interseção com a esfera envolvente de cada pato vivo.
// O pato mais próximo atingido é abatido e o placar é incrementado.
// (Este é o teste de interseção com propósito exigido pelo trabalho: picking
//  de patos via raio-esfera.)
void Shoot()
{
    // Som do disparo (toca em todo clique, acertando ou não).
    PlayGameSound("../../data/shot.wav");

    glm::vec4 O = g_ShotOrigin;
    glm::vec4 D = g_ShotDirection; // já normalizado

    float best_t = std::numeric_limits<float>::max();
    int   best_i = -1;

    for (size_t i = 0; i < g_Ducks.size(); ++i)
    {
        Duck& d = g_Ducks[i];
        if (d.state != DUCK_FLYING) // só patos voando podem ser atingidos
            continue;

        // Raio de colisão da esfera envolvente do pato (um pouco generoso para
        // facilitar a mira). O modelo tem ~0.5 un, escalado por d.scale.
        float r = d.scale * 0.6f;

        // Interseção raio-esfera (ver collisions.cpp).
        float t;
        if (RaySphereIntersect(O, D, d.worldPos, r, &t) && t < best_t)
        {
            best_t = t;
            best_i = (int)i;
        }
    }

    // Testamos também a interseção do raio com as pedras: se a pedra mais
    // próxima estiver ANTES do pato no caminho do tiro, o tiro é bloqueado
    // (o pato atrás da pedra não é atingido).
    float obstacle_t = std::numeric_limits<float>::max();
    for (size_t i = 0; i < g_Obstacles.size(); ++i)
    {
        glm::vec4 center = glm::vec4(g_Obstacles[i].pos.x, g_Obstacles[i].drawY,
                                     g_Obstacles[i].pos.z, 1.0f);
        float t;
        if (RaySphereIntersect(O, D, center, g_Obstacles[i].scale, &t) && t < obstacle_t)
            obstacle_t = t;
    }

    // Copa das árvores: aproximamos por uma esfera que também bloqueia o tiro
    // (um pato atrás da copa não é atingido).
    for (size_t i = 0; i < g_Trees.size(); ++i)
    {
        float s = g_Trees[i].scale;
        glm::vec4 center = glm::vec4(g_Trees[i].x, 200.0f * s, g_Trees[i].z, 1.0f);
        float t;
        if (RaySphereIntersect(O, D, center, 95.0f * s, &t) && t < obstacle_t)
            obstacle_t = t;
    }

    if (best_i >= 0 && best_t < obstacle_t)
    {
        // Pato atingido: inicia a QUEDA (em vez de sumir na hora). Despenca a
        // partir da posição atual, com inércia horizontal do voo e um pequeno
        // impulso para cima, tombando enquanto cai.
        Duck& d = g_Ducks[best_i];
        d.state    = DUCK_FALLING;
        d.fallPos  = d.worldPos;
        d.fallVel  = d.fwd * 4.0f;       // inércia horizontal na direção do voo
        d.fallVel.y = 2.0f;              // pequeno "tranco" para cima
        d.fallSpin = 0.0f;
        g_Score += 1;
        PlayGameSound("../../data/quack.wav"); // som do pato ao ser atingido
        printf("HIT! Pontos: %d\n", g_Score);
        fflush(stdout);
    }
    else if (best_i >= 0)
    {
        printf("Tiro bloqueado por uma pedra.\n");
        fflush(stdout);
    }
}

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "Duck Hunt 3D - INF01047", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Escondemos e capturamos o cursor do mouse: ele fica preso no centro da
    // janela e usamos seu movimento relativo para olhar ao redor (estilo FPS).
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Inicializamos a engine de áudio (miniaudio). Se falhar, o jogo continua
    // sem som (g_AudioReady = false).
    if (ma_engine_init(NULL, &g_AudioEngine) == MA_SUCCESS)
        g_AudioReady = true;
    else
        fprintf(stderr, "AVISO: não foi possível inicializar o áudio.\n");

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos as imagens de textura utilizadas pelo jogo. Cada chamada
    // associa a imagem a uma "texture unit" sequencial (TextureImage0, 1, ...).
    LoadTextureImage("../../data/pato_texture.jpeg");            // TextureImage0 - pato
    LoadTextureImage("../../data/rocky_terrain_02_diff_1k.jpg"); // TextureImage1 - chão
    LoadTextureImage("../../data/red_brick_diff_1k.jpg");        // TextureImage2 - jogador
    LoadTextureImage("../../data/graeser_mischung.png");         // TextureImage3 - grama (difusa)
    LoadTextureImage("../../data/graeser_mischung_opacity.jpg"); // TextureImage4 - grama (opacidade)
    LoadTextureImage("../../data/sugar_maple_bark.jpg");         // TextureImage5 - tronco (casca)
    LoadTextureImage("../../data/sugar_maple_leaf.png", true);   // TextureImage6 - folhas (RGBA, com alpha)
    LoadTextureImage("../../data/chao_base.jpeg");               // TextureImage7 - chão (terreno de floresta)

    // Construímos a representação dos objetos geométricos (malhas de triângulos).
    ObjModel patomodel("../../data/pato.obj");
    ComputeNormals(&patomodel);
    BuildTrianglesAndAddToVirtualScene(&patomodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    // Modelo do jogador (avatar visível na câmera de 3a pessoa). Por enquanto
    // usamos o coelho como placeholder; pode ser trocado por um modelo de
    // caçador/arma futuramente.
    ObjModel playermodel("../../data/bunny.obj");
    ComputeNormals(&playermodel);
    BuildTrianglesAndAddToVirtualScene(&playermodel);

    // Modelo usado como obstáculo (rocha).
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    // Modelo de grama (tufo com vários retângulos cruzados; usa textura com
    // canal de opacidade para recortar o formato das folhas).
    ObjModel grassmodel("../../data/Gras.obj");
    ComputeNormals(&grassmodel);
    BuildTrianglesAndAddToVirtualScene(&grassmodel);

    // Árvore (sugar maple) separada em tronco (casca) e folhas. As folhas usam
    // textura com canal alpha (recorte das folhas).
    ObjModel treetrunk("../../data/tree_trunk.obj");
    ComputeNormals(&treetrunk);
    BuildTrianglesAndAddToVirtualScene(&treetrunk);

    ObjModel treeleaves("../../data/tree_leaves.obj");
    ComputeNormals(&treeleaves);
    BuildTrianglesAndAddToVirtualScene(&treeleaves);

    // Criamos os patos da cena (instâncias com trajetórias linear e Bézier).
    SpawnDucks();

    // Criamos os obstáculos (rochas) com os quais o jogador colide.
    SpawnObstacles();

    // Espalhamos os tufos de grama pelo chão.
    SpawnGrass();

    // Posicionamos as árvores pelo mapa.
    SpawnTrees();

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(0.45f, 0.7f, 1.0f, 1.0f); // céu azul

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // ===================== Animação baseada no tempo =====================
        // Computamos o "delta-time": quanto tempo (em segundos) passou desde o
        // quadro anterior. TODAS as movimentações (jogador, patos, câmera) são
        // multiplicadas por g_DeltaTime, garantindo velocidade constante
        // independente do FPS da máquina (requisito de animação por tempo).
        float current_time = (float)glfwGetTime();
        g_DeltaTime = current_time - g_LastFrameTime;
        g_LastFrameTime = current_time;

        // ===================== Direção da câmera =====================
        // O vetor "view" (direção para onde a câmera/jogador olha) é definido a
        // partir dos ângulos de yaw (g_CameraTheta) e pitch (g_CameraPhi),
        // controlados pelo mouse. Coordenadas esféricas convertidas para
        // cartesianas.
        glm::vec4 camera_up_vector = glm::vec4(0.0f,1.0f,0.0f,0.0f);
        glm::vec4 view_direction = glm::vec4(
            cosf(g_CameraPhi)*sinf(g_CameraTheta),
            sinf(g_CameraPhi),
            cosf(g_CameraPhi)*cosf(g_CameraTheta),
            0.0f
        );

        // ===================== Movimentação do jogador (WASD) =====================
        // O jogador anda sobre o plano XZ. "forward" é o vetor view projetado no
        // chão (sem componente Y); "right" é perpendicular a ele.
        glm::vec4 forward = glm::vec4(view_direction.x, 0.0f, view_direction.z, 0.0f);
        if (norm(forward) > 1e-6f)
            forward = forward / norm(forward);
        glm::vec4 right = crossproduct(forward, camera_up_vector); // perpendicular no chão
        if (norm(right) > 1e-6f)
            right = right / norm(right);

        float player_speed = 8.0f; // unidades por segundo
        glm::vec4 move = glm::vec4(0.0f,0.0f,0.0f,0.0f);
        if (g_KeyW) move += forward;
        if (g_KeyS) move -= forward;
        if (g_KeyD) move += right;
        if (g_KeyA) move -= right;
        if (norm(move) > 1e-6f)
            g_PlayerPosition += (move / norm(move)) * player_speed * g_DeltaTime;

        // ===================== Colisões do jogador =====================
        // (1) Limites do mapa: o jogador não pode sair do terreno.
        if (g_PlayerPosition.x >  g_MapHalfSize) g_PlayerPosition.x =  g_MapHalfSize;
        if (g_PlayerPosition.x < -g_MapHalfSize) g_PlayerPosition.x = -g_MapHalfSize;
        if (g_PlayerPosition.z >  g_MapHalfSize) g_PlayerPosition.z =  g_MapHalfSize;
        if (g_PlayerPosition.z < -g_MapHalfSize) g_PlayerPosition.z = -g_MapHalfSize;

        // (2) Obstáculos (rochas): teste de interseção círculo-círculo no plano
        // XZ (ver collisions.cpp). Se o jogador penetra o raio de um obstáculo,
        // ele é empurrado para fora (impedindo atravessar a rocha).
        const float player_radius = 0.8f;
        for (size_t i = 0; i < g_Obstacles.size(); ++i)
        {
            ResolveCircleCollisionXZ(&g_PlayerPosition.x, &g_PlayerPosition.z,
                                     player_radius,
                                     g_Obstacles[i].pos.x, g_Obstacles[i].pos.z,
                                     g_Obstacles[i].radius);
        }

        // (3) Troncos das árvores: colisão círculo-círculo (o tronco é fino).
        const float trunk_radius = 1.3f;
        for (size_t i = 0; i < g_Trees.size(); ++i)
        {
            ResolveCircleCollisionXZ(&g_PlayerPosition.x, &g_PlayerPosition.z,
                                     player_radius,
                                     g_Trees[i].x, g_Trees[i].z, trunk_radius);
        }

        // ===================== Definição da câmera virtual =====================
        // Implementamos dois tipos de câmera bem distintos (requisito do trabalho):
        //  - 1a pessoa: câmera LIVRE posicionada nos "olhos" do jogador.
        //  - 3a pessoa: câmera LOOK-AT que orbita o jogador a uma distância.
        const float eye_height = 1.7f;
        glm::vec4 camera_position_c;
        glm::vec4 camera_view_vector;

        if (g_CameraMode == CAMERA_FIRST_PERSON)
        {
            camera_position_c  = g_PlayerPosition + glm::vec4(0.0f, eye_height, 0.0f, 0.0f);
            camera_view_vector = view_direction;
        }
        else // CAMERA_THIRD_PERSON (look-at)
        {
            glm::vec4 lookat = g_PlayerPosition + glm::vec4(0.0f, eye_height, 0.0f, 0.0f);
            camera_position_c  = lookat - view_direction * g_CameraDistance;
            camera_view_vector = lookat - camera_position_c;
        }

        // O tiro sempre parte do olho do jogador na direção em que ele olha
        // (centro da mira), independente do modo de câmera. Atualizamos a cada
        // quadro para que o clique do mouse (MouseButtonCallback) use estes valores.
        g_ShotOrigin    = g_PlayerPosition + glm::vec4(0.0f, eye_height, 0.0f, 0.0f);
        g_ShotDirection = view_direction / norm(view_direction);

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;   // Posição do "near plane"
        float farplane  = -300.0f; // Posição do "far plane" (alcance de visão)

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 2.4f; // ~75 graus (campo de visão amplo)
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // Identificadores de cada tipo de objeto (devem casar com os #define do
        // arquivo shader_fragment.glsl).
        #define GROUND   0
        #define DUCK     1
        #define PLAYER   2
        #define OBSTACLE 3
        #define GRASS    4

        // ---- Chão ----
        // O plano "the_plane" é 2x2 em XZ; escalamos para cobrir um grande
        // terreno (100x100 unidades) centrado na origem.
        model = Matrix_Translate(0.0f, 0.0f, 0.0f)
              * Matrix_Scale(50.0f, 1.0f, 50.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, GROUND);
        DrawVirtualObject("the_plane");

        // ---- Patos (instâncias) ----
        // Cada pato avança sua trajetória com base no delta-time. A posição e a
        // orientação (yaw, alinhada à direção do movimento) definem a Model
        // matrix daquela instância. Todos compartilham o mesmo VBO ("0").
        glUniform1i(g_object_id_uniform, DUCK);
        for (size_t i = 0; i < g_Ducks.size(); ++i)
        {
            Duck& d = g_Ducks[i];

            // --- Pato MORTO: escondido até reaparecer ---
            if (d.state == DUCK_DEAD)
            {
                if ((float)glfwGetTime() >= d.respawnAt)
                    d.state = DUCK_FLYING;
                else
                    continue;
            }

            // --- Pato CAINDO: despenca com gravidade e tomba até o chão ---
            if (d.state == DUCK_FALLING)
            {
                d.fallVel.y -= 16.0f * g_DeltaTime;     // gravidade
                d.fallPos   += d.fallVel * g_DeltaTime; // integra a posição
                d.fallSpin  += 7.0f * g_DeltaTime;      // tombamento

                const float ground_y = 0.4f; // altura do corpo ao tocar o chão
                if (d.fallPos.y <= ground_y)
                {
                    d.state = DUCK_DEAD;
                    d.respawnAt = (float)glfwGetTime() + 2.0f;
                    continue; // some até reaparecer
                }

                model = Matrix_Translate(d.fallPos.x, d.fallPos.y, d.fallPos.z)
                      * Matrix_Rotate_Y(d.lastYaw)
                      * Matrix_Rotate_X(d.fallSpin)          // tombando ao cair
                      * Matrix_Rotate_Y(g_DuckYawOffset)
                      * Matrix_Scale(d.scale, d.scale, d.scale);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                DrawVirtualObject("0");
                continue;
            }

            // --- Pato VOANDO: avança a trajetória ---
            d.param += d.speed * g_DeltaTime;

            glm::vec4 pos;
            glm::vec4 tangent;

            if (d.path == PATH_LINEAR)
            {
                if (d.param > d.range) d.param -= d.range; // reaparece do início
                pos = d.start + d.dir * d.param;
                tangent = d.dir;
            }
            else if (d.path == PATH_BEZIER)
            {
                if (d.param > 1.0f) d.param -= 1.0f; // a curva é percorrida em loop
                pos = BezierPoint(d.b0, d.b1, d.b2, d.b3, d.param);
                tangent = BezierTangent(d.b0, d.b1, d.b2, d.b3, d.param);
            }
            else // PATH_BEZIER_CHAIN (Bézier composta: zig-zag)
            {
                if (d.param > 1.0f) d.param -= 1.0f;
                int nseg = (int)(d.chain.size() - 1) / 3; // segmentos cúbicos
                float tt = d.param * nseg;                // posição global na cadeia
                int seg = (int)tt;
                if (seg >= nseg) seg = nseg - 1;
                float lt = tt - seg;                      // parâmetro local do segmento
                glm::vec4 c0 = d.chain[3*seg + 0];
                glm::vec4 c1 = d.chain[3*seg + 1];
                glm::vec4 c2 = d.chain[3*seg + 2];
                glm::vec4 c3 = d.chain[3*seg + 3];
                pos = BezierPoint(c0, c1, c2, c3, lt);
                tangent = BezierTangent(c0, c1, c2, c3, lt);
            }

            // Guardamos a posição atual no mundo para o teste de tiro (raycast).
            // Forçamos w=1 (é um ponto): a avaliação da Bézier soma termos de
            // Bernstein e o w resultante pode não ser exatamente 1.0 por erro de
            // ponto flutuante, o que faria o dotproduct() de matrices.h abortar.
            pos.w = 1.0f;
            d.worldPos = pos;

            // Orientamos o pato ao longo da tangente 3D da trajetória, para que
            // ele "olhe" na direção do voo (deixa as curvas bem mais perceptíveis):
            //  - yaw   = rotação horizontal (direção no plano XZ);
            //  - pitch = inclinação vertical (nariz sobe ao subir, desce ao descer).
            float horiz = sqrtf(tangent.x*tangent.x + tangent.z*tangent.z);
            float yaw   = atan2f(tangent.x, tangent.z);
            float pitch = atan2f(tangent.y, horiz);

            // Guardamos a direção horizontal e o yaw atuais: se o pato for
            // abatido, a queda continua nessa direção, mantendo o sentido do voo.
            d.fwd = (horiz > 1e-6f) ? glm::vec4(tangent.x/horiz, 0.0f, tangent.z/horiz, 0.0f)
                                    : glm::vec4(0.0f,0.0f,1.0f,0.0f);
            d.lastYaw = yaw;

            model = Matrix_Translate(pos.x, pos.y, pos.z)
                  * Matrix_Rotate_Y(yaw)
                  * Matrix_Rotate_X(-pitch)
                  * Matrix_Rotate_Y(g_DuckYawOffset)
                  * Matrix_Scale(d.scale, d.scale, d.scale);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            DrawVirtualObject("0"); // nome do shape dentro de pato.obj
        }

        // ---- Jogador (avatar) ----
        // Desenhamos o corpo do jogador apenas na câmera de 3a pessoa (na 1a
        // pessoa a câmera está "dentro" dele). O avatar é posicionado em
        // g_PlayerPosition e girado para a direção em que o jogador olha (yaw).
        if (g_CameraMode == CAMERA_THIRD_PERSON)
        {
            // O coelho tem ~2 un de altura e é centrado na origem; com escala
            // 0.9 ele fica ~1.8 un (altura humana). Somamos 0.9*0.991 em Y para
            // que os "pés" fiquem sobre o chão (y=0).
            model = Matrix_Translate(g_PlayerPosition.x, g_PlayerPosition.y + 0.89f, g_PlayerPosition.z)
                  * Matrix_Rotate_Y(g_CameraTheta)
                  * Matrix_Scale(0.9f, 0.9f, 0.9f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, PLAYER);
            DrawVirtualObject("the_bunny");
        }

        // ---- Obstáculos (rochas) ----
        // Cada rocha é uma instância da esfera (raio ~1, centrada na origem);
        // posicionamos o centro em y=escala para a esfera repousar sobre o chão.
        glUniform1i(g_object_id_uniform, OBSTACLE);
        for (size_t i = 0; i < g_Obstacles.size(); ++i)
        {
            float s = g_Obstacles[i].scale;
            model = Matrix_Translate(g_Obstacles[i].pos.x, g_Obstacles[i].drawY, g_Obstacles[i].pos.z)
                  * Matrix_Scale(s, s, s);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            DrawVirtualObject("the_sphere");
        }

        // ---- Grama ----
        // Os tufos são quads cruzados com recorte por opacidade (alpha discard
        // no fragment shader). Desligamos o backface culling para que as folhas
        // sejam visíveis dos dois lados.
        //
        // Aplicamos uma rotação de correção (g_GrassRotX) para deixar o tufo em
        // pé. Calculamos o menor Y dos 8 cantos da bbox APÓS essa rotação, para
        // depois transladar a base para o chão (y=0), independente do ângulo.
        glm::mat4 grass_correction = Matrix_Rotate_X(g_GrassRotX);
        float grass_min_y = std::numeric_limits<float>::max();
        for (int cx = 0; cx < 2; ++cx)
        for (int cy = 0; cy < 2; ++cy)
        for (int cz = 0; cz < 2; ++cz)
        {
            glm::vec4 corner(
                cx ? g_GrassBBoxMax.x : g_GrassBBoxMin.x,
                cy ? g_GrassBBoxMax.y : g_GrassBBoxMin.y,
                cz ? g_GrassBBoxMax.z : g_GrassBBoxMin.z,
                1.0f);
            glm::vec4 r = grass_correction * corner;
            if (r.y < grass_min_y) grass_min_y = r.y;
        }

        glDisable(GL_CULL_FACE);
        glUniform1i(g_object_id_uniform, GRASS);
        for (size_t i = 0; i < g_Grass.size(); ++i)
        {
            float s = g_Grass[i].scale;
            model = Matrix_Translate(g_Grass[i].x, -grass_min_y * s, g_Grass[i].z)
                  * Matrix_Rotate_Y(g_Grass[i].rot)
                  * grass_correction
                  * Matrix_Scale(s, s, s);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            for (size_t k = 0; k < sizeof(g_GrassShapes)/sizeof(g_GrassShapes[0]); ++k)
                DrawVirtualObject(g_GrassShapes[k]);
        }
        glEnable(GL_CULL_FACE);

        // ---- Árvores ----
        #define TREE_TRUNK  5
        #define TREE_LEAVES 6
        // Rotação de correção (Z-up -> Y-up) e cálculo automático da base, como
        // na grama.
        glm::mat4 tree_correction = Matrix_Rotate_X(g_TreeRotX);
        float tree_min_y = std::numeric_limits<float>::max();
        for (int cx = 0; cx < 2; ++cx)
        for (int cy = 0; cy < 2; ++cy)
        for (int cz = 0; cz < 2; ++cz)
        {
            glm::vec4 corner(
                cx ? g_TreeBBoxMax.x : g_TreeBBoxMin.x,
                cy ? g_TreeBBoxMax.y : g_TreeBBoxMin.y,
                cz ? g_TreeBBoxMax.z : g_TreeBBoxMin.z, 1.0f);
            glm::vec4 r = tree_correction * corner;
            if (r.y < tree_min_y) tree_min_y = r.y;
        }

        glDisable(GL_CULL_FACE); // troncos/folhas convertidos: evita faces faltando
        for (size_t i = 0; i < g_Trees.size(); ++i)
        {
            float s = g_Trees[i].scale;
            model = Matrix_Translate(g_Trees[i].x, -tree_min_y * s, g_Trees[i].z)
                  * Matrix_Rotate_Y(g_Trees[i].rot)
                  * tree_correction
                  * Matrix_Scale(s, s, s);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));

            glUniform1i(g_object_id_uniform, TREE_TRUNK);
            DrawVirtualObject("tree_trunk");

            glUniform1i(g_object_id_uniform, TREE_LEAVES);
            DrawVirtualObject("tree_leaves");
        }
        glEnable(GL_CULL_FACE);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // HUD: cruzeta de mira no centro da tela e placar no canto superior
        // esquerdo (sempre visíveis, independente de g_ShowInfoText).
        {
            float cw = TextRendering_CharWidth(window);
            float lh = TextRendering_LineHeight(window);
            TextRendering_PrintString(window, "+", -cw*0.5f, -lh*0.5f, 1.5f);
            char score_buffer[64];
            snprintf(score_buffer, 64, "Pontos: %d", g_Score);
            TextRendering_PrintString(window, score_buffer, -1.0f + cw, 1.0f - lh, 1.0f);
        }

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos a engine de áudio.
    if (g_AudioReady)
        ma_engine_uninit(&g_AudioEngine);

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura.
// Se with_alpha == true, a imagem é carregada com 4 canais (RGBA), preservando
// o canal de transparência (usado, por exemplo, no recorte das folhas).
void LoadTextureImage(const char* filename, bool with_alpha)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    int req_comp = with_alpha ? 4 : 3;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, req_comp);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Modo de wrap: as texturas opacas usam GL_REPEAT para que possam ser
    // "tileadas" (ex.: o chão, cuja UV vai de 0 a 8). As texturas de recorte
    // (folhagem) usam GL_CLAMP_TO_EDGE para evitar bleeding nas bordas dos cards.
    GLint wrap_mode = with_alpha ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, wrap_mode);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, wrap_mode);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Filtragem anisotrópica APENAS nas texturas opacas: mantém o chão (e
    // pedras) nítido em ângulos rasantes / à distância. Sem isso, o filtro
    // trilinear deixa o chão nítido perto e borrado longe, criando um "disco"
    // de detalhe que parece acompanhar o jogador. NÃO aplicamos nas texturas de
    // recorte (folhas), pois a anisotropia mistura folha+céu e as faz sumir.
    if (!with_alpha)
    {
        #ifndef GL_TEXTURE_MAX_ANISOTROPY
        #define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
        #endif
        #ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY
        #define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
        #endif
        GLfloat max_aniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
        glSamplerParameterf(sampler_id, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);
    }

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    GLint  internalformat = with_alpha ? GL_SRGB8_ALPHA8 : GL_SRGB8;
    GLenum format         = with_alpha ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice e que pertencem ao mesmo "smoothing group".

    // Obtemos a lista dos smoothing groups que existem no objeto
    std::set<unsigned int> sgroup_ids;
    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        assert(model->shapes[shape].mesh.smoothing_group_ids.size() == num_triangles);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            unsigned int sgroup = model->shapes[shape].mesh.smoothing_group_ids[triangle];
            assert(sgroup >= 0);
            sgroup_ids.insert(sgroup);
        }
    }

    size_t num_vertices = model->attrib.vertices.size() / 3;
    model->attrib.normals.reserve( 3*num_vertices );

    // Processamos um smoothing group por vez
    for (const unsigned int & sgroup : sgroup_ids)
    {
        std::vector<int> num_triangles_per_vertex(num_vertices, 0);
        std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

        // Acumulamos as normais dos vértices de todos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                glm::vec4  vertices[3];
                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                    const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                    const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                    vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
                }

                const glm::vec4  a = vertices[0];
                const glm::vec4  b = vertices[1];
                const glm::vec4  c = vertices[2];

                const glm::vec4  n = crossproduct(b-a,c-a);

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    num_triangles_per_vertex[idx.vertex_index] += 1;
                    vertex_normals[idx.vertex_index] += n;
                }
            }
        }

        // Computamos a média das normais acumuladas
        std::vector<size_t> normal_indices(num_vertices, 0);

        for (size_t vertex_index = 0; vertex_index < vertex_normals.size(); ++vertex_index)
        {
            if (num_triangles_per_vertex[vertex_index] == 0)
                continue;

            glm::vec4 n = vertex_normals[vertex_index] / (float)num_triangles_per_vertex[vertex_index];
            n /= norm(n);

            model->attrib.normals.push_back( n.x );
            model->attrib.normals.push_back( n.y );
            model->attrib.normals.push_back( n.z );

            size_t normal_index = (model->attrib.normals.size() / 3) - 1;
            normal_indices[vertex_index] = normal_index;
        }

        // Escrevemos os índices das normais para os vértices dos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index =
                        normal_indices[ idx.vertex_index ];
                }
            }
        }

    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::lowest();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;

        // Clique esquerdo = atirar. Lança o raio da mira e abate o pato atingido.
        Shoot();
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Como o cursor está capturado (GLFW_CURSOR_DISABLED), este callback é
    // chamado continuamente com a posição virtual do mouse. Usamos o
    // deslocamento relativo para girar a câmera no estilo FPS (mouse-look).

    // Na primeira chamada não há posição anterior válida; apenas a registramos
    // para evitar um "salto" brusco da câmera.
    static bool first_mouse = true;
    if (first_mouse)
    {
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
        first_mouse = false;
        return;
    }

    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;

    const float sensitivity = 0.003f;
    g_CameraTheta -= sensitivity * dx; // yaw   (olhar para os lados)
    g_CameraPhi   -= sensitivity * dy; // pitch (olhar para cima/baixo)

    // Limitamos o pitch para não "virar de cabeça para baixo" (evita também a
    // singularidade da câmera look-at quando view fica paralelo ao up).
    const float phimax = 3.141592f/2 - 0.01f;
    const float phimin = -phimax;
    if (g_CameraPhi > phimax) g_CameraPhi = phimax;
    if (g_CameraPhi < phimin) g_CameraPhi = phimin;
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

void Correcao_KeyCallback(int key, int action, int mod);

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // =======================
    // Não modifique esta chamada! Ela é utilizada para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    Correcao_KeyCallback(key, action, mod);
    // =======================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // ===== Movimentação do jogador (WASD) =====
    // Guardamos apenas o ESTADO (pressionada/solta) de cada tecla. O
    // deslocamento em si é aplicado no loop principal, multiplicado por
    // g_DeltaTime, para que a velocidade seja constante (animação por tempo).
    if (action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        bool pressed = (action == GLFW_PRESS);
        if (key == GLFW_KEY_W) g_KeyW = pressed;
        if (key == GLFW_KEY_A) g_KeyA = pressed;
        if (key == GLFW_KEY_S) g_KeyS = pressed;
        if (key == GLFW_KEY_D) g_KeyD = pressed;
    }

    // ===== Alterna entre câmera de 1a e 3a pessoa (tecla C) =====
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        g_CameraMode = (g_CameraMode == CAMERA_FIRST_PERSON)
                     ? CAMERA_THIRD_PERSON : CAMERA_FIRST_PERSON;
    }

    // ===== Ajuste fino da orientação do pato (teclas '[' e ']') =====
    // Gira todos os patos em torno de Y para alinhar o bico com a direção do
    // voo. Imprime o valor atual no terminal para anotarmos o ângulo final.
    if ((key == GLFW_KEY_LEFT_BRACKET || key == GLFW_KEY_RIGHT_BRACKET)
        && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        g_DuckYawOffset += (key == GLFW_KEY_RIGHT_BRACKET ? 1.0f : -1.0f) * 0.0872665f; // ~5 graus
        printf("g_DuckYawOffset = %.4f rad (%.1f graus)\n",
               g_DuckYawOffset, g_DuckYawOffset * 180.0f / 3.141592f);
        fflush(stdout);
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

