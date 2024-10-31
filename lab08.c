#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define TAMANHO_PAGINA 4      // Tamanho da página em bytes
#define NUM_PAGES 4           // Número de páginas na memória virtual
#define NUM_FRAMES 2          // Número de frames na memória física
#define NUM_PROCESSES 4       // Número de processos

// Estrutura para representar uma página virtual
typedef struct {
    int page_number;
    bool is_loaded; // Indica se a página está carregada na memória física
} Page;

// Estrutura para representar um frame na memória física
typedef struct {
    int frame_number;
    Page *page; // Aponta para a página carregada neste frame
    bool is_free; // Indica se o frame está livre
} Frame;

// Estrutura para uma entrada na tabela de páginas
typedef struct {
    Page *page;
    int frame_number;
    bool valid; // Indica se o mapeamento é válido
} PageTableEntry;

// Estrutura para a tabela de páginas
typedef struct {
    PageTableEntry entries[NUM_PAGES];
} PageTable;

// Estrutura para gerenciar a memória física
typedef struct {
    Frame frames[NUM_FRAMES];
} PhysicalMemory;

// Estrutura para um processo
typedef struct {
    int pid;
    int *enderecos;       // Endereços virtuais que o processo irá acessar
    int num_enderecos;    // Número de endereços
} Processo;

// Funções para inicialização
void initialize_page_table(PageTable *page_table) {
    for (int i = 0; i < NUM_PAGES; i++) {
        page_table->entries[i].page = (Page *)malloc(sizeof(Page));
        if (!page_table->entries[i].page) {
            perror("Erro ao alocar memória para a página");
            exit(EXIT_FAILURE);
        }
        page_table->entries[i].page->page_number = i;
        page_table->entries[i].valid = false;
        page_table->entries[i].frame_number = -1; // Sem mapeamento inicial
    }
}

void initialize_physical_memory(PhysicalMemory *phy_memory) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        phy_memory->frames[i].frame_number = i;
        phy_memory->frames[i].page = NULL; // Frame inicialmente vazio
        phy_memory->frames[i].is_free = true; // Frame livre inicialmente
    }
}

// Funções para gerenciamento de memória
void map_page_to_frame(PageTable *page_table, int page_number, int frame_number) {
    if (page_number >= NUM_PAGES) {
        printf("Erro: Número da página inválido.\n");
        return;
    }

    PageTableEntry *entry = &page_table->entries[page_number];
    entry->frame_number = frame_number;
    entry->valid = true;
    entry->page->is_loaded = true;
}

bool load_page(PhysicalMemory *phy_memory, Page *page) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (phy_memory->frames[i].is_free) {
            phy_memory->frames[i].page = page;
            phy_memory->frames[i].is_free = false;
            return true; // Página carregada com sucesso
        }
    }
    return false; // Nenhum frame livre
}

int translate_address(PageTable *page_table, int virtual_address) {
    int page_number = virtual_address / TAMANHO_PAGINA; // Extraindo o número da página
    int offset = virtual_address % TAMANHO_PAGINA;       // Extraindo o offset

    // Verifica se a página está mapeada
    if (page_number < NUM_PAGES && page_table->entries[page_number].valid) {
        int frame_number = page_table->entries[page_number].frame_number;
        return (frame_number * TAMANHO_PAGINA) + offset; // Combinando frame e offset
    } else {
        printf("Erro: Página %d não encontrada na tabela de páginas (page fault).\n", page_number);
        return -1; // Indica que ocorreu uma falta de página
    }
}

// Função para liberar memória
void free_memory(PageTable *page_table) {
    for (int i = 0; i < NUM_PAGES; i++) {
        free(page_table->entries[i].page);
    }
}

// Função principal
int main() {
    PageTable page_tables[NUM_PROCESSES];
    PhysicalMemory physical_memory;
    Processo processes[NUM_PROCESSES];

    // Inicializa a tabela de páginas e a memória física
    initialize_physical_memory(&physical_memory);
    for (int i = 0; i < NUM_PROCESSES; i++) {
        initialize_page_table(&page_tables[i]);

        // Simulando a criação de um processo
        processes[i].pid = i + 1;
        processes[i].num_enderecos = NUM_PAGES;
        processes[i].enderecos = (int *)malloc(NUM_PAGES * sizeof(int));
        
        // Alocando endereços virtuais
        for (int j = 0; j < NUM_PAGES; j++) {
            processes[i].enderecos[j] = j * TAMANHO_PAGINA; // Ex.: 0, 4, 8, 12
            // Mapeia as páginas para os frames disponíveis
            if (j < NUM_FRAMES) {
                map_page_to_frame(&page_tables[i], j, j); // Mapeamento direto
            } else {
                page_tables[i].entries[j].valid = false; // Não pode mapear
            }
        }
    }

    // Testando a tradução de endereços virtuais para cada processo
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("Processo %d:\n", processes[i].pid);
        for (int j = 0; j < processes[i].num_enderecos; j++) {
            int virtual_address = processes[i].enderecos[j];
            int physical_address = translate_address(&page_tables[i], virtual_address);
            if (physical_address != -1) {
                printf("  Endereço físico para %d: %d\n", virtual_address, physical_address);
            }
        }
        printf("\n");
    }

    // Libera a memória alocada
    for (int i = 0; i < NUM_PROCESSES; i++) {
        free_memory(&page_tables[i]);
        free(processes[i].enderecos); // Liberando a memória dos endereços virtuais
    }

    return 0;
}

