#include "deduplication.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/md5.h>
#include <dirent.h>

// Fonction de hachage MD5 pour l'indexation
// dans la table de hachage
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

// Fonction permettant de chercher un MD5 dans la table de hachage
int find_md5(Md5Entry *hash_table, unsigned char *md5) {
    /* @param: hash_table est le tableau de hachage qui contient les MD5 et l'index des chunks unique
    *           md5 est le md5 du chunk dont on veut déterminer l'unicité
    *  @return: retourne l'index s'il trouve le md5 dans le tableau et -1 sinon
    */
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (hash_table[i].index != -1 && memcmp(hash_table[i].md5, md5, MD5_DIGEST_LENGTH) == 0) {
            return hash_table[i].index;
            }
    }
    return -1;
}

// Ajouter un MD5 dans la table de hachage
void add_md5(Md5Entry *hash_table, unsigned char *md5, int index) {
    unsigned int position = hash_md5(md5);
    int compteur = 0;
    while (hash_table[position].index != -1) {
        position = (position + 1) % HASH_TABLE_SIZE;
        compteur++;
        if (compteur >= HASH_TABLE_SIZE) {
            fprintf(stderr, "Erreur : Table de hachage pleine !\n");
            exit(EXIT_FAILURE);
        }
    }
    memcpy(hash_table[position].md5, md5, MD5_DIGEST_LENGTH);
    hash_table[position].index = index;
}

// Fonction pour convertir un fichier non dédupliqué en tableau de chunks
void deduplicate_file(FILE *file, Chunk *chunks, Md5Entry *hash_table) {
    /* @param:  file est le fichier qui sera dédupliqué
    *           chunks est le tableau de chunks initialisés qui contiendra les chunks issu du fichier
    *           hash_table est le tableau de hachage qui contient les MD5 et l'index des chunks unique
    */
    unsigned char buffer[CHUNK_SIZE];
    int chunk_index = 0;
    size_t octets_lus;
    unsigned char md5[MD5_DIGEST_LENGTH];

    while ((octets_lus = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        compute_md5(buffer, octets_lus, md5);

        int existing_index = find_md5(hash_table, md5);
        if (existing_index == -1) {
            chunks[chunk_index].data = malloc(octets_lus);
            if (!chunks[chunk_index].data) {
                perror("Erreur d'allocation mémoire");
                exit(EXIT_FAILURE);
            }
            memcpy(chunks[chunk_index].data, buffer, octets_lus);
            memcpy(chunks[chunk_index].md5, md5, MD5_DIGEST_LENGTH);
            add_md5(hash_table, md5, chunk_index);
            chunk_index++;
        } else {
            printf("Chunk %d déjà sauvegardé avec l'index : %d\n", chunk_index, existing_index);
        }
    }

    printf("Fichier dédupliqué avec succès. Nombre de chunks uniques : %d\n", chunk_index);
}


// Fonction permettant de charger un fichier dédupliqué en table de chunks
// en remplaçant les références par les données correspondantes
void undeduplicate_file(FILE *file, Chunk **chunks, int *chunk_count) {
    /* @param: file est le nom du fichier dédupliqué présent dans le répertoire de sauvegarde
    *           chunks représente le tableau de chunk qui contiendra les chunks restauré depuis filename
    *           chunk_count est un compteur du nombre de chunk restauré depuis le fichier filename
    */
    unsigned char buffer[CHUNK_SIZE];
    unsigned char md5[MD5_DIGEST_LENGTH];
    int index = 0;

    // Réinitialisation des chunks et du compteur
    *chunks = NULL;
    *chunk_count = 0;

    // Lecture du fichier dédupliqué
    while (fread(md5, 1, MD5_DIGEST_LENGTH, file) == MD5_DIGEST_LENGTH) {
        // Lecture des données du chunk
        if (fread(buffer, 1, CHUNK_SIZE, file) != CHUNK_SIZE) {
            fprintf(stderr, "Erreur: lecture des données du chunk échouée.\n");
            exit(EXIT_FAILURE);
        }

        // Ajout des données au tableau de chunks
        *chunks = realloc(*chunks, sizeof(Chunk) * (*chunk_count + 1));
        if (*chunks == NULL) {
            perror("Erreur d'allocation mémoire pour les chunks");
            exit(EXIT_FAILURE);
        }

        (*chunks)[*chunk_count].data = malloc(CHUNK_SIZE);
        if ((*chunks)[*chunk_count].data == NULL) {
            perror("Erreur d'allocation mémoire pour les données du chunk");
            exit(EXIT_FAILURE);
        }

        memcpy((*chunks)[*chunk_count].data, buffer, CHUNK_SIZE);
        memcpy((*chunks)[*chunk_count].md5, md5, MD5_DIGEST_LENGTH);
        (*chunk_count)++;
    }

    printf("Restauration réussie. Nombre de chunks restaurés : %d\n", *chunk_count);

}
