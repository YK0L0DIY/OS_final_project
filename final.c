#include <stdio.h>
#include <stdlib.h>
#include "queue.c"
#include <stdbool.h>

#define N_MAXIMO_DE_INSTRUCOES 30
#define N_MAXIMO_DE_PROCESSOS 30
#define MAX_MEMORIA 300
#define QUANTUM 4
#define FIT 1               // 1 -> bestfit | 0 -> nexfit

int disk;
int memoria[MAX_MEMORIA];
int apontadorDaUltimaAlocacao = 0;

struct pcb {
    int id;
    int pc;
    short estado;
};

struct pcb *novoPcb(int id, int pc, short estado) {
    struct pcb *pcb = malloc(sizeof(struct pcb));
    pcb->id = id;
    pcb->pc = pc;
    pcb->estado = estado; //-1 - n introduzido, 0 -NEW, 1 - WAIT, 2 - RUN, 3 - BLOCK, 4 - EXIT
    return pcb;
}

struct processo {
    int instante;
    int quantum;
    int tempo_que_precisa_de_ficar_em_block;
    int codigo[N_MAXIMO_DE_INSTRUCOES * 3];
    int maxPc;
    int posicaoInicial;
    int posicaoFinal;
    struct pcb *pcb;
};

struct processo *novoProcesso(int instante, int codigo[N_MAXIMO_DE_INSTRUCOES * 3], struct pcb *pcb, int size) {
    struct processo *processo = malloc(sizeof(struct processo));
    int i;
    processo->instante = instante;
    for (i = 0; i <= size; i++) {
        processo->codigo[i] = codigo[i];
    }
    processo->posicaoInicial = 0;
    processo->posicaoFinal = 0;
    processo->quantum = 0;
    processo->tempo_que_precisa_de_ficar_em_block = 3;
    processo->pcb = pcb;
    processo->maxPc = size / 3;
    return processo;
}

int obterPosicao(struct processo *processo) {

//    printf("\n ### \n obterPosicao \n ### \n");

    int inicio,
            novoInicio,
            fim,
            espacoDisponivel = MAX_MEMORIA + 1,
            espacoNecessario = processo->maxPc * 3 + 10;

    if (FIT == 1) {          //bestfit

        inicio = -1;

        for (int x = 0; x < MAX_MEMORIA; x++) {

            //Só prossegue quando encontrar pelo menos um espaço livre.
            if (memoria[x] != -1) {
                continue;

            } else {

                novoInicio = x;        //Quando encontrar um espaço livre guarda-o.

                while (x < MAX_MEMORIA && memoria[x] == -1) {
                    fim = x;    //Vai atualizando o ultimo espaço livre encontrado.
                    x++;
                }

                int diferenca = fim - novoInicio;

                //Calcula o tamanho do espaço encontrado e verifica se o processo cabe, se couber retorna a posição para onde pode começar a copiar.
                if (diferenca < espacoDisponivel && diferenca >= espacoNecessario) {
                    espacoDisponivel = diferenca;
                    inicio = novoInicio;
                }
            }

        }
        return inicio;

    } else {            // nextfit

        inicio = -1;

        //Search for a space that is greater or equal to the space needed.
        for (int x = apontadorDaUltimaAlocacao; x < MAX_MEMORIA; x++) {

            //Só prossegue quando encontrar pelo menos um espaço livre.
            if (memoria[x] != -1) {
                continue;

            } else {

                inicio = x;        //Quando encontrar um espaço livre guarda-o.

                while (x < MAX_MEMORIA && memoria[x] == -1) {
                    fim = x;    //Vai atualizando o ultimo espaço livre encontrado.
                    x++;
                }

                //Calcula o tamanho do espaço encontrado e verifica se o processo cabe, se couber retorna a posição para onde pode começar a copiar.
                if ((fim - inicio) >= espacoNecessario) {
                    return inicio;
                }
            }

        }
        //Caso não tenha encontrado do ultimo apontador até ao fim, vê do inicio ao ultimo apontador.
        for (int y = 0; y < apontadorDaUltimaAlocacao; y++) {

            inicio = y;

            while (y < apontadorDaUltimaAlocacao && memoria[y] == -1) {
                fim = y++;
            }

            if ((fim - inicio) >= espacoNecessario) {
                return inicio;
            }
        }
        return -1;
    }
}

void copiarParaMemoria(struct processo *processo, int posicao) {

//    printf("\n ### \n copiarParaMemoria \n ### \n");
    processo->posicaoInicial = posicao;

    //Copiar o codigo do processo para a memoria.

    posicao += 10;

    for (int i = 0; i < (processo->maxPc) * 3; i++) {
        memoria[posicao] = processo->codigo[i];
        posicao++;
    }

    processo->posicaoFinal = posicao - 1;
    apontadorDaUltimaAlocacao = posicao;
}

