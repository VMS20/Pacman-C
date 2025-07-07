#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MAXFILAS 21
#define MAXCOLS 31

// Variáveis do jogo
int px = 14, py = 16;          // Posição do Pac-Man
int fx[4] = {13, 14, 15, 16};  // Posições dos fantasmas
int fy[4] = {10, 10, 10, 10};
int pos_fx[4] = {13, 14, 15, 16};  // Posições iniciais
int pos_fy[4] = {10, 10, 10, 10};
char direcao = ' ';
int pontos = 0;
int vidas = 3;
bool invencivel = false;
time_t tempo_invencivel;
time_t ultimo_modo = 0;
bool modo_perseguicao = true;

// Mapa do jogo
char mapa[MAXFILAS][MAXCOLS] = {
    "##############_##############", 
    "# . . . . . .   . . . . . . #",
    "#.###.#####.## ##.#####.###.#", 
    "#@###.##### ## ## #####.###@#",
    "#. . . . . . . . . . . . . .#", 
    "# ### ##.###########.## ### #",
    "#. . .## . . ### . . ##. . .#", 
    "# ### ######.###.###### ### #",
    "#.###.## . . . . . . ##.###.#", 
    "# . . ##.##### #####.## . . #",
    "| ###.  .#         #.  .### |", 
    "# . . ##.###########.## . . #",
    "#.###.## . . . . . . ##.###.#", 
    "# ### ######.###.###### ### #",
    "#. . .## . . ### . . ##. . .#", 
    "# ### ##.###########.## ### #",
    "#. . . . . . . . . . . . . .#", 
    "#@###.##### ## ## #####.###@#",
    "#.###.#####.## ##.#####.###.#", 
    "# . . . . . .   . . . . . . #",
    "##############_##############"};

// Função para capturar tecla pressionada
int getch(void)
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// Função para verificar se tecla foi pressionada
int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

// Função para imprimir o mapa
void imprimir_mapa()
{
    system("clear");
    for (int row = 0; row < MAXFILAS; row++)
    {
        for (int col = 0; col < MAXCOLS; col++)
        {
            if (row == py && col == px)
            {
                printf("Q");  // Pac-Man
            }
            else
            {
                bool fantasma_imprimido = false;
                for (int i = 0; i < 4; i++)
                {
                    if (row == fy[i] && col == fx[i])
                    {
                        printf(invencivel ? "u" : "U");  // Fantasma
                        fantasma_imprimido = true;
                        break;
                    }
                }
                if (!fantasma_imprimido)
                {
                    printf("%c", mapa[row][col]);
                }
            }
        }
        printf("\n");
    }

    // Painel de status
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║ Pontos: %-5d | Vidas: %-2d", pontos, vidas);

    if (invencivel)
    {
        double restante = 10 - difftime(time(NULL), tempo_invencivel);
        if (restante < 0)
            restante = 0;
        printf(" |   Invencível: %.0f seg ║\n", restante);
    }
    else
    {
        printf("                    ║\n");
    }
    printf("╚══════════════════════════════════════════════╝\n");
}

// Função para teletransporte
void teletransportar(int *novo_px, int *novo_py, char portal_char)
{
    for (int row = 0; row < MAXFILAS; row++)
    {
        for (int col = 0; col < MAXCOLS; col++)
        {
            if (mapa[row][col] == portal_char &&
                (row != *novo_py || col != *novo_px))
            {
                *novo_px = col;
                *novo_py = row;

                // Ajuste para não ficar preso na borda
                if (*novo_px == 0)
                    *novo_px += 1;
                else if (*novo_px == MAXCOLS - 1)
                    *novo_px -= 1;
                if (*novo_py == 0)
                    *novo_py += 1;
                else if (*novo_py == MAXFILAS - 1)
                    *novo_py -= 1;
                return;
            }
        }
    }
}

// Movimento do Pac-Man
void mover_pacman()
{
    int novo_px = px;
    int novo_py = py;

    switch (direcao)
    {
    case 'w':
        novo_py--;
        break;
    case 's':
        novo_py++;
        break;
    case 'a':
        novo_px--;
        break;
    case 'd':
        novo_px++;
        break;
    }

    // Teletransporte nas bordas
    if (novo_px < 0)
        novo_px = MAXCOLS - 1;
    if (novo_px >= MAXCOLS)
        novo_px = 0;
    if (novo_py < 0)
        novo_py = MAXFILAS - 1;
    if (novo_py >= MAXFILAS)
        novo_py = 0;

    // Verifica portais
    char destino = mapa[novo_py][novo_px];
    if (destino == '|' || destino == '_')
    {
        teletransportar(&novo_px, &novo_py, destino);
    }

    // Movimento válido
    if (mapa[novo_py][novo_px] != '#')
    {
        px = novo_px;
        py = novo_py;

        // Coleta de pontos
        if (mapa[py][px] == '.')
        {
            mapa[py][px] = ' ';
            pontos += 10;
        }
        else if (mapa[py][px] == '@')
        {
            mapa[py][px] = ' ';
            pontos += 50;
            invencivel = true;
            tempo_invencivel = time(NULL);
        }
    }
}

