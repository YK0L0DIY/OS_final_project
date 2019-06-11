#include <stdio.h>
#include <stdlib.h>
#include "queue.c"
#include <stdbool.h>

#define N_MAXIMO_DE_INSTRUCOES 30
#define N_MAXIMO_DE_PROCESSOS 30
#define MAX_MEMORIA 300
#define QUANTUM 4
#define FIT 0               // 1 -> bestfit | 0 -> nexfit

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

void copiarParaMemoria(struct processo *processos[], struct processo *processo, int posicao, int pid_pai) {

    processo->posicaoInicial = posicao;

    if (pid_pai != -1){

        int posIPai = processos[pid_pai]->posicaoInicial,
            posFPai = processos[pid_pai]->posicaoFinal;

        for (int i = posIPai; i <= posFPai; i++) {
            memoria[posicao] = memoria[i];
            posicao++;
        }

    } else {

        //Copiar o codigo do processo para a memoria.
        posicao += 10;

        for (int i = 0; i < (processo->maxPc) * 3; i++) {
            memoria[posicao] = processo->codigo[i];
            posicao++;
        }
    }

    processo->posicaoFinal = posicao - 1;
    apontadorDaUltimaAlocacao = posicao;
}

void printMemoria() {

    FILE *file = fopen("scheduler_complexo.out", "a");

    for (int i = 0; i < MAX_MEMORIA; i++) {
        if (i % 20 == 0 && i != 0) {
            fprintf(file, "\n");
        }
        fprintf(file, "%d ", memoria[i]);
    }

    fprintf(file, "\n");
    fclose(file);
}

void limparExit(int *processo_em_exit, int *n_processos_corridos, struct processo *processos[]) {

    if (*processo_em_exit != -1) {
        for (int x = processos[*processo_em_exit]->posicaoInicial;
             x <= processos[*processo_em_exit]->posicaoFinal; x++) {
            memoria[x] = -1;
        }
        processos[*processo_em_exit]->pcb->estado = -2;
        (*processo_em_exit) = -1;
        (*n_processos_corridos)++;
        printMemoria();
    }

}

void percorrerBlock(queue *block, queue *wait, struct processo *processos[], int first, int last) {

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

    processos[*processo_em_run]->pcb->pc++;
    processos[*processo_em_run]->pcb->estado = 3;
    processos[*processo_em_run]->tempo_que_precisa_de_ficar_em_block = 3;
    enqueue(*processo_em_run, block);
    (*processo_em_run) = -1;

}

void runParaExit(int *processo_em_run, int *processo_em_exit, queue *block, struct processo *processos[]) {

    processos[*processo_em_run]->pcb->estado = 4;
    (*processo_em_exit) = (*processo_em_run);
    (*processo_em_run) = -1;
}

void runParaWait(int *processo_em_run, queue *wait, struct processo *processos[]) {

    processos[*processo_em_run]->pcb->estado = 1;
    enqueue(*processo_em_run, wait);
    (*processo_em_run) = -1;
}

void newParaWait(int p_id, queue *wait, struct processo *processos[]) {

    int n_p = 0;
    
    while ((!isFull(wait)) && (n_p < p_id)) {
        if (processos[n_p]->pcb->estado == 0) {
            processos[n_p]->pcb->estado = 1;
            int posicao = obterPosicao(processos[n_p]);

            if (posicao != -1) {
                copiarParaMemoria(processos, processos[n_p], posicao, -1);
                enqueue(n_p, wait);
                printMemoria();
            }

        }
        n_p++;
    }
}

void waitParaRun(int *processo_em_run, queue *wait, struct processo *processos[]) {

    if (((*processo_em_run) == -1) && (!isEmpty(wait))) {
        (*processo_em_run) = dequeue(wait);
        processos[*processo_em_run]->pcb->estado = 2; // processo passa para run
        processos[*processo_em_run]->quantum = QUANTUM;
    }
}

void receberParaNew(int timer, int p_id, struct processo *processos[]) {

    for (int n_p = 0; n_p < p_id; n_p++) {
        if (processos[n_p]->instante == timer && processos[n_p]->pcb->estado == -1) {
            processos[n_p]->pcb->estado = 0;
        }
    }
}

