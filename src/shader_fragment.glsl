#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader,
// gerados pelo rasterizador como interpolação dos atributos de vértice
// definidos em "shader_vertex.glsl".
in vec4 position_world;
in vec4 normal;
in vec4 position_model;
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento.
// DEVE casar com os #define correspondentes em "main.cpp".
#define GROUND   0
#define DUCK     1
#define PLAYER   2
#define OBSTACLE 3
#define GRASS    4
#define TREE_TRUNK  5
#define TREE_LEAVES 6
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Imagens de textura
uniform sampler2D TextureImage0; // pato
uniform sampler2D TextureImage1; // chão
uniform sampler2D TextureImage2; // jogador
uniform sampler2D TextureImage3; // grama (difusa)
uniform sampler2D TextureImage4; // grama (opacidade)
uniform sampler2D TextureImage5; // tronco (casca)
uniform sampler2D TextureImage6; // folhas (RGBA)
uniform sampler2D TextureImage7; // chão (terreno de floresta)

// Cor final do fragmento
out vec4 color;

#define M_PI   3.14159265358979323846

void main()
{
    // Posição da câmera (origem do sistema de câmera no mundo).
    vec4 camera_position = inverse(view) * vec4(0.0, 0.0, 0.0, 1.0);

    vec4 p = position_world;          // ponto sendo iluminado
    vec4 n = normalize(normal);       // normal interpolada

    // Fonte de luz direcional (um "sol"): vetor que aponta do ponto p para a luz.
    vec4 l = normalize(vec4(0.5, 1.0, 0.3, 0.0));
    // Vetor do ponto p em direção à câmera.
    vec4 v = normalize(camera_position - p);
    // Vetor "half" do modelo de Blinn-Phong.
    vec4 h = normalize(l + v);

    // Coordenadas de textura e propriedades de material por objeto.
    vec2 uv = texcoords;
    vec3 Kd;             // refletância difusa (vem da textura)
    vec3 Ks;             // refletância especular
    float shininess;     // expoente especular (Blinn-Phong)

    if ( object_id == GROUND )
    {
        // O plano possui UV em [0,1]; multiplicamos para "tilear" a textura do
        // terreno de floresta. Um fator menor deixa o padrão maior e mais
        // visível (com fator muito alto o padrão some no mipmap).
        uv = texcoords * 8.0;
        Kd = texture(TextureImage7, uv).rgb;
        // Sem especular no chão: num plano grande, o brilho especular forma uma
        // mancha que acompanha a câmera, dando a impressão de que a "textura
        // anda junto" com o jogador. Só difusa + ambiente => 100% estático.
        Ks = vec3(0.0);
        shininess = 1.0;
    }
    else if ( object_id == GRASS )
    {
        // Recorte das folhas: descartamos o fragmento onde a máscara de
        // opacidade é baixa (alpha test via discard, sem precisar de blending).
        float opacity = texture(TextureImage4, texcoords).r;
        if (opacity < 0.5)
            discard;
        Kd = texture(TextureImage3, texcoords).rgb;
        Ks = vec3(0.0);   // grama sem brilho especular
        shininess = 1.0;
    }
    else if ( object_id == TREE_TRUNK )
    {
        Kd = texture(TextureImage5, texcoords).rgb;
        Ks = vec3(0.05);
        shininess = 8.0;
    }
    else if ( object_id == TREE_LEAVES )
    {
        // Folhas recortadas pelo canal alpha da textura RGBA.
        vec4 leaf = texture(TextureImage6, texcoords);
        if (leaf.a < 0.5)
            discard;
        Kd = leaf.rgb;
        Ks = vec3(0.0);
        shininess = 1.0;
    }
    else if ( object_id == OBSTACLE )
    {
        // A esfera não possui coordenadas de textura no OBJ; geramos UV por
        // projeção esférica em coordenadas do modelo (centrada na bbox).
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;
        vec4 d = position_model - bbox_center;
        float rho   = length(vec3(d.x, d.y, d.z));
        float theta = atan(d.x, d.z);
        float phi   = asin(d.y / rho);
        uv = vec2((theta + M_PI) / (2.0*M_PI), (phi + M_PI/2.0) / M_PI);
        Kd = texture(TextureImage1, uv).rgb; // textura de terreno rochoso
        Ks = vec3(0.05);
        shininess = 8.0;
    }
    else if ( object_id == PLAYER )
    {
        // O modelo do jogador (coelho) não possui coordenadas de textura no
        // arquivo OBJ; geramos UV por projeção planar XY em coordenadas do
        // modelo, normalizadas pela bounding box.
        float u = (position_model.x - bbox_min.x) / (bbox_max.x - bbox_min.x);
        float w = (position_model.y - bbox_min.y) / (bbox_max.y - bbox_min.y);
        Kd = texture(TextureImage2, vec2(u, w)).rgb;
        Ks = vec3(0.1);
        shininess = 16.0;
    }
    else // DUCK
    {
        Kd = texture(TextureImage0, uv).rgb;
        Ks = vec3(0.3);
        shininess = 32.0;
    }

    // Cor/intensidade da luz e termo ambiente.
    vec3 light_spectrum   = vec3(1.0, 1.0, 1.0);
    vec3 ambient_spectrum = vec3(0.25, 0.25, 0.25);

    // Modelo de iluminação de Blinn-Phong:
    //   cor = ambiente + difusa (Lambert) + especular (Blinn-Phong)
    float lambert  = max(0.0, dot(n, l));
    float specular = pow(max(0.0, dot(n, h)), shininess);

    vec3 ambient_term  = Kd * ambient_spectrum;
    vec3 diffuse_term  = Kd * light_spectrum * lambert;
    vec3 specular_term = Ks * light_spectrum * specular;

    color.rgb = ambient_term + diffuse_term + specular_term;
    color.a = 1.0;

    // Correção gamma (monitor sRGB).
    color.rgb = pow(color.rgb, vec3(1.0)/2.2);
}
