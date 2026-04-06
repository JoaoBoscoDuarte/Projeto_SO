# 09 - Histórico de comandos no shell

## 🧾 Resumo da Mudança

Foi implementado suporte a histórico de comandos no shell, permitindo navegar entre comandos previamente executados utilizando as teclas de seta para cima e para baixo.

A solução foi construída diretamente no shell.c, sem modificar a interface do driver de teclado, mantendo a arquitetura do sistema simples e modular.

---

## 🎯 Objetivo

Adicionar uma funcionalidade básica de linha de comando interativa, permitindo:

- Reutilizar comandos anteriores
- Navegar no histórico com:
  - ↑ (seta para cima)
  - ↓ (seta para baixo)
- Melhorar a usabilidade do shell

---

## 🧠 Justificativa

O shell original apenas aceitava entrada linear via kbd_readline, sem qualquer mecanismo de edição ou histórico.

Para implementar o histórico sem impactar outras partes do sistema:

- O driver de teclado continua simples (apenas envia caracteres)
- O shell assume responsabilidade pela lógica de interação

Princípio adotado:

O driver captura entrada.  
O shell interpreta comportamento.

---

## ⚙️ Detalhes Técnicos

### 1. Estrutura de histórico

Foi adicionado um buffer fixo no shell.c:

    #define SHELL_MAX_LINE     128
    #define SHELL_HISTORY_SIZE 16

    static char shell_history[SHELL_HISTORY_SIZE][SHELL_MAX_LINE];
    static unsigned int shell_history_count = 0;

- Armazena até 16 comandos
- Cada comando possui até 128 caracteres

---

### 2. Inserção no histórico
No arquivo shell.c:

    static void shell_history_push(const char *cmd)
    {
        unsigned int i;

        if (!cmd || cmd[0] == '\0')
            return;

        if (shell_history_count > 0 &&
            strcmp(shell_history[shell_history_count - 1], cmd) == 0)
            return;

        if (shell_history_count < SHELL_HISTORY_SIZE) {
            strcpy(shell_history[shell_history_count], cmd);
            shell_history_count++;
            return;
        }

        for (i = 1; i < SHELL_HISTORY_SIZE; i++)
            strcpy(shell_history[i - 1], shell_history[i]);

        strcpy(shell_history[SHELL_HISTORY_SIZE - 1], cmd);
    }

---

### 3. Leitura de linha customizada
No arquivo shell.c:

Substituímos:

    kbd_readline(...)

por:

    shell_readline(...)

Essa função:

- lê caractere por caractere (kbd_getchar)
- trata Enter, Backspace e setas

---

### 4. Interpretação das setas
No arquivo shell.c:

As setas são recebidas como:

    ESC [ A  -> cima
    ESC [ B  -> baixo

Trecho:

    if (c == 27) {
        char c1 = kbd_getchar();
        char c2 = kbd_getchar();

        if (c1 == '[' && c2 == 'A') {
            // seta para cima
        }

        if (c1 == '[' && c2 == 'B') {
            // seta para baixo
        }
    }

---

### 5. Navegação no histórico
No arquivo shell.c:

    int hist_index = shell_history_count;

- começa fora do histórico
- ↑ navega para comandos antigos
- ↓ navega para comandos mais recentes

---

### 6. Redesenho da linha
No arquivo shell.c:

    shell_redraw_line(row, start_col, buf, old_len);

Função responsável por:

- apagar conteúdo anterior
- escrever novo comando
- reposicionar cursor

---

### 7. Integração no loop principal
No arquivo shell.c:

Antes:

    kbd_readline(buf, sizeof(buf));
    shell_execute(buf);

Depois:

    shell_readline(buf, sizeof(buf));
    shell_history_push(buf);
    shell_execute(buf);

---

## 🔄 Alterações no Driver de Teclado
No arquivo keyboard.c:

As setas foram mapeadas para sequências compatíveis:

    case 0x48:
        kbd_buffer_put(27);
        kbd_buffer_put('[');
        kbd_buffer_put('A');
        return;

    case 0x50:
        kbd_buffer_put(27);
        kbd_buffer_put('[');
        kbd_buffer_put('B');
        return;

---

## 🧪 Exemplos

Entrada:

    kernel> help
    kernel> ps
    kernel> info

↑:

    info
    ps
    help

---

Entrada parcial:

    kernel> sp

↑:

    spawn

↓

    sp

---

## ⚠️ Limitações

- sem edição no meio da linha
- sem setas esquerda/direita
- histórico fixo (16 comandos)

---

## ✅ Resultado

- histórico funcional
- navegação com setas implementada
- arquitetura preservada