void printEstados(int timer, int p_id, struct processo *processos[], int *print, int *fork) {

    FILE *file = fopen("scheduler_complexo.out", "a");
    FILE *file2 = fopen("scheduler_simples.out", "a");

    fprintf(file, "T: %3d | ", timer);
    fprintf(file2, "T: %3d | ", timer);


    //print de todos os processos introduzidos
    for (int n_p = 0; n_p < p_id; n_p++) {
        switch (processos[n_p]->pcb->estado) {
            case -2:
                fprintf(file, "      | ");
                fprintf(file2, "      | ");
                break;

            case 0:
                fprintf(file, "NEW   | ");
                fprintf(file2, "NEW   | ");
                break;

            case 1:
                fprintf(file, "WAIT  | ");
                fprintf(file2, "WAIT  | ");
                break;

            case 2:
                fprintf(file, "RUN   | ");
                fprintf(file2, "RUN   | ");
                break;

            case 3:
                fprintf(file, "BLOCK | ");
                fprintf(file2, "BLOCK | ");
                break;

            case 4:
                fprintf(file, "EXIT  | ");
                fprintf(file2, "EXIT  | ");
                break;

            default:
                break;
        }
    }

    if (*print != -1) {
        fprintf(file, "  %d", *print);
        fprintf(file2, "  %d", *print);
        *print = -1;
    }

    if (*fork == 1) {
        fprintf(file, " falha no fork");
        fprintf(file2, " falha no fork");
        *fork = 0;
    }

    fprintf(file, "\n");
    fprintf(file2, "\n");

    fclose(file);
    fclose(file2);

}

void debugPrint(int p_id, struct processo *processos[]) {

    printf("\nPROCESSOS\n");
    struct processo *aux;

    for (int i = 0; i < p_id; i++) {
        aux = processos[i];
        printf("%d estado: %d | pi = %d | pc = %d | pf = %d\n", aux->pcb->id, aux->pcb->estado, aux->posicaoInicial,
               aux->pcb->pc, aux->posicaoFinal);
    }
}