void limparExit(int *processo_em_exit, int *n_processos_corridos, struct processo *processos[]) {

//    printf("\n ### \n limparExit \n ### \n");

    if (*processo_em_exit != -1) {
        for (int x = processos[*processo_em_exit]->posicaoInicial;
             x <= processos[*processo_em_exit]->posicaoFinal; x++) {
            memoria[x] = -1;
        }
        processos[*processo_em_exit]->pcb->estado = -2;
        (*processo_em_exit) = -1;
        (*n_processos_corridos)++;
    }

}

void percorrerBlock(queue *block, queue *wait, struct processo *processos[], int first, int last) {

//    printf("\n ### \n percorrerBlock \n ### \n");

    int inst, variavel, prox, posicao;

    for (int i = first; i <= last; i++) {
        processos[block->Q[i]]->tempo_que_precisa_de_ficar_em_block--;

        if (processos[block->Q[i]]->tempo_que_precisa_de_ficar_em_block == 0) {

            posicao = (processos[i]->pcb->pc * 3) - 3;

            inst = memoria[processos[i]->posicaoInicial + 10 + posicao];
            variavel = memoria[processos[i]->posicaoInicial + 10 + posicao + 1];

            if (inst == 8) {
                disk = memoria[variavel];
            } else {
                memoria[variavel] = disk;
            }
            prox = dequeue(block);

            processos[block->Q[i]]->pcb->estado = 1;
            enqueue(prox, wait);
        }
    }
}

void blockParaWait(queue *block, queue *wait, struct processo *processos[]) {

//    printf("\n ### \n blockParaWait \n ### \n");

    if (!isEmpty(block)) {
        if (block->first <= block->last) {
            percorrerBlock(block, wait, processos, block->first, block->last);
        } else {
            percorrerBlock(block, wait, processos, block->first, block->size);
            percorrerBlock(block, wait, processos, 0, block->last);
        }
    }
}

void runParaBlock(int *processo_em_run, queue *block, struct processo *processos[]) {

//    printf("\n ### \n runParaBlock \n ### \n");

    processos[*processo_em_run]->pcb->pc++;
    processos[*processo_em_run]->pcb->estado = 3;
    processos[*processo_em_run]->tempo_que_precisa_de_ficar_em_block = 3;
    enqueue(*processo_em_run, block);
    (*processo_em_run) = -1;

}

void runParaExit(int *processo_em_run, int *processo_em_exit, queue *block, struct processo *processos[]) {

//    printf("\n ### \n runParaExit \n ### \n");

    processos[*processo_em_run]->pcb->estado = 4;
    (*processo_em_exit) = (*processo_em_run);
    //enqueue(*processo_em_run, block); //burros tinham isto aqui
    (*processo_em_run) = -1;
}

void runParaWait(int *processo_em_run, queue *wait, struct processo *processos[]) {

//    printf("\n ### \n runParaWait \n ### \n");

    processos[*processo_em_run]->pcb->estado = 1;
    enqueue(*processo_em_run, wait);
    (*processo_em_run) = -1;
}

void newParaWait(int p_id, queue *wait, struct processo *processos[]) {

//    printf("\n ### \n newParaWait \n ### \n");


    int n_p = 0;
    while ((!isFull(wait)) && (n_p < p_id)) {
        if (processos[n_p]->pcb->estado == 0) {
            processos[n_p]->pcb->estado = 1;
            int posicao = obterPosicao(processos[n_p]);

            if (posicao != -1) {
                copiarParaMemoria(processos[n_p], posicao);
                enqueue(n_p, wait);
            }
        }
        n_p++;
    }
}

void waitParaRun(int *processo_em_run, queue *wait, struct processo *processos[]) {

//    printf("\n ### \n waitParaRun \n ### \n");

    if (((*processo_em_run) == -1) && (!isEmpty(wait))) {
        (*processo_em_run) = dequeue(wait);
        processos[*processo_em_run]->pcb->estado = 2; // processo passa para run
        processos[*processo_em_run]->quantum = QUANTUM;
    }
}

void receberParaNew(int timer, int p_id, struct processo *processos[]) {

//    printf("\n ### \n receberParaNew \n ### \n");

    for (int n_p = 0; n_p < p_id; n_p++) {
        if (processos[n_p]->instante == timer && processos[n_p]->pcb->estado == -1) {
            processos[n_p]->pcb->estado = 0;
        }
    }
}

void memoriaAlterda() {
    //TODO colocar print complegto de merda no ficheiro
}

