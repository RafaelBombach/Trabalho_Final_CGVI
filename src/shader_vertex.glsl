#version 330 core

// Atributos de vértice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a função BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Intensidade da batida de asas dos patos (oscila no tempo, calculada no C++).
// Para os demais objetos vale 0 (sem deformação). Veja shader_vertex em main.cpp.
uniform float u_flap;

// Atributos de vértice que serão gerados como saída ("out") pelo Vertex Shader.
// ** Estes serão interpolados pelo rasterizador! ** gerando, assim, valores
// para cada fragmento, os quais serão recebidos como entrada pelo Fragment
// Shader. Veja o arquivo "shader_fragment.glsl".
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;

void main()
{
    // A variável gl_Position define a posição final de cada vértice
    // OBRIGATORIAMENTE em "normalized device coordinates" (NDC), onde cada
    // coeficiente estará entre -1 e 1 após divisão por w.
    // Veja {+NDC2+}.
    //
    // O código em "main.cpp" define os vértices dos modelos em coordenadas
    // locais de cada modelo (array model_coefficients). Abaixo, utilizamos
    // operações de modelagem, definição da câmera, e projeção, para computar
    // as coordenadas finais em NDC (variável gl_Position). Após a execução
    // deste Vertex Shader, a placa de vídeo (GPU) fará a divisão por W. Veja
    // slides 41-67 e 69-86 do documento Aula_09_Projecoes.pdf.

    // Animação procedural de batida de asas (apenas patos; para os demais
    // objetos u_flap = 0). As "asas" são os vértices afastados lateralmente do
    // plano de simetria do corpo (que neste modelo fica em ~z=0.06, não em z=0).
    // Deslocamos esses vértices em Y (para cima/baixo) proporcionalmente ao quão
    // distante estão desse plano, criando a batida. Um "gate" suave em X exclui a
    // CABEÇA (parte da frente do modelo) para que ela não se mexa junto.
    // A oscilação no tempo vem de u_flap (calculada no C++).
    vec4 mc = model_coefficients;
    // Este modelo só tem UMA asa espalhada (lado +z). Deformamos apenas essa
    // asa, excluindo a cabeça (frente, x baixo) e as pernas (baixo, y baixo)
    // para não mexê-las junto.
    float wing     = max((mc.z - 0.06) - 0.10, 0.0); // só o lado +z (a asa real)
    float bodyGate = smoothstep(-0.10, 0.02, mc.x);  // 0 na cabeça/pescoço (frente)
    float legGate  = smoothstep(-0.22, -0.14, mc.y); // 0 nas pernas (parte de baixo)
    mc.y += u_flap * wing * bodyGate * legGate;

    gl_Position = projection * view * model * mc;

    // Como as variáveis acima  (tipo vec4) são vetores com 4 coeficientes,
    // também é possível acessar e modificar cada coeficiente de maneira
    // independente. Esses são indexados pelos nomes x, y, z, e w (nessa
    // ordem, isto é, 'x' é o primeiro coeficiente, 'y' é o segundo, ...):
    //
    //     gl_Position.x = model_coefficients.x;
    //     gl_Position.y = model_coefficients.y;
    //     gl_Position.z = model_coefficients.z;
    //     gl_Position.w = model_coefficients.w;
    //

    // Agora definimos outros atributos dos vértices que serão interpolados pelo
    // rasterizador para gerar atributos únicos para cada fragmento gerado.

    // Posição do vértice atual no sistema de coordenadas global (World).
    position_world = model * mc;

    // Posição do vértice atual no sistema de coordenadas local do modelo.
    position_model = mc;

    // Normal do vértice atual no sistema de coordenadas global (World).
    // Veja slides 123-151 do documento Aula_07_Transformacoes_Geometricas_3D.pdf.
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    // Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
    texcoords = texture_coefficients;
}