int main(void) {

    int instante, 
        p_id = 0, 
        controlo, 
        n_processos_corridos = 0,
        timer = 0,
        processo_em_run = -1,
        processo_em_exit = -1,
        algoParaImprimir = -1, 
        falhaFork = 0;

    struct processo *processos[N_MAXIMO_DE_PROCESSOS] = {0};
    char terminator;

    fclose(fopen("scheduler_complexo.out", "w"));
    fclose(fopen("scheduler_simples.out", "w"));

    //inicializa a memoria a -1 (posicao vazia)
    for (int i = 0; i < MAX_MEMORIA; i++) {
        memoria[i] = -1;
    }

    // le toda a informacao do ficheiro de teste
    while (scanf(" %d", &instante) != EOF) {
        int line[N_MAXIMO_DE_INSTRUCOES * 3] = {0};

        controlo = 0;
        struct pcb *nPCB = novoPcb(p_id, 0, -1);            // novo pcb para cada programa

        while (scanf("%c", &terminator) == 1) {                   // le a linha toda ate ao \n "codigo"

            if (terminator == '\n') {
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

        //debugPrint(p_id, processos);

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

            int pmemoria = pInicial + arg1 - 1,
                pmemoria2 = pInicial + arg2 - 1;

            switch (inst) {
                case 0:                                                 // x1=x2            
                    memoria[pmemoria] = memoria[pmemoria2];
                    processos[processo_em_run]->pcb->pc++;
                    break;

                case 1:                                                 // x=n

                    memoria[pmemoria] = arg2;
                    processos[processo_em_run]->pcb->pc++;
                    break;

                case 2:                                                 // x=x+1
                    memoria[pmemoria] += 1;
                    processos[processo_em_run]->pcb->pc++;
                    break;

                case 3:                                                 // x=x-1
                    memoria[pmemoria] -= 1;
                    processos[processo_em_run]->pcb->pc++;
                    break;

                case 4:                                                 // pc-=N
                    if (processos[processo_em_run]->pcb->pc - arg1 >= 0)
                        processos[processo_em_run]->pcb->pc -= arg1;

                    else {
                        FILE *file = fopen("scheduler_complexo.out", "a");
                        FILE *file2 = fopen("scheduler_simples.out", "a");

                        fprintf(file, "MEMORY ACCESS VIOLATION 4\n");
                        fprintf(file2, "MEMORY ACCESS VIOLATION 4\n");

                        fclose(file);
                        fclose(file2);
                        runParaExit(&processo_em_run, &processo_em_exit, block, processos);
                    }
                    break;

                case 5:                                                 // pc+=N
                    if (arg1 > 0) {
                        if (processos[processo_em_run]->pcb->pc + arg1 <= processos[processo_em_run]->maxPc)
                            processos[processo_em_run]->pcb->pc += arg1;

                        else {
                            FILE *file = fopen("scheduler_complexo.out", "a");
                            FILE *file2 = fopen("scheduler_simples.out", "a");

                            fprintf(file, "MEMORY ACCESS VIOLATION 5\n");
                            fprintf(file2, "MEMORY ACCESS VIOLATION 5\n");

                            fclose(file);
                            fclose(file2);

                            runParaExit(&processo_em_run, &processo_em_exit, block, processos);
                        }

                    } else if(arg1 == 0) {
                        processos[processo_em_run]->pcb->pc++;
                    }                    
                    break;

                case 6:                                                 // if x=0, pc+=N else pc++
                    if (arg1 == 0) {
                        if (processos[processo_em_run]->pcb->pc += arg2 <= processos[processo_em_run]->maxPc)
                            processos[processo_em_run]->pcb->pc += arg2;

                        else {
                            FILE *file = fopen("scheduler_complexo.out", "a");
                            FILE *file2 = fopen("scheduler_simples.out", "a");

                            fprintf(file, "MEMORY ACCESS VIOLATION 6\n");
                            fprintf(file2, "MEMORY ACCESS VIOLATION 6\n");

                            fclose(file);
                            fclose(file2);                            
                            runParaExit(&processo_em_run, &processo_em_exit, block, processos);
                        }

                    } else
                        processos[processo_em_run]->pcb->pc++;

                    break;

                case 7:                                                 // fork
                    processos[processo_em_run]->pcb->pc++;

                    if (!isFull(wait)) {
                        int posicao = obterPosicao(processos[processo_em_run]);

                        if (posicao != -1) {
                            struct pcb *newPcb = novoPcb(p_id, processos[processo_em_run]->pcb->pc + 1, 1);
                            processos[p_id] = novoProcesso(timer, processos[processo_em_run]->codigo, newPcb,
                                                           processos[processo_em_run]->maxPc * 3);

                            copiarParaMemoria(processos, processos[p_id], posicao, processo_em_run);

                            memoria[processos[p_id]->posicaoInicial + arg1 - 1] = 0;
                            memoria[processos[processo_em_run]->posicaoInicial + arg1 -
                                    1] = processos[processo_em_run]->pcb->id;
                            enqueue(p_id, wait);
                            printMemoria();
                            p_id++;

                        } else {
                            falhaFork = 1;
                            memoria[processos[processo_em_run]->posicaoInicial + arg1 - 1] = -1;
                        }

                    } else {
                        falhaFork = 1;
                        memoria[processos[processo_em_run]->posicaoInicial + arg1 - 1] = -1;
                    }
                    debugPrint(p_id, processos);
                    break;

                case 8:                                                 // guardar no disco
                    runParaBlock(&processo_em_run, block, processos);
                    break;

                case 9:                                                 // ler do disco
                    runParaBlock(&processo_em_run, block, processos);
                    break;

                case 10:                                                // imprimir variavel
                    algoParaImprimir = memoria[pInicial + arg1];
                    processos[processo_em_run]->pcb->pc++;
                    break;

                default:
                    runParaExit(&processo_em_run, &processo_em_exit, block, processos);
                    break;
            }

            //Se houver 1 processo em run, diminuir o quantum.
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

        timer++;

    }

    //print de todos os processos introduzidos, descomentar para ver.
    //debugPrint(p_id, processos);

    return 0;
}