void printEstados(int timer, int p_id, struct processo *processos[], int *print, int *fork) {

    //TODO coloccar no ficheiro
//    printf("\n ### \n printEstados \n ### \n");

    printf("T: %3d | ", timer);

    //print de todos os processos introduzidos
    for (int n_p = 0; n_p < p_id; n_p++) {
        switch (processos[n_p]->pcb->estado) {
            case -2:
                printf("      | ");
                break;
            case 0:
                printf("NEW   | ");
                break;
            case 1:
                printf("WAIT  | ");
                break;
            case 2:
                printf("RUN   | ");
                break;
            case 3:
                printf("BLOCK | ");
                break;
            case 4:
                printf("EXIT  | ");
                break;
            default:
                break;
        }

    }
    if (*print != -1) {
        printf("  %d", *print);
        *print = -1;
    }
    if (*fork == 1) {
        printf(" falha no fork");
        *fork = 0;
    }
    printf("\n");
}

void debugPrint(int p_id, struct processo *processos[]) {

    printf("\nPROCESSOS\n");
    struct processo *aux;

    for (int i = 0; i < p_id; i++) {
        aux = processos[i];
        printf("%d estado: %d | pi = %d | pc = %d | pf = %d", aux->pcb->id, aux->pcb->estado, aux->posicaoInicial,
               aux->pcb->pc, aux->posicaoFinal);
        printf("\n");
    }

}