// Movimento dos fantasmas (versão aprimorada)
void mover_fantasmas()
{
    // Alternar modo a cada 10 segundos
    if (difftime(time(NULL), ultimo_modo) > 10)
    {
        modo_perseguicao = !modo_perseguicao;
        ultimo_modo = time(NULL);
    }

    for (int i = 0; i < 4; i++)
    {
        int novo_fx = fx[i];
        int novo_fy = fy[i];

        // 70% de chance de seguir comportamento do modo atual
        if (rand() % 10 < 7)
        {
            if (modo_perseguicao || invencivel)
            {
                // Persegue ou foge do Pac-Man
                if (invencivel)
                {
                    // Modo fuga
                    if (fx[i] < px)
                        novo_fx--;
                    else if (fx[i] > px)
                        novo_fx++;

                    if (fy[i] < py)
                        novo_fy--;
                    else if (fy[i] > py)
                        novo_fy++;
                }
                else
                {
                    // Modo perseguição
                    if (fx[i] < px)
                        novo_fx++;
                    else if (fx[i] > px)
                        novo_fx--;

                    if (fy[i] < py)
                        novo_fy++;
                    else if (fy[i] > py)
                        novo_fy--;
                }
            }
            else
            {
                // Modo dispersão (cada fantasma vai para seu canto)
                int alvo_x = (i % 2 == 0) ? 1 : MAXCOLS - 2;
                int alvo_y = (i < 2) ? 1 : MAXFILAS - 2;

                if (fx[i] < alvo_x)
                    novo_fx++;
                else if (fx[i] > alvo_x)
                    novo_fx--;

                if (fy[i] < alvo_y)
                    novo_fy++;
                else if (fy[i] > alvo_y)
                    novo_fy--;
            }
        }
        else
        {
            // 30% de movimento aleatório
            switch (rand() % 4)
            {
            case 0:
                novo_fy--;
                break;
            case 1:
                novo_fy++;
                break;
            case 2:
                novo_fx--;
                break;
            case 3:
                novo_fx++;
                break;
            }
        }

        // Verifica movimento válido
        if (novo_fx >= 0 && novo_fx < MAXCOLS && novo_fy >= 0 &&
            novo_fy < MAXFILAS && mapa[novo_fy][novo_fx] != '#' &&
            mapa[novo_fy][novo_fx] != '|' && mapa[novo_fy][novo_fx] != '_')
        {
            fx[i] = novo_fx;
            fy[i] = novo_fy;
        }
    }
}

// Verifica se o jogo acabou (todos pontos coletados)
bool game_over()
{
    for (int row = 0; row < MAXFILAS; row++)
    {
        for (int col = 0; col < MAXCOLS; col++)
        {
            if (mapa[row][col] == '.' || mapa[row][col] == '@')
            {
                return false;
            }
        }
    }
    return true;
}

// Verifica colisões com fantasmas
void verificar_colisoes()
{
    for (int i = 0; i < 4; i++)
    {
        if (px == fx[i] && py == fy[i])
        {
            if (invencivel)
            {
                // Fantasma é comido
                fx[i] = pos_fx[i];
                fy[i] = pos_fy[i];
                pontos += 200;
            }
            else
            {
                // Pac-Man perde vida
                vidas--;
                px = 14;
                py = 16;
                for (int j = 0; j < 4; j++)
                {
                    fx[j] = pos_fx[j];
                    fy[j] = pos_fy[j];
                }
                sleep(1);  // Pausa após perder vida
            }
        }
    }
}

// Atualiza estado de invencibilidade
void atualizar_invencibilidade()
{
    if (invencivel && difftime(time(NULL), tempo_invencivel) >= 10)
    {
        invencivel = false;
    }
}

// Função principal
int main()
{
    srand(time(NULL));
    ultimo_modo = time(NULL);

    while (vidas > 0)
    {
        imprimir_mapa();

        if (game_over())
        {
            printf("Você venceu! Pontuação final: %d\n", pontos);
            break;
        }

        if (kbhit())
        {
            direcao = tolower(getch());
            if (direcao == 'q')
            {
                printf("Jogo encerrado.\n");
                break;
            }
        }

        mover_pacman();
        mover_fantasmas();
        verificar_colisoes();
        atualizar_invencibilidade();

        usleep(200000);  // Controle de velocidade do jogo
    }

    if (vidas == 0)
    {
        printf("Game Over! Pontuação final: %d\n", pontos);
    }

    return 0;
}