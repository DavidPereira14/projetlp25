#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#define CHUNK_SIZE 1024
#define HASH_TABLE_SIZE 100
#define MD5_DIGEST_LENGTH 16  //ahgagaglkaghlakghlaglkha

// Structure pour stocker les chunks
typedef struct {
    unsigned char data[CHUNK_SIZE];
    unsigned char md5[MD5_DIGEST_LENGTH];
} Chunk;

// Structure pour stocker les entrées dans la table de hachage
typedef struct {
    unsigned char md5[MD5_DIGEST_LENGTH];
    int index;
} Md5Entry;

// Fonction qui vérifie l'existence du fichier
int file_exists(const char *filename) {
    if (access(filename, F_OK) == 0) {
        return 1;
    }
    return 0;
}

// Fonction de hachage MD5 pour l'indexation
unsigned int hash_md5(unsigned char *md5) {
    unsigned int hash = 0;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        hash = (hash << 5) + hash + md5[i];
    }
    return hash % HASH_TABLE_SIZE;
}

// Fonction pour calculer le MD5 d'un chunk
void compute_md5(void *data, size_t len, unsigned char *md5_out) {
    MD5(data, len, md5_out);
}

// Fonction pour chercher un MD5 dans la table de hachage
int find_md5(Md5Entry *hash_table, unsigned char *md5) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (memcmp(hash_table[i].md5, md5, MD5_DIGEST_LENGTH) == 0) {
            return hash_table[i].index;
        }
    }
    return -1;
}

// Ajouter un MD5 dans la table de hachage
void add_md5(Md5Entry *hash_table, unsigned char *md5, int index) {
    unsigned int position = hash_md5(md5);
    while (hash_table[position].index != -1) {
        position = (position + 1) % HASH_TABLE_SIZE;
    }
    memcpy(hash_table[position].md5, md5, MD5_DIGEST_LENGTH);
    hash_table[position].index = index;
}

// Fonction pour dédupliquer le fichier
void deduplicate_file(FILE *file, Chunk *chunks, Md5Entry *hash_table) {
    unsigned char buffer[CHUNK_SIZE];
    int chunk_index = 0;
    size_t octets_lu;
    unsigned char md5[MD5_DIGEST_LENGTH];

    while ((octets_lu = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        compute_md5(buffer, octets_lu, md5);

        int existing_index = find_md5(hash_table, md5);
        if (existing_index == -1) {
            memcpy(chunks[chunk_index].data, buffer, octets_lu);
            memcpy(chunks[chunk_index].md5, md5, MD5_DIGEST_LENGTH);
            add_md5(hash_table, md5, chunk_index);
            chunk_index++;
        } else {
            printf("Chunk %d déjà sauvegardé avec l'index : %d\n", chunk_index, existing_index);
        }
    }
    printf("Fichier dédupliqué avec succès. Nombre de chunks uniques : %d\n", chunk_index);
}

// Fonction pour écrire le backup dans un fichier
void write_backup_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Erreur d'ouverture du fichier de sauvegarde");
        return;
    }

    for (int i = 0; i < chunk_count; i++) {
        size_t data_size = CHUNK_SIZE; // Utiliser la taille fixe du chunk
        if (fwrite(chunks[i].data, 1, data_size, output_file) != data_size) {
            perror("Erreur d'écriture dans le fichier");
            fclose(output_file);
            return;
        }
    }

    fclose(output_file);
    printf("Fichier de sauvegarde créé : %s\n", output_filename);
}

// Fonction pour restaurer le fichier à partir du backup
void write_restored_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Erreur lors de l'ouverture du fichier de sortie");
        return;
    }

    Md5Entry hash_table[HASH_TABLE_SIZE] = {0};

    for (int i = 0; i < chunk_count; i++) {
        int index = find_md5(hash_table, chunks[i].md5);
        if (index == -1) {
            add_md5(hash_table, chunks[i].md5, i);
            if (fwrite(chunks[i].data, 1, CHUNK_SIZE, output_file) != CHUNK_SIZE) {
                fprintf(stderr, "Erreur lors de l'écriture du chunk %d\n", i);
                fclose(output_file);
                return;
            }
        } else {
            printf("Chunk %d déjà écrit (référence trouvée dans la table de hachage).\n", i);
        }
    }

    fclose(output_file);
    printf("Fichier restauré avec succès dans '%s'\n", output_filename);
}

int main() {
    char filename[256];
    printf("Entrez le nom du fichier à sauvegarder : ");
    scanf("%s", filename);

    if (!file_exists(filename)) {
        printf("Le fichier %s n'existe pas.\n", filename);
        return 1;
    }

    FILE *file = fopen(filename, "rb");
    Md5Entry hash_table[HASH_TABLE_SIZE] = {0};
    Chunk chunks[100];
    deduplicate_file(file, chunks, hash_table);
    write_backup_file("backup_file.bin", chunks, 100);
    fclose(file);

    char restore_choice;
    printf("Souhaitez-vous restaurer le fichier sauvegardé (y/n) ? ");
    scanf(" %c", &restore_choice);

    if (restore_choice == 'y' || restore_choice == 'Y') {
        printf("Restauration du fichier...\n");
        write_restored_file("restored_file.txt", chunks, 100);
    }

    return 0;
}