int main(void) {

    int instante, p_id = 0, controlo, n_processos_corridos = 0,
            timer = 0, processo_em_run = -1, processo_em_exit = -1,
            algoParaImprimir = -1, falhaFork = 0;

    struct processo *processos[N_MAXIMO_DE_PROCESSOS] = {0};
    char test;

    //inicializa a memoria a -1 (posicao vazia)
    for (int i = 0; i < MAX_MEMORIA; i++) {
        memoria[i] = -1;
    }

    // le toda a informacao do ficheiro de teste
    while (scanf(" %d", &instante) != EOF) {
        int line[N_MAXIMO_DE_INSTRUCOES * 3] = {0};

        controlo = 0;
        struct pcb *nPCB = novoPcb(p_id, 0, -1);            // novo pcb para cada programa

        while (scanf("%c", &test) == 1) {                   // le a linha toda ate ao \n "codigo"

            if (test == '\n') {
                break;
            }

            //-----------------instrucao--------variavel 1-----------variavel 2
            scanf(" %d %d %d", &line[controlo], &line[controlo + 1], &line[controlo + 2]);
            controlo += 3;

        }
        line[controlo++] = -1;

        processos[p_id] = novoProcesso(instante, line, nPCB, controlo);
        p_id++;
    }

    queue *wait = new_queue();  // fila do wait
    queue *block = new_queue();  // fila do block

    while (1) {

        /*
            IF YOU DON'T WANT THE BLANK LINE MOVE THE IF AFTER THE limparExit() FUNTION.
        */
        //printf("\ttime: %d\n", timer);
        if (n_processos_corridos == p_id)
            break;

//        debugPrint(p_id, processos);

        // exit
        limparExit(&processo_em_exit, &n_processos_corridos, processos);

        // block -> wait
        // nao tem restrico para espço
        // block passa para wait direto
        blockParaWait(block, wait, processos);

        // run -> block se for necessario ou run -> exit
        // run -> wait verificar se atingiu o quantum Q=4 (max) se Q=0 passa para wait
        if (processo_em_run != -1) {

            //vai a memoria apanhar oque esta nas posicoes
            int pInicial = processos[processo_em_run]->posicaoInicial;
            int pc = processos[processo_em_run]->pcb->pc * 3;


            int inst = memoria[pInicial + pc + 10],
                    arg1 = memoria[pInicial + pc + 11],
                    arg2 = memoria[pInicial + pc + 12];

//            printf("\n pc + pi : %d , pc: %d \n ", pc + pInicial + 10, pc);
//            printf("%d %d %d\n", inst, arg1, arg2);

            int pmemoria, pmemoria2;

            switch (inst) {
                case 0:                                                 // x1=x2
//                    printf("executar case 0\n");
                    pmemoria = pInicial + arg1 - 1;
                    pmemoria2 = pInicial + arg2 - 1;
                    memoria[pmemoria] = memoria[pmemoria2];
//                    printf("memoria p: %d %d \n", pmemoria, pmemoria2);
                    processos[processo_em_run]->pcb->pc++;
                    break;

                case 1:                                                 // x=n
//                    printf("executar case 1\n");
                    pmemoria = pInicial + arg1 - 1;
                    memoria[pmemoria] = arg2;
//                    printf("memoria p: %d \n", pmemoria);
                    processos[processo_em_run]->pcb->pc++;
                    break;

                case 2:                                                 // x=x+1
//                    printf("executar case 2\n");
                    pmemoria = pInicial + arg1 - 1;
                    memoria[pmemoria] += 1;
//                    printf("memoria p: %d \n", pmemoria);
                    processos[processo_em_run]->pcb->pc++;
                    break;

                case 3:                                                 // x=x-1
//                    printf("executar case 3\n");
                    pmemoria = pInicial + arg1 - 1;
                    memoria[pmemoria] -= 1;
//                    printf("memoria p: %d \n", pmemoria);
                    processos[processo_em_run]->pcb->pc++;
                    break;

                case 4:                                                 // pc-=N
//                    printf("executar case 4\n");
                    if (processos[processo_em_run]->pcb->pc - arg1 >= 0)
                        processos[processo_em_run]->pcb->pc -= arg1;
                    else {
                        printf("MEMORY ACCESS VIOLATION 4\n");
                        runParaExit(&processo_em_run, &processo_em_exit, block, processos);
                    }
                    break;
                case 5:                                                 // pc+=N
//                    printf("executar case 5\n");
                    if (arg1 > 0) {
                        if (processos[processo_em_run]->pcb->pc + arg1 <= processos[processo_em_run]->maxPc)
                            processos[processo_em_run]->pcb->pc += arg1;
                        else {
                            printf("MEMORY ACCESS VIOLATION 5\n");
                            runParaExit(&processo_em_run, &processo_em_exit, block, processos);
                        }
                    }
                    break;
                case 6:                                                 // if x=0, pc+=N else pc++
//                    printf("executar case 6\n");
                    if (arg1 == 0) {
                        if (processos[processo_em_run]->pcb->pc += arg2 <= processos[processo_em_run]->maxPc)
                            processos[processo_em_run]->pcb->pc += arg2;
                        else {
                            printf("MEMORY ACCESS VIOLATION \n");
                            runParaExit(&processo_em_run, &processo_em_exit, block, processos);
                        }
                    } else
                        processos[processo_em_run]->pcb->pc++;

                    break;
                case 7:                                                 // fork
//                    printf("executar case 7\n");

                    if (!isFull(wait)) {
                        int posicao = obterPosicao(processos[processo_em_run]);

                        if (posicao != -1) {
                            struct pcb *newPcb = novoPcb(p_id, processos[processo_em_run]->pcb->pc + 1, 1);
                            processos[p_id] = novoProcesso(timer, processos[processo_em_run]->codigo, newPcb,
                                                           processos[processo_em_run]->maxPc * 3);
                            copiarParaMemoria(processos[p_id], posicao);

                            memoria[processos[p_id]->posicaoInicial + arg1 - 1] = 0;
                            memoria[processos[processo_em_run]->posicaoInicial + arg1 -
                                    1] = processos[processo_em_run]->pcb->id;
                            enqueue(p_id, wait);
                            p_id++;
                        } else {
                            falhaFork = 1;
                            memoria[processos[processo_em_run]->posicaoInicial + arg1 - 1] = -1;
                        }

                    } else {
                        falhaFork = 1;
                        memoria[processos[processo_em_run]->posicaoInicial + arg1 - 1] = -1;
                    }
                    processos[processo_em_run]->pcb->pc++;
                    break;
                case 8:                                                 // guardar no disco
//                    printf("executar case 8\n");
                    runParaBlock(&processo_em_run, block, processos);
                    break;
                case 9:                                                 // ler do disco
//                    printf("executar case 9\n");
                    runParaBlock(&processo_em_run, block, processos);
                    break;
                case 10:                                                // imprimir variavel
//                    printf("executar case 10\n");
                    algoParaImprimir = memoria[pInicial + arg1];
                    processos[processo_em_run]->pcb->pc++;
                    break;
                default:
//                    printf("executar case default/exit\n");
                    runParaExit(&processo_em_run, &processo_em_exit, block, processos);
                    break;
            }
            if (processo_em_run != -1) {
                processos[processo_em_run]->quantum--;
                if (processos[processo_em_run]->quantum == 0) {
                    runParaWait(&processo_em_run, wait, processos);
                }
            }
        }

        // new -> wait
        newParaWait(p_id, wait, processos);

        // wait -> run
        waitParaRun(&processo_em_run, wait, processos);

        // receber o programa no seu instante passando para new
        receberParaNew(timer, p_id, processos);

        //print do instante e do estado de cada processo que já foi introduzido
        printEstados(timer, p_id, processos, &algoParaImprimir, &falhaFork);

        if (memoriaAlterda) {
            printMemoria();
            memoriaAlterada = 0;
        }

        timer++;
        //          SHOW LE MEMORY
//        for (int i = 0; i < MAX_MEMORIA; i++) {
//            printf("%d ", memoria[i]);
//            /*if (memoria[i] == 11) {
//            	i++;
//            	printf("%d ", memoria[i]);
//            	i++;
//            	printf("%d \n", memoria[i]);
//            }*/
//        }
//        printf("\n");

    }

    //print de todos os processos introduzidos, descomentar para ver.
    //debugPrint(p_id, processos);

    return 0